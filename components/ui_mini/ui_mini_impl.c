#include "ui_interface.h"
#include "system_events.h"
#include "ui_mini.h"
#include "ui_screen_controller.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char *TAG = "ui_mini_impl";

static TaskHandle_t ui_task_handle = NULL;
static bool ui_running = false;

// Internal state
static struct {
    uint8_t battery_level;
    bool battery_charging;
    bool usb_connected;
    bool lora_connected;
    int8_t lora_rssi;
    char last_command[16];
    device_mode_t current_mode;
} ui_state = {0};

static void battery_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const system_event_battery_t *evt = (const system_event_battery_t *)data;
    ui_state.battery_level = evt->level;
    ui_state.battery_charging = evt->charging;
    
    // Update UI status
    ui_mini_status_t status = {
        .battery_level = ui_state.battery_level,
        .usb_connected = ui_state.usb_connected,
        .lora_connected = ui_state.lora_connected,
        .lora_signal = ui_state.lora_rssi,
    };
    ui_mini_update_status(&status);
}

static void usb_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const system_event_usb_t *evt = (const system_event_usb_t *)data;
    ui_state.usb_connected = evt->connected;
    
    if (ui_state.current_mode == DEVICE_MODE_PC && !evt->connected) {
        ui_mini_show_message("PC Mode", "Connect USB Cable", 3000);
    }
    
    ui_mini_status_t status = {
        .battery_level = ui_state.battery_level,
        .usb_connected = ui_state.usb_connected,
        .lora_connected = ui_state.lora_connected,
        .lora_signal = ui_state.lora_rssi,
    };
    ui_mini_update_status(&status);
}

static void lora_state_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const system_event_lora_t *evt = (const system_event_lora_t *)data;
    ui_state.lora_connected = evt->connected;
    ui_state.lora_rssi = evt->rssi;
    
    ui_mini_status_t status = {
        .battery_level = ui_state.battery_level,
        .usb_connected = ui_state.usb_connected,
        .lora_connected = ui_state.lora_connected,
        .lora_signal = ui_state.lora_rssi,
    };
    ui_mini_update_status(&status);
}

static void lora_command_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const system_event_lora_cmd_t *evt = (const system_event_lora_cmd_t *)data;
    strncpy(ui_state.last_command, evt->command, sizeof(ui_state.last_command) - 1);
    ui_state.lora_rssi = evt->rssi;
    
    ui_mini_status_t status = {
        .battery_level = ui_state.battery_level,
        .usb_connected = ui_state.usb_connected,
        .lora_connected = ui_state.lora_connected,
        .lora_signal = ui_state.lora_rssi,
    };
    strncpy(status.last_command, ui_state.last_command, sizeof(status.last_command) - 1);
    ui_mini_update_status(&status);
}

static void button_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const system_event_button_t *evt = (const system_event_button_t *)data;
    
    button_event_t ui_event;
    switch (evt->type) {
        case BUTTON_EVENT_SHORT:
            ui_event = BUTTON_EVENT_SHORT;
            break;
        case BUTTON_EVENT_LONG:
            ui_event = BUTTON_EVENT_LONG;
            break;
        case BUTTON_EVENT_DOUBLE:
            ui_event = BUTTON_EVENT_DOUBLE;
            break;
        default:
            return;
    }
    
    ui_screen_controller_handle_button(ui_event);
}

static void ota_progress_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const system_event_ota_t *evt = (const system_event_ota_t *)data;
    
    // Switch to OTA screen if not already there
    if (ui_mini_get_screen() != OLED_SCREEN_OTA_UPDATE) {
        ui_mini_set_screen(OLED_SCREEN_OTA_UPDATE);
    }
    
    // Update progress
    extern void ota_update_screen_set_progress(uint8_t percent, const char *status);
    ota_update_screen_set_progress(evt->percent, evt->status);
}

static void mode_changed_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const system_event_mode_t *evt = (const system_event_mode_t *)data;
    ui_state.current_mode = evt->mode;
    
    // Update screen based on mode
    ui_mini_set_screen(OLED_SCREEN_MAIN);
    
    ui_mini_status_t status = {
        .battery_level = ui_state.battery_level,
        .usb_connected = ui_state.usb_connected,
        .lora_connected = ui_state.lora_connected,
        .lora_signal = ui_state.lora_rssi,
    };
    ui_mini_update_status(&status);
}

static void ui_task(void *arg)
{
    ESP_LOGI(TAG, "UI task started");
    
    esp_event_loop_handle_t loop = system_events_get_loop();
    
    // Subscribe to all system events
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        loop, SYSTEM_EVENTS, SYSTEM_EVENT_BATTERY_CHANGED,
        battery_event_handler, NULL));
    
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        loop, SYSTEM_EVENTS, SYSTEM_EVENT_USB_CHANGED,
        usb_event_handler, NULL));
    
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        loop, SYSTEM_EVENTS, SYSTEM_EVENT_LORA_STATE_CHANGED,
        lora_state_event_handler, NULL));
    
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        loop, SYSTEM_EVENTS, SYSTEM_EVENT_LORA_COMMAND_RECEIVED,
        lora_command_event_handler, NULL));
    
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        loop, SYSTEM_EVENTS, SYSTEM_EVENT_BUTTON_PRESSED,
        button_event_handler, NULL));
    
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        loop, SYSTEM_EVENTS, SYSTEM_EVENT_OTA_PROGRESS,
        ota_progress_event_handler, NULL));
    
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        loop, SYSTEM_EVENTS, SYSTEM_EVENT_MODE_CHANGED,
        mode_changed_event_handler, NULL));
    
    // Show boot screen
    ui_mini_set_screen(OLED_SCREEN_BOOT);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Transition to main screen
    ui_mini_set_screen(OLED_SCREEN_MAIN);
    
    // Main loop - just keep task alive
    while (ui_running) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Unregister event handlers
    esp_event_handler_unregister_with(loop, SYSTEM_EVENTS, 
                                      SYSTEM_EVENT_BATTERY_CHANGED, battery_event_handler);
    esp_event_handler_unregister_with(loop, SYSTEM_EVENTS,
                                      SYSTEM_EVENT_USB_CHANGED, usb_event_handler);
    esp_event_handler_unregister_with(loop, SYSTEM_EVENTS,
                                      SYSTEM_EVENT_LORA_STATE_CHANGED, lora_state_event_handler);
    esp_event_handler_unregister_with(loop, SYSTEM_EVENTS,
                                      SYSTEM_EVENT_LORA_COMMAND_RECEIVED, lora_command_event_handler);
    esp_event_handler_unregister_with(loop, SYSTEM_EVENTS,
                                      SYSTEM_EVENT_BUTTON_PRESSED, button_event_handler);
    esp_event_handler_unregister_with(loop, SYSTEM_EVENTS,
                                      SYSTEM_EVENT_OTA_PROGRESS, ota_progress_event_handler);
    esp_event_handler_unregister_with(loop, SYSTEM_EVENTS,
                                      SYSTEM_EVENT_MODE_CHANGED, mode_changed_event_handler);
    
    ESP_LOGI(TAG, "UI task stopped");
    ui_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t ui_init(void)
{
    ESP_LOGI(TAG, "Initializing UI Mini");
    
    // Initialize ui_mini core
    esp_err_t ret = ui_mini_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ui_mini: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create UI task
    ui_running = true;
    BaseType_t task_ret = xTaskCreate(ui_task, "ui_mini", 4096, NULL, 5, &ui_task_handle);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UI task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "UI Mini initialized successfully");
    return ESP_OK;
}

esp_err_t ui_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing UI Mini");
    
    ui_running = false;
    
    // Wait for task to finish
    if (ui_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    return ESP_OK;
}
