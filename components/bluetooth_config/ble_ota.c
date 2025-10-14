#include "ble_ota.h"

#ifdef SIMULATOR_BUILD

esp_err_t ble_ota_init(void) { return ESP_ERR_NOT_SUPPORTED; }
void ble_ota_handle_control(const uint8_t *data, uint16_t len) { (void)data; (void)len; }
void ble_ota_handle_data(const uint8_t *data, uint16_t len) { (void)data; (void)len; }

#else

#include "ota_engine.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "BLE_OTA";

void ble_ota_handle_control(const uint8_t *data, uint16_t len)
{
    if (len < 17 || strncmp((char*)data, "FIRMWARE_UPGRADE ", 17) != 0) {
        ESP_LOGW(TAG, "Invalid control command");
        return;
    }
    
    size_t size = atoi((char*)data + 17);
    if (size == 0 || size > 4*1024*1024) {
        ESP_LOGE(TAG, "Invalid firmware size: %zu", size);
        return;
    }
    
    esp_err_t ret = ota_engine_start(size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA start failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "OTA started via BLE: %zu bytes", size);
}

void ble_ota_handle_data(const uint8_t *data, uint16_t len)
{
    if (ota_engine_get_state() != OTA_STATE_ACTIVE) {
        return;
    }
    
    esp_err_t ret = ota_engine_write(data, len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(ret));
        ota_engine_abort();
        return;
    }
    
    size_t progress = ota_engine_get_progress();
    if (progress == 100) {
        ret = ota_engine_finish();
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "OTA complete via BLE, rebooting...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(ret));
        }
    }
}

esp_err_t ble_ota_init(void)
{
    ESP_LOGI(TAG, "BLE OTA handlers initialized");
    return ESP_OK;
}

#endif
