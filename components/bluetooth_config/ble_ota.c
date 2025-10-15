#include "ble_ota.h"

#ifdef SIMULATOR_BUILD

esp_err_t ble_ota_service_init(esp_gatt_if_t gatts_if) { (void)gatts_if; return ESP_ERR_NOT_SUPPORTED; }
void ble_ota_handle_control_write(const uint8_t *data, uint16_t len) { (void)data; (void)len; }
void ble_ota_handle_data_write(const uint8_t *data, uint16_t len) { (void)data; (void)len; }
void ble_ota_send_response(uint8_t response, const char *message) { (void)response; (void)message; }
void ble_ota_update_progress(uint8_t percentage) { (void)percentage; }

#else

#include "ota_engine.h"
#include "esp_log.h"
#include "esp_gatts_api.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "BLE_OTA";

// External handles set by bluetooth_config.c
extern uint16_t ota_control_handle;
extern uint16_t ota_progress_handle;
extern esp_gatt_if_t ota_gatts_if;

static size_t expected_size = 0;
static uint8_t current_progress = 0;

void ble_ota_send_response(uint8_t response, const char *message)
{
    if (!ota_gatts_if || !ota_control_handle) return;
    
    uint8_t data[64];
    data[0] = response;
    
    if (message) {
        size_t msg_len = strlen(message);
        if (msg_len > 63) msg_len = 63;
        memcpy(data + 1, message, msg_len);
        esp_ble_gatts_send_indicate(ota_gatts_if, 0, ota_control_handle, msg_len + 1, data, false);
    } else {
        esp_ble_gatts_send_indicate(ota_gatts_if, 0, ota_control_handle, 1, data, false);
    }
}

void ble_ota_update_progress(uint8_t percentage)
{
    if (!ota_gatts_if || !ota_progress_handle) return;
    
    current_progress = percentage;
    esp_ble_gatts_send_indicate(ota_gatts_if, 0, ota_progress_handle, 1, &percentage, false);
}

void ble_ota_handle_control_write(const uint8_t *data, uint16_t len)
{
    if (len < 1) {
        ble_ota_send_response(OTA_RESP_ERROR, "Invalid command");
        return;
    }
    
    uint8_t cmd = data[0];
    
    switch (cmd) {
        case OTA_CMD_START:
            if (len < 5) {
                ble_ota_send_response(OTA_RESP_ERROR, "Missing size");
                return;
            }
            
            expected_size = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
            
            if (expected_size == 0 || expected_size > 4*1024*1024) {
                ble_ota_send_response(OTA_RESP_ERROR, "Invalid size");
                return;
            }
            
            esp_err_t ret = ota_engine_start(expected_size);
            if (ret != ESP_OK) {
                ble_ota_send_response(OTA_RESP_ERROR, "OTA start failed");
                ESP_LOGE(TAG, "OTA start failed: %s", esp_err_to_name(ret));
                return;
            }
            
            current_progress = 0;
            ble_ota_send_response(OTA_RESP_READY, NULL);
            ESP_LOGI(TAG, "OTA started via BLE: %zu bytes", expected_size);
            break;
            
        case OTA_CMD_ABORT:
            ota_engine_abort();
            expected_size = 0;
            current_progress = 0;
            ble_ota_send_response(OTA_RESP_READY, "Aborted");
            ESP_LOGI(TAG, "OTA aborted");
            break;
            
        case OTA_CMD_FINISH: {
            esp_err_t ret = ota_engine_finish();
            if (ret == ESP_OK) {
                // Set boot partition after successful upload
                const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
                if (update_partition) {
                    ret = esp_ota_set_boot_partition(update_partition);
                    if (ret == ESP_OK) {
                        ble_ota_send_response(OTA_RESP_COMPLETE, "Rebooting...");
                        ESP_LOGI(TAG, "OTA complete via BLE, rebooting...");
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        esp_restart();
                    } else {
                        ble_ota_send_response(OTA_RESP_ERROR, "Failed to set boot partition");
                    }
                } else {
                    ble_ota_send_response(OTA_RESP_ERROR, "No update partition");
                }
            } else {
                ble_ota_send_response(OTA_RESP_ERROR, "OTA finish failed");
                ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(ret));
            }
            break;
        }
            
        default:
            ble_ota_send_response(OTA_RESP_ERROR, "Unknown command");
            break;
    }
}

void ble_ota_handle_data_write(const uint8_t *data, uint16_t len)
{
    if (ota_engine_get_state() != OTA_STATE_ACTIVE) {
        ESP_LOGW(TAG, "OTA not active, ignoring data");
        return;
    }
    
    esp_err_t ret = ota_engine_write(data, len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(ret));
        ota_engine_abort();
        ble_ota_send_response(OTA_RESP_ERROR, "Write failed");
        return;
    }
    
    uint8_t progress = (uint8_t)ota_engine_get_progress();
    if (progress != current_progress && progress % 5 == 0) {
        ble_ota_update_progress(progress);
    }
}

esp_err_t ble_ota_service_init(esp_gatt_if_t gatts_if)
{
    (void)gatts_if;  // Set externally in bluetooth_config.c
    ESP_LOGI(TAG, "BLE OTA service initialized");
    return ESP_OK;
}

#endif
