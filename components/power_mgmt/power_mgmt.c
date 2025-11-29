/**
 * @file power_mgmt.c
 * @brief Power management implementation for LoRaCue battery optimization
 *
 * CONTEXT: ESP32-S3 power modes with GPIO wake and timer wake
 * TARGET: >24h battery life with 1000mAh battery
 */

#include "power_mgmt.h"
#include "usb_hid.h"
#include "bsp.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc.h"

static const char *TAG = "POWER_MGMT";

// Power management state
static bool power_mgmt_initialized   = false;
static power_config_t current_config = {0};
static power_stats_t power_stats     = {0};
static uint64_t last_activity_time   = 0;
static uint64_t session_start_time   = 0;
static bool display_sleeping         = false;

// Default timeout constants
#define POWER_MGMT_DEFAULT_DISPLAY_SLEEP_MS  10000   // 10 seconds
#define POWER_MGMT_DEFAULT_LIGHT_SLEEP_MS    30000   // 30 seconds
#define POWER_MGMT_DEFAULT_DEEP_SLEEP_MS     300000  // 5 minutes

// Default configuration
static const power_config_t default_config = {
    .display_sleep_timeout_ms  = POWER_MGMT_DEFAULT_DISPLAY_SLEEP_MS,
    .light_sleep_timeout_ms    = POWER_MGMT_DEFAULT_LIGHT_SLEEP_MS,
    .deep_sleep_timeout_ms     = POWER_MGMT_DEFAULT_DEEP_SLEEP_MS,
    .enable_auto_display_sleep = true,
    .enable_auto_light_sleep   = true,
    .enable_auto_deep_sleep    = true,
    .cpu_freq_mhz              = 80, // 80MHz for power efficiency
};

// GPIO pins for wake (button)
#define WAKE_GPIO_BUTTON 0 // User button on Heltec V3

esp_err_t power_mgmt_init(const power_config_t *config)
{
    ESP_LOGI(TAG, "Initializing power management");

    // Use provided config or default
    if (config) {
        current_config = *config;
    } else {
        current_config = default_config;
    }

    // Configure power management
    esp_pm_config_t pm_config = {.max_freq_mhz       = current_config.cpu_freq_mhz,
                                 .min_freq_mhz       = 10, // Minimum frequency for power saving
                                 .light_sleep_enable = current_config.enable_auto_light_sleep};

    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NOT_SUPPORTED) {
            ESP_LOGW(TAG, "Power management not supported (simulator mode)");
            // Continue initialization for simulator compatibility
        } else {
            ESP_LOGE(TAG, "Failed to configure power management: %s", esp_err_to_name(ret));
            return ret;
        }
    } else {
        ESP_LOGI(TAG, "Power management configured successfully");
    }

    // Configure GPIO wake sources (button)
    esp_sleep_enable_ext0_wakeup(WAKE_GPIO_BUTTON, 0); // Wake on LOW (button pressed)

    // Configure GPIO pin for wake
    gpio_config_t wake_gpio_config = {
        .pin_bit_mask = (1ULL << WAKE_GPIO_BUTTON),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&wake_gpio_config);

    // Initialize timestamps
    last_activity_time = esp_timer_get_time();
    session_start_time = last_activity_time;

    // Check wake cause
    esp_sleep_wakeup_cause_t wake_cause = esp_sleep_get_wakeup_cause();
    switch (wake_cause) {
        case ESP_SLEEP_WAKEUP_EXT1:
            ESP_LOGI(TAG, "Wake from button press");
            power_stats.wake_count_button++;
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wake from timer");
            power_stats.wake_count_timer++;
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            ESP_LOGI(TAG, "Cold boot or reset");
            break;
        default:
            ESP_LOGI(TAG, "Wake cause: %d", wake_cause);
            break;
    }

    power_mgmt_initialized = true;
    ESP_LOGI(TAG, "Power management initialized (CPU: %dMHz, Display: %s, Light: %s, Deep: %s)",
             current_config.cpu_freq_mhz, current_config.enable_auto_display_sleep ? "ON" : "OFF",
             current_config.enable_auto_light_sleep ? "ON" : "OFF",
             current_config.enable_auto_deep_sleep ? "ON" : "OFF");

    return ESP_OK;
}

esp_err_t power_mgmt_display_sleep(void)
{
    if (!power_mgmt_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!display_sleeping) {
        ESP_LOGD(TAG, "Entering display sleep");
        display_sleeping = true;

        uint64_t current_time = esp_timer_get_time();
        power_stats.display_sleep_time_ms += (current_time - last_activity_time) / 1000;
        
        // Turn off display backlight
        bsp_display_sleep();
    }

    return ESP_OK;
}

esp_err_t power_mgmt_light_sleep(uint32_t timeout_ms)
{
    if (!power_mgmt_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Entering light sleep for %dms", timeout_ms);

    display_sleeping = true;

    uint64_t sleep_start = esp_timer_get_time();

    // Configure wake sources
    if (timeout_ms > 0) {
        esp_sleep_enable_timer_wakeup(timeout_ms * 1000ULL);
    }
    
    // Enable button wake (GPIO0)
    esp_sleep_enable_ext0_wakeup(WAKE_GPIO_BUTTON, 0);
    
    // Enable UART wake
    esp_sleep_enable_uart_wakeup(UART_NUM_0);

    esp_err_t ret = esp_light_sleep_start();

    uint64_t sleep_end      = esp_timer_get_time();
    uint32_t sleep_duration = (sleep_end - sleep_start) / 1000;

    power_stats.light_sleep_time_ms += sleep_duration;
    power_mgmt_update_activity();

    // Reinitialize display properly
    display_sleeping = false;

    ESP_LOGI(TAG, "Woke from light sleep after %dms", sleep_duration);

    return ret;
}

esp_err_t power_mgmt_deep_sleep(uint32_t timeout_ms)
{
    if (!power_mgmt_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Entering deep sleep for %dms", timeout_ms);

    // Update statistics before deep sleep
    uint64_t current_time = esp_timer_get_time();
    power_stats.active_time_ms += (current_time - session_start_time) / 1000;

    // Prepare for sleep
    power_mgmt_prepare_sleep();

    // Configure wake sources
    if (timeout_ms > 0) {
        esp_sleep_enable_timer_wakeup(timeout_ms * 1000ULL);
    }
    
    // Enable button wake (GPIO0)
    esp_sleep_enable_ext0_wakeup(WAKE_GPIO_BUTTON, 0);

    // Enter deep sleep (function doesn't return)
    esp_deep_sleep_start();

    return ESP_OK; // Never reached
}

esp_err_t power_mgmt_update_activity(void)
{
    if (!power_mgmt_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    last_activity_time = esp_timer_get_time();

    if (display_sleeping) {
        display_sleeping = false;
        bsp_display_wake();
    }

    return ESP_OK;
}

power_mode_t power_mgmt_get_recommended_mode(void)
{
    if (!power_mgmt_initialized) {
        return POWER_MODE_ACTIVE;
    }

    uint64_t current_time     = esp_timer_get_time();
    uint32_t inactive_time_ms = (current_time - last_activity_time) / 1000;

    // Don't sleep when USB HID is connected (PC mode)
    if (usb_hid_is_connected()) {
        return POWER_MODE_ACTIVE;
    }

    if (current_config.enable_auto_deep_sleep && inactive_time_ms >= current_config.deep_sleep_timeout_ms) {
        return POWER_MODE_DEEP_SLEEP;
    } else if (current_config.enable_auto_light_sleep && inactive_time_ms >= current_config.light_sleep_timeout_ms) {
        return POWER_MODE_LIGHT_SLEEP;
    } else if (current_config.enable_auto_display_sleep &&
               inactive_time_ms >= current_config.display_sleep_timeout_ms) {
        return POWER_MODE_DISPLAY_SLEEP;
    }

    return POWER_MODE_ACTIVE;
}

esp_err_t power_mgmt_get_stats(power_stats_t *stats)
{
    if (!power_mgmt_initialized || !stats) {
        return ESP_ERR_INVALID_ARG;
    }

    // Update current session active time
    uint64_t current_time       = esp_timer_get_time();
    uint32_t current_session_ms = (current_time - session_start_time) / 1000;

    *stats = power_stats;
    stats->active_time_ms += current_session_ms;

    // Estimate battery life (rough calculation)
    // Assumptions: 1000mAh battery, 10mA active, 8mA display sleep, 1mA light sleep, 0.01mA deep sleep
    float avg_current_ma = (stats->active_time_ms * 10.0f + stats->display_sleep_time_ms * 8.0f +
                            stats->light_sleep_time_ms * 1.0f + stats->deep_sleep_time_ms * 0.01f) /
                           (stats->active_time_ms + stats->display_sleep_time_ms + stats->light_sleep_time_ms +
                            stats->deep_sleep_time_ms + 1);

    stats->estimated_battery_hours = 1000.0f / avg_current_ma; // mAh / mA = hours

    return ESP_OK;
}

esp_err_t power_mgmt_set_cpu_freq(uint8_t freq_mhz)
{
    if (!power_mgmt_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (freq_mhz != 80 && freq_mhz != 160 && freq_mhz != 240) {
        ESP_LOGE(TAG, "Invalid CPU frequency: %d MHz", freq_mhz);
        return ESP_ERR_INVALID_ARG;
    }

    current_config.cpu_freq_mhz = freq_mhz;

    esp_pm_config_t pm_config = {
        .max_freq_mhz = freq_mhz, .min_freq_mhz = 10, .light_sleep_enable = current_config.enable_auto_light_sleep};

    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "CPU frequency set to %d MHz", freq_mhz);
    } else {
        ESP_LOGE(TAG, "Failed to set CPU frequency: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_sleep_wakeup_cause_t power_mgmt_get_wake_cause(void)
{
    return esp_sleep_get_wakeup_cause();
}

esp_err_t power_mgmt_prepare_sleep(void)
{
    ESP_LOGD(TAG, "Preparing system for deep sleep");

    // Note: Peripherals will be powered down by deep sleep
    // No need to manually free buses - device will reboot on wake

    ESP_LOGD(TAG, "System prepared for deep sleep");
    return ESP_OK;
}

// Note: After deep sleep, device reboots and main() reinitializes everything
// No restore function needed
