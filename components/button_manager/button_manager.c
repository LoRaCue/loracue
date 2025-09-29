/**
 * @file button_manager.c
 * @brief Button manager implementation with timing and debouncing
 * 
 * CONTEXT: Converts raw button states to UI events with proper timing
 * TIMING: Short press <1.5s, Long press >1.5s, Both buttons >3s
 */

#include "button_manager.h"
#include "bsp.h"
#include "led_manager.h"
#include "power_mgmt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "BUTTON_MGR";

// Timing constants (in milliseconds)
#define DEBOUNCE_TIME_MS        50
#define LONG_PRESS_TIME_MS      1500
#define BOTH_PRESS_TIME_MS      3000
#define INACTIVITY_TIMEOUT_MS   300000  // 5 minutes

// Button manager state
static TaskHandle_t button_task_handle = NULL;
static bool manager_running = false;
static uint32_t last_activity_time = 0;

// Button state tracking
typedef struct {
    bool current_state;
    bool last_state;
    uint32_t press_start_time;
    bool long_press_sent;
} button_state_t;

static button_state_t prev_button = {0};
static button_state_t next_button = {0};

static void update_button_state(button_state_t *btn, bool pressed, uint32_t current_time)
{
    // Debouncing
    if (pressed != btn->last_state) {
        btn->last_state = pressed;
        return; // Wait for next cycle to confirm
    }
    
    // State change detection
    if (pressed && !btn->current_state) {
        // Button press detected
        btn->current_state = true;
        btn->press_start_time = current_time;
        btn->long_press_sent = false;
        last_activity_time = current_time;
        ESP_LOGD(TAG, "Button pressed");
    } else if (!pressed && btn->current_state) {
        // Button release detected
        btn->current_state = false;
        last_activity_time = current_time;
        ESP_LOGD(TAG, "Button released");
    }
}

static void button_manager_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Button manager task started");
    
    last_activity_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    while (manager_running) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Read button states
        bool prev_pressed = bsp_read_button(BSP_BUTTON_PREV);
        bool next_pressed = bsp_read_button(BSP_BUTTON_NEXT);
        
        // Control LED based on button state
        static bool any_button_was_pressed = false;
        bool any_button_pressed = prev_pressed || next_pressed;
        
        if (any_button_pressed && !any_button_was_pressed) {
            // Button just pressed - go solid
            led_manager_solid(true);
        } else if (!any_button_pressed && any_button_was_pressed) {
            // Button just released - resume fading
            led_manager_fade(3000);
        }
        any_button_was_pressed = any_button_pressed;
        
        // Update button states
        update_button_state(&prev_button, prev_pressed, current_time);
        update_button_state(&next_button, next_pressed, current_time);
        
        ui_event_t event = UI_EVENT_NONE;
        
        // Check for both buttons pressed (highest priority)
        if (prev_button.current_state && next_button.current_state) {
            uint32_t both_press_duration = current_time - 
                ((prev_button.press_start_time > next_button.press_start_time) ? 
                 prev_button.press_start_time : next_button.press_start_time);
                 
            if (both_press_duration >= BOTH_PRESS_TIME_MS && !prev_button.long_press_sent) {
                event = UI_EVENT_BOTH_LONG;
                prev_button.long_press_sent = true;
                next_button.long_press_sent = true;
                ESP_LOGI(TAG, "Both buttons long press");
            }
        }
        // Check individual button events
        else {
            // PREV button events
            if (prev_button.current_state) {
                uint32_t press_duration = current_time - prev_button.press_start_time;
                if (press_duration >= LONG_PRESS_TIME_MS && !prev_button.long_press_sent) {
                    event = UI_EVENT_PREV_LONG;
                    prev_button.long_press_sent = true;
                    ESP_LOGI(TAG, "PREV long press");
                }
            } else if (!prev_button.current_state && prev_button.press_start_time > 0) {
                // Button was released
                uint32_t press_duration = current_time - prev_button.press_start_time;
                if (press_duration < LONG_PRESS_TIME_MS && !prev_button.long_press_sent) {
                    event = UI_EVENT_PREV_SHORT;
                    ESP_LOGI(TAG, "PREV short press");
                }
                prev_button.press_start_time = 0;
            }
            
            // NEXT button events (only if no PREV event)
            if (event == UI_EVENT_NONE) {
                if (next_button.current_state) {
                    uint32_t press_duration = current_time - next_button.press_start_time;
                    if (press_duration >= LONG_PRESS_TIME_MS && !next_button.long_press_sent) {
                        event = UI_EVENT_NEXT_LONG;
                        next_button.long_press_sent = true;
                        ESP_LOGI(TAG, "NEXT long press");
                    }
                } else if (!next_button.current_state && next_button.press_start_time > 0) {
                    // Button was released
                    uint32_t press_duration = current_time - next_button.press_start_time;
                    if (press_duration < LONG_PRESS_TIME_MS && !next_button.long_press_sent) {
                        event = UI_EVENT_NEXT_SHORT;
                        ESP_LOGI(TAG, "NEXT short press");
                    }
                    next_button.press_start_time = 0;
                }
            }
        }
        
        // Check for inactivity timeout and power management
        if (event == UI_EVENT_NONE) {
            // Check if system should enter sleep mode
            power_mode_t recommended_mode = power_mgmt_get_recommended_mode();
            
            if (recommended_mode == POWER_MODE_LIGHT_SLEEP) {
                ESP_LOGI(TAG, "Entering light sleep due to inactivity");
                power_mgmt_light_sleep(30000); // 30 second light sleep
                last_activity_time = current_time; // Reset after wake
            } else if (recommended_mode == POWER_MODE_DEEP_SLEEP) {
                ESP_LOGI(TAG, "Entering deep sleep due to extended inactivity");
                power_mgmt_deep_sleep(0); // Indefinite deep sleep, wake on button only
            }
            
            uint32_t inactive_time = current_time - last_activity_time;
            if (inactive_time >= INACTIVITY_TIMEOUT_MS) {
                event = UI_EVENT_TIMEOUT;
                last_activity_time = current_time; // Reset to prevent repeated timeouts
                ESP_LOGI(TAG, "Inactivity timeout");
            }
        }
        
        // Send event to UI
        if (event != UI_EVENT_NONE) {
            oled_ui_handle_event(event);
        }
        
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
    }
    
    ESP_LOGI(TAG, "Button manager task stopped");
    vTaskDelete(NULL);
}

esp_err_t button_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing button manager");
    
    // Reset button states
    memset(&prev_button, 0, sizeof(button_state_t));
    memset(&next_button, 0, sizeof(button_state_t));
    
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
    
    BaseType_t ret = xTaskCreate(
        button_manager_task,
        "button_mgr",
        4096,  // Increased from 2048 to prevent stack overflow
        NULL,
        5, // Priority
        &button_task_handle
    );
    
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
