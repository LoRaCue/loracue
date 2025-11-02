#include "ui_interface.h"
#include "system_events.h"
#include "button_manager.h"
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

static void hid_command_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const system_event_hid_command_t *evt = (const system_event_hid_command_t *)data;
    
    // Extract keyboard data for display
    uint8_t modifiers = 0, keycode = 0;
    system_event_get_keyboard_data(evt, &modifiers, &keycode);
    
    // Map keycode to command name for display
    const char *cmd_name = "KEY";
    switch (keycode) {
        // Navigation keys
        case 0x4E: cmd_name = "PgDn"; break;
        case 0x4B: cmd_name = "PgUp"; break;
        case 0x4F: cmd_name = "→"; break;      // Right arrow
        case 0x50: cmd_name = "←"; break;      // Left arrow
        case 0x51: cmd_name = "↓"; break;      // Down arrow
        case 0x52: cmd_name = "↑"; break;      // Up arrow
        case 0x4A: cmd_name = "Home"; break;
        case 0x4D: cmd_name = "End"; break;
        
        // Special keys
        case 0x2C: cmd_name = "Space"; break;
        case 0x28: cmd_name = "Enter"; break;
        case 0x2A: cmd_name = "BkSpc"; break;
        case 0x2B: cmd_name = "Tab"; break;
        case 0x29: cmd_name = "Esc"; break;
        
        // Function keys
        case 0x3A: cmd_name = "F1"; break;
        case 0x3B: cmd_name = "F2"; break;
        case 0x3C: cmd_name = "F3"; break;
        case 0x3D: cmd_name = "F4"; break;
        case 0x3E: cmd_name = "F5"; break;
        case 0x3F: cmd_name = "F6"; break;
        case 0x40: cmd_name = "F7"; break;
        case 0x41: cmd_name = "F8"; break;
        case 0x42: cmd_name = "F9"; break;
        case 0x43: cmd_name = "F10"; break;
        case 0x44: cmd_name = "F11"; break;
        case 0x45: cmd_name = "F12"; break;
        
        // Letters (a-z)
        case 0x04: cmd_name = "A"; break;
        case 0x05: cmd_name = "B"; break;
        case 0x06: cmd_name = "C"; break;
        case 0x07: cmd_name = "D"; break;
        case 0x08: cmd_name = "E"; break;
        case 0x09: cmd_name = "F"; break;
        case 0x0A: cmd_name = "G"; break;
        case 0x0B: cmd_name = "H"; break;
        case 0x0C: cmd_name = "I"; break;
        case 0x0D: cmd_name = "J"; break;
        case 0x0E: cmd_name = "K"; break;
        case 0x0F: cmd_name = "L"; break;
        case 0x10: cmd_name = "M"; break;
        case 0x11: cmd_name = "N"; break;
        case 0x12: cmd_name = "O"; break;
        case 0x13: cmd_name = "P"; break;
        case 0x14: cmd_name = "Q"; break;
        case 0x15: cmd_name = "R"; break;
        case 0x16: cmd_name = "S"; break;
        case 0x17: cmd_name = "T"; break;
        case 0x18: cmd_name = "U"; break;
        case 0x19: cmd_name = "V"; break;
        case 0x1A: cmd_name = "W"; break;
        case 0x1B: cmd_name = "X"; break;
        case 0x1C: cmd_name = "Y"; break;
        case 0x1D: cmd_name = "Z"; break;
        
        // Numbers (1-9, 0)
        case 0x1E: cmd_name = "1"; break;
        case 0x1F: cmd_name = "2"; break;
        case 0x20: cmd_name = "3"; break;
        case 0x21: cmd_name = "4"; break;
        case 0x22: cmd_name = "5"; break;
        case 0x23: cmd_name = "6"; break;
        case 0x24: cmd_name = "7"; break;
        case 0x25: cmd_name = "8"; break;
        case 0x26: cmd_name = "9"; break;
        case 0x27: cmd_name = "0"; break;
        
        // Special characters
        case 0x2D: cmd_name = "-"; break;
        case 0x2E: cmd_name = "="; break;
        case 0x2F: cmd_name = "["; break;
        case 0x30: cmd_name = "]"; break;
        case 0x31: cmd_name = "\\"; break;
        case 0x33: cmd_name = ";"; break;
        case 0x34: cmd_name = "'"; break;
        case 0x35: cmd_name = "`"; break;
        case 0x36: cmd_name = ","; break;
        case 0x37: cmd_name = "."; break;
        case 0x38: cmd_name = "/"; break;
    }
    
    strncpy(ui_state.last_command, cmd_name, sizeof(ui_state.last_command) - 1);
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
    
    button_event_type_t ui_event;
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
    
    // Update OTA progress
    ui_mini_update_ota_progress(evt->percent);
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
        loop, SYSTEM_EVENTS, SYSTEM_EVENT_HID_COMMAND_RECEIVED,
        hid_command_event_handler, NULL));
    
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
                                      SYSTEM_EVENT_HID_COMMAND_RECEIVED, hid_command_event_handler);
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
