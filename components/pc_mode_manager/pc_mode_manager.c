/**
 * @file pc_mode_manager.c
 * @brief PC Mode Manager implementation
 */

#include "pc_mode_manager.h"
#include "device_registry.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "system_events.h"
#include "usb_hid.h"
#include <string.h>

static const char *TAG = "PC_MODE_MGR";

// Active presenter tracking
typedef struct {
    uint16_t device_id;
    int16_t last_rssi;
    uint32_t last_seen_ms;
    uint32_t command_count;
} active_presenter_t;

#define MAX_ACTIVE_PRESENTERS 4
static active_presenter_t active_presenters[MAX_ACTIVE_PRESENTERS] = {0};

// Rate limiter
typedef struct {
    uint32_t last_command_ms;
    uint32_t command_count_1s;
} rate_limiter_t;

static rate_limiter_t rate_limiter = {0};
static SemaphoreHandle_t state_mutex = NULL;

static bool rate_limiter_check(void)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (now - rate_limiter.last_command_ms > 1000) {
        rate_limiter.command_count_1s = 0;
    }

    if (rate_limiter.command_count_1s >= 10) {
        return false;
    }

    rate_limiter.last_command_ms = now;
    rate_limiter.command_count_1s++;
    return true;
}

static void update_active_presenter(uint16_t device_id, int16_t rssi)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    int slot = -1;

    // Find existing or empty slot
    for (int i = 0; i < MAX_ACTIVE_PRESENTERS; i++) {
        if (active_presenters[i].device_id == device_id) {
            slot = i;
            break;
        }
        if (slot == -1 && active_presenters[i].device_id == 0) {
            slot = i;
        }
    }

    // Expire old entries (>30s)
    for (int i = 0; i < MAX_ACTIVE_PRESENTERS; i++) {
        if (active_presenters[i].device_id != 0 && 
            (now - active_presenters[i].last_seen_ms) > 30000) {
            ESP_LOGI(TAG, "Presenter 0x%04X expired", active_presenters[i].device_id);
            memset(&active_presenters[i], 0, sizeof(active_presenter_t));
        }
    }

    if (slot != -1) {
        active_presenters[slot].device_id = device_id;
        active_presenters[slot].last_rssi = rssi;
        active_presenters[slot].last_seen_ms = now;
        active_presenters[slot].command_count++;
    }
}

esp_err_t pc_mode_manager_init(void)
{
    state_mutex = xSemaphoreCreateMutex();
    if (!state_mutex) {
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "PC mode manager initialized");
    return ESP_OK;
}

esp_err_t pc_mode_manager_process_command(uint16_t device_id, uint16_t sequence_num,
                                          lora_command_t command, const uint8_t *payload,
                                          uint8_t payload_length, int16_t rssi)
{
    if (!state_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(state_mutex, portMAX_DELAY);

    ESP_LOGI(TAG, "Processing: device=0x%04X, seq=%u, cmd=0x%02X, rssi=%d dBm",
             device_id, sequence_num, command, rssi);

    // Device pairing validation
    if (!device_registry_is_paired(device_id)) {
        ESP_LOGW(TAG, "Ignoring command from unpaired device 0x%04X", device_id);
        xSemaphoreGive(state_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    // Track active presenter
    update_active_presenter(device_id, rssi);

    // Rate limiting
    if (!rate_limiter_check()) {
        ESP_LOGW(TAG, "Rate limit exceeded (>10 cmd/s)");
        xSemaphoreGive(state_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    usb_hid_keycode_t keycode = 0;
    uint8_t modifiers = 0;

    // Parse HID report from payload
    if (command == CMD_HID_REPORT && payload_length >= sizeof(lora_payload_t)) {
        const lora_payload_t *pkt = (const lora_payload_t *)payload;
        uint8_t hid_type = LORA_HID_TYPE(pkt->type_flags);

        if (hid_type == HID_TYPE_KEYBOARD) {
            keycode = pkt->hid_report.keyboard.keycode[0];
            modifiers = pkt->hid_report.keyboard.modifiers;
        }

        // Forward to USB HID if connected
        if (usb_hid_is_connected() && keycode != 0) {
            ESP_LOGI(TAG, "Forwarding keycode 0x%02X to USB HID", keycode);
            usb_hid_send_key(keycode);
        }

        // Post HID command event for UI
        system_event_hid_command_t hid_evt = {
            .device_id = device_id,
            .hid_type = hid_type,
            .hid_report = {
                pkt->hid_report.keyboard.modifiers,
                pkt->hid_report.keyboard.keycode[0],
                pkt->hid_report.keyboard.keycode[1],
                pkt->hid_report.keyboard.keycode[2],
                pkt->hid_report.keyboard.keycode[3]
            },
            .flags = LORA_FLAGS(pkt->type_flags),
            .rssi = rssi
        };
        system_events_post_hid_command(&hid_evt);
    }

    xSemaphoreGive(state_mutex);
    return ESP_OK;
}

void pc_mode_manager_deinit(void)
{
    if (state_mutex) {
        vSemaphoreDelete(state_mutex);
        state_mutex = NULL;
    }
    ESP_LOGI(TAG, "PC mode manager deinitialized");
}
