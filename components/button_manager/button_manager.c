/**
 * @file button_manager.c
 * @brief Button manager implementation with timing and debouncing
 *
 * CONTEXT: Converts raw button states to UI events with proper timing
 * TIMING: Short press <500ms, Double press 2 clicks <500ms, Long press >2s
 */

#include "button_manager.h"
#include "system_events.h"
#include "bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_manager.h"
#include "power_mgmt.h"
#include <string.h>

static const char *TAG = "BUTTON_MGR";

// Timing constants (in milliseconds)
#define DEBOUNCE_TIME_MS 50
#define SHORT_PRESS_MAX_MS 500
#define DOUBLE_CLICK_WINDOW_MS 200
#define LONG_PRESS_TIME_MS 1500
#define INACTIVITY_TIMEOUT_MS 300000 // 5 minutes

// Button manager state
static TaskHandle_t button_task_handle = NULL;
static bool manager_running            = false;
static bool display_sleep_active       = false;
static uint32_t last_activity_time     = 0;

// Button event callback
static button_event_callback_t event_callback = NULL;
static void *callback_arg                     = NULL;

// Button state tracking for single button
typedef struct {
    bool pressed;
    uint32_t press_start_time;
    uint32_t last_release_time;
    bool long_press_sent;
    uint8_t click_count;
} button_state_t;

static button_state_t button = {0};

static void button_manager_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Button manager task started");

    last_activity_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    while (manager_running) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // Read single button state (GPIO 0 on Heltec V3)
        bool btn_pressed = bsp_read_button(BSP_BUTTON_NEXT);

        // LED control
        static bool was_pressed = false;
        if (btn_pressed && !was_pressed) {
            led_manager_solid(true);
        } else if (!btn_pressed && was_pressed) {
            led_manager_fade(3000);
        }
        was_pressed = btn_pressed;

        // Button press detection
        if (btn_pressed && !button.pressed) {
            button.pressed          = true;
            button.press_start_time = current_time;
            button.long_press_sent  = false;
            last_activity_time      = current_time;
            display_sleep_active    = false;  // Wake from display sleep
            led_manager_button_feedback(true);

            bsp_display_wake();
            power_mgmt_update_activity();

            ESP_LOGD(TAG, "Button pressed");
        }
        // Button release detection
        else if (!btn_pressed && button.pressed) {
            button.pressed          = false;
            uint32_t press_duration = current_time - button.press_start_time;
            last_activity_time      = current_time;
            led_manager_button_feedback(false); // Restore LED

            // Check if long press was already sent
            if (button.long_press_sent) {
                ESP_LOGD(TAG, "Button released (long press already sent)");
                button.click_count = 0;
            }
            // Short press detected
            else if (press_duration < SHORT_PRESS_MAX_MS) {
                button.click_count++;
                button.last_release_time = current_time;
                ESP_LOGD(TAG, "Short press detected (count: %d)", button.click_count);
            }
        }
        // Long press detection (while still pressed)
        else if (button.pressed && !button.long_press_sent) {
            uint32_t press_duration = current_time - button.press_start_time;
            if (press_duration >= LONG_PRESS_TIME_MS) {
                ESP_LOGI(TAG, "Long press");
                system_events_post_button(BUTTON_EVENT_LONG);
                if (event_callback)
                    event_callback(BUTTON_EVENT_LONG, callback_arg);
                button.long_press_sent = true;
                button.click_count     = 0;
            }
        }

        // Double-click detection (after release)
        if (!button.pressed && button.click_count > 0) {
            uint32_t time_since_release = current_time - button.last_release_time;

            if (button.click_count == 2) {
                ESP_LOGI(TAG, "Double press");
                system_events_post_button(BUTTON_EVENT_DOUBLE);
                if (event_callback)
                    event_callback(BUTTON_EVENT_DOUBLE, callback_arg);
                button.click_count = 0;
            } else if (time_since_release >= DOUBLE_CLICK_WINDOW_MS) {
                // Single click confirmed
                ESP_LOGI(TAG, "Short press");
                system_events_post_button(BUTTON_EVENT_SHORT);
                if (event_callback)
                    event_callback(BUTTON_EVENT_SHORT, callback_arg);
                button.click_count = 0;
            }
        }

        // Power management
        {
            power_mode_t recommended_mode = power_mgmt_get_recommended_mode();

            if (recommended_mode == POWER_MODE_DISPLAY_SLEEP) {
                if (!display_sleep_active) {
                    ESP_LOGI(TAG, "Entering display sleep due to inactivity");
                    display_sleep_active = true;
                }
                power_mgmt_display_sleep();
            } else if (recommended_mode == POWER_MODE_LIGHT_SLEEP) {
                ESP_LOGI(TAG, "Entering light sleep due to inactivity");
                // Use configured timeout from power_mgmt
                power_mgmt_light_sleep(0); // 0 = indefinite, wake on button/UART
                // Display and peripherals restored by power_mgmt_light_sleep()
                last_activity_time = current_time;
            } else if (recommended_mode == POWER_MODE_DEEP_SLEEP) {
                ESP_LOGI(TAG, "Entering deep sleep due to extended inactivity");
                power_mgmt_deep_sleep(0);
            }

            uint32_t inactive_time = current_time - last_activity_time;
            if (inactive_time >= INACTIVITY_TIMEOUT_MS) {
                last_activity_time = current_time;
                ESP_LOGI(TAG, "Inactivity timeout");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
    }

    ESP_LOGI(TAG, "Button manager task stopped");
    vTaskDelete(NULL);
}

esp_err_t button_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing button manager");

    // Reset button state
    memset(&button, 0, sizeof(button_state_t));

    ESP_LOGI(TAG, "Button manager initialized");
    return ESP_OK;
}

esp_err_t button_manager_start(void)
{
    if (manager_running) {
        ESP_LOGW(TAG, "Button manager already running");
        return ESP_OK;
    }

    manager_running = true;

    BaseType_t ret = xTaskCreate(button_manager_task, "button_mgr",
                                 4096, // Increased from 2048 to prevent stack overflow
                                 NULL,
                                 5, // Priority
                                 &button_task_handle);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button manager task");
        manager_running = false;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Button manager started");
    return ESP_OK;
}

esp_err_t button_manager_stop(void)
{
    if (!manager_running) {
        return ESP_OK;
    }

    manager_running = false;

    if (button_task_handle) {
        // Task will delete itself when manager_running becomes false
        button_task_handle = NULL;
    }

    ESP_LOGI(TAG, "Button manager stopped");
    return ESP_OK;
}

esp_err_t button_manager_register_callback(button_event_callback_t callback, void *arg)
{
    event_callback = callback;
    callback_arg   = arg;
    return ESP_OK;
}
