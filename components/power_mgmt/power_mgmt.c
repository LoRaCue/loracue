/**
 * @file power_mgmt.c
 * @brief Power management implementation for LoRaCue battery optimization
 *
 * CONTEXT: ESP32-S3 power modes with GPIO wake and timer wake
 * TARGET: >24h battery life with 1000mAh battery
 */

#include "power_mgmt.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
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

// Default configuration
static const power_config_t default_config = {
    .light_sleep_timeout_ms  = 30000,  // 30 seconds
    .deep_sleep_timeout_ms   = 300000, // 5 minutes
    .enable_auto_light_sleep = true,
    .enable_auto_deep_sleep  = true,
    .cpu_freq_mhz            = 80, // 80MHz for power efficiency
};

// GPIO pins for wake (buttons)
#define WAKE_GPIO_PREV 45 // PREV button
#define WAKE_GPIO_NEXT 46 // NEXT button

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

    // Configure GPIO wake sources (buttons)
    esp_sleep_enable_ext1_wakeup((1ULL << WAKE_GPIO_PREV) | (1ULL << WAKE_GPIO_NEXT), ESP_EXT1_WAKEUP_ANY_HIGH);

    // Configure GPIO pins for wake
    gpio_config_t wake_gpio_config = {
        .pin_bit_mask = (1ULL << WAKE_GPIO_PREV) | (1ULL << WAKE_GPIO_NEXT),
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
    ESP_LOGI(TAG, "Power management initialized (CPU: %dMHz, Light sleep: %s, Deep sleep: %s)",
             current_config.cpu_freq_mhz, current_config.enable_auto_light_sleep ? "ON" : "OFF",
             current_config.enable_auto_deep_sleep ? "ON" : "OFF");

    return ESP_OK;
}

esp_err_t power_mgmt_light_sleep(uint32_t timeout_ms)
{
    if (!power_mgmt_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Entering light sleep for %dms", timeout_ms);

    // Prepare for sleep
    power_mgmt_prepare_sleep();

    uint64_t sleep_start = esp_timer_get_time();

    if (timeout_ms > 0) {
        esp_sleep_enable_timer_wakeup(timeout_ms * 1000ULL); // Convert to microseconds
    }

    // Enter light sleep
    esp_err_t ret = esp_light_sleep_start();

    uint64_t sleep_end      = esp_timer_get_time();
    uint32_t sleep_duration = (sleep_end - sleep_start) / 1000; // Convert to milliseconds

    power_stats.light_sleep_time_ms += sleep_duration;

    // Restore after wake
    power_mgmt_restore_wake();

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

    if (timeout_ms > 0) {
        esp_sleep_enable_timer_wakeup(timeout_ms * 1000ULL); // Convert to microseconds
    }

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
    return ESP_OK;
}

power_mode_t power_mgmt_get_recommended_mode(void)
{
    if (!power_mgmt_initialized) {
        return POWER_MODE_ACTIVE;
    }

    uint64_t current_time     = esp_timer_get_time();
    uint32_t inactive_time_ms = (current_time - last_activity_time) / 1000;

    if (current_config.enable_auto_deep_sleep && inactive_time_ms >= current_config.deep_sleep_timeout_ms) {
        return POWER_MODE_DEEP_SLEEP;
    } else if (current_config.enable_auto_light_sleep && inactive_time_ms >= current_config.light_sleep_timeout_ms) {
        return POWER_MODE_LIGHT_SLEEP;
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
    // Assumptions: 1000mAh battery, 10mA active, 1mA light sleep, 0.01mA deep sleep
    float avg_current_ma =
        (stats->active_time_ms * 10.0f + stats->light_sleep_time_ms * 1.0f + stats->deep_sleep_time_ms * 0.01f) /
        (stats->active_time_ms + stats->light_sleep_time_ms + stats->deep_sleep_time_ms + 1);

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
    ESP_LOGD(TAG, "Preparing system for sleep");

    // TODO: Disable unnecessary peripherals before sleep
    // - Deinitialize I2C bus (oled_dev_handle, i2c_bus_handle)
    // - Disable SPI for LoRa
    // - Save peripheral states for restoration

    return ESP_OK;
}

esp_err_t power_mgmt_restore_wake(void)
{
    ESP_LOGD(TAG, "Restoring system after wake");

    // TODO: Re-enable and reinitialize peripherals after wake
    // - Reinitialize I2C bus for OLED (call bsp_init_i2c())
    // - Reinitialize SPI for LoRa
    // - Restore peripheral configurations
    // CRITICAL: Without this, I2C operations crash with semaphore assertion failures
    // For now, just update activity timestamp

    power_mgmt_update_activity();

    return ESP_OK;
}
