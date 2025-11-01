#include "system_events.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "system_events";

ESP_EVENT_DEFINE_BASE(SYSTEM_EVENTS);

static esp_event_loop_handle_t event_loop = NULL;

esp_err_t system_events_init(void)
{
    if (event_loop != NULL) {
        return ESP_OK;
    }

    esp_event_loop_args_t loop_args = {
        .queue_size = 32,
        .task_name = "sys_events",
        .task_priority = 10,
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY
    };

    esp_err_t ret = esp_event_loop_create(&loop_args, &event_loop);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "System event loop initialized");
    return ESP_OK;
}

esp_event_loop_handle_t system_events_get_loop(void)
{
    return event_loop;
}

esp_err_t system_events_post_battery(uint8_t level, bool charging)
{
    system_event_battery_t data = {
        .level = level,
        .charging = charging
    };
    return esp_event_post_to(event_loop, SYSTEM_EVENTS, SYSTEM_EVENT_BATTERY_CHANGED,
                             &data, sizeof(data), portMAX_DELAY);
}

esp_err_t system_events_post_usb(bool connected)
{
    system_event_usb_t data = { .connected = connected };
    return esp_event_post_to(event_loop, SYSTEM_EVENTS, SYSTEM_EVENT_USB_CHANGED,
                             &data, sizeof(data), portMAX_DELAY);
}

esp_err_t system_events_post_lora_state(bool connected, int8_t rssi)
{
    system_event_lora_t data = {
        .connected = connected,
        .rssi = rssi
    };
    return esp_event_post_to(event_loop, SYSTEM_EVENTS, SYSTEM_EVENT_LORA_STATE_CHANGED,
                             &data, sizeof(data), portMAX_DELAY);
}

esp_err_t system_events_post_lora_command(const char *command, int8_t rssi)
{
    system_event_lora_cmd_t data = { .rssi = rssi };
    strncpy(data.command, command, sizeof(data.command) - 1);
    return esp_event_post_to(event_loop, SYSTEM_EVENTS, SYSTEM_EVENT_LORA_COMMAND_RECEIVED,
                             &data, sizeof(data), portMAX_DELAY);
}

esp_err_t system_events_post_button(button_event_type_t type)
{
    system_event_button_t data = { .type = type };
    return esp_event_post_to(event_loop, SYSTEM_EVENTS, SYSTEM_EVENT_BUTTON_PRESSED,
                             &data, sizeof(data), portMAX_DELAY);
}

esp_err_t system_events_post_ota_progress(uint8_t percent, const char *status)
{
    system_event_ota_t data = { .percent = percent };
    strncpy(data.status, status, sizeof(data.status) - 1);
    return esp_event_post_to(event_loop, SYSTEM_EVENTS, SYSTEM_EVENT_OTA_PROGRESS,
                             &data, sizeof(data), portMAX_DELAY);
}

esp_err_t system_events_post_mode_changed(device_mode_t mode)
{
    system_event_mode_t data = { .mode = mode };
    return esp_event_post_to(event_loop, SYSTEM_EVENTS, SYSTEM_EVENT_MODE_CHANGED,
                             &data, sizeof(data), portMAX_DELAY);
}
