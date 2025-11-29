#include "ui_interface.h"
#include "system_events.h"
#include "ui_rich.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ui_rich_impl";

static TaskHandle_t ui_task_handle = NULL;
static bool ui_running = false;

static void battery_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    system_event_battery_t *evt = (system_event_battery_t *)data;
    // Update UI with battery info
    ESP_LOGI(TAG, "Battery: %d%% %s", evt->level, evt->charging ? "charging" : "");
}

static void usb_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    system_event_usb_t *evt = (system_event_usb_t *)data;
    ESP_LOGI(TAG, "USB: %s", evt->connected ? "connected" : "disconnected");
}

static void lora_state_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    system_event_lora_t *evt = (system_event_lora_t *)data;
    ESP_LOGI(TAG, "LoRa: %s RSSI=%d", evt->connected ? "connected" : "disconnected", evt->rssi);
}

static void lora_command_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    system_event_lora_cmd_t *evt = (system_event_lora_cmd_t *)data;
    ESP_LOGI(TAG, "LoRa command: %s RSSI=%d", evt->command, evt->rssi);
}

static void button_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    system_event_button_t *evt = (system_event_button_t *)data;
    ESP_LOGI(TAG, "Button: %d", evt->type);
}

static void ota_progress_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    system_event_ota_t *evt = (system_event_ota_t *)data;
    ESP_LOGI(TAG, "OTA: %d%% - %s", evt->percent, evt->status);
}

static void mode_changed_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    system_event_mode_t *evt = (system_event_mode_t *)data;
    ESP_LOGI(TAG, "Mode changed: %d", evt->mode);
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
    ui_rich_show_bootscreen();
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Main loop
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
    ESP_LOGI(TAG, "Initializing UI Rich");
    
    // Initialize ui_rich core
    esp_err_t ret = ui_rich_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ui_rich: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create UI task
    ui_running = true;
    BaseType_t task_ret = xTaskCreate(ui_task, "ui_rich", 8192, NULL, 5, &ui_task_handle);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UI task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "UI Rich initialized successfully");
    return ESP_OK;
}

esp_err_t ui_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing UI Rich");
    
    ui_running = false;
    
    // Wait for task to finish
    if (ui_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    return ESP_OK;
}
