#include "ota_engine.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "ota_engine";

static SemaphoreHandle_t ota_mutex = NULL;
static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *ota_partition = NULL;
static size_t ota_received_bytes = 0;
static size_t ota_total_size = 0;
static ota_state_t ota_state = OTA_STATE_IDLE;
static TickType_t ota_last_write_time = 0;

#define OTA_TIMEOUT_MS 30000

esp_err_t ota_engine_init(void)
{
    if (!ota_mutex) {
        ota_mutex = xSemaphoreCreateMutex();
        if (!ota_mutex) return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t ota_engine_start(size_t firmware_size)
{
    if (!ota_mutex) return ESP_ERR_INVALID_STATE;
    
    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    if (ota_state != OTA_STATE_IDLE) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    
    if (firmware_size == 0 || firmware_size > 4*1024*1024) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }
    
    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        xSemaphoreGive(ota_mutex);
        return ESP_FAIL;
    }
    
    if (firmware_size > ota_partition->size) {
        ESP_LOGE(TAG, "Firmware too large: %zu > %zu", firmware_size, ota_partition->size);
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }
    
    esp_err_t ret = esp_ota_begin(ota_partition, firmware_size, &ota_handle);
    if (ret != ESP_OK) {
        xSemaphoreGive(ota_mutex);
        return ret;
    }
    
    ota_total_size = firmware_size;
    ota_received_bytes = 0;
    ota_state = OTA_STATE_ACTIVE;
    ota_last_write_time = xTaskGetTickCount();
    
    ESP_LOGI(TAG, "OTA started: %zu bytes to partition %s", firmware_size, ota_partition->label);
    
    xSemaphoreGive(ota_mutex);
    return ESP_OK;
}

esp_err_t ota_engine_write(const uint8_t *data, size_t len)
{
    if (!ota_mutex) return ESP_ERR_INVALID_STATE;
    
    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    if (ota_state != OTA_STATE_ACTIVE || !ota_handle) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    
    TickType_t now = xTaskGetTickCount();
    if ((now - ota_last_write_time) > pdMS_TO_TICKS(OTA_TIMEOUT_MS)) {
        ESP_LOGE(TAG, "OTA timeout");
        xSemaphoreGive(ota_mutex);
        ota_engine_abort();
        return ESP_ERR_TIMEOUT;
    }
    
    if (ota_received_bytes + len > ota_total_size) {
        ESP_LOGE(TAG, "Write exceeds firmware size");
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }
    
    esp_err_t ret = esp_ota_write(ota_handle, data, len);
    if (ret == ESP_OK) {
        ota_received_bytes += len;
        ota_last_write_time = now;
    }
    
    xSemaphoreGive(ota_mutex);
    return ret;
}

esp_err_t ota_engine_finish(void)
{
    if (!ota_mutex) return ESP_ERR_INVALID_STATE;
    
    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    if (ota_state != OTA_STATE_ACTIVE || !ota_handle) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    
    if (ota_received_bytes != ota_total_size) {
        ESP_LOGW(TAG, "Size mismatch: received %zu, expected %zu", ota_received_bytes, ota_total_size);
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }
    
    ota_state = OTA_STATE_FINALIZING;
    
    esp_err_t ret = esp_ota_end(ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA validation failed: %s", esp_err_to_name(ret));
        ota_handle = 0;
        ota_state = OTA_STATE_IDLE;
        xSemaphoreGive(ota_mutex);
        return ret;
    }
    
    ESP_LOGI(TAG, "OTA complete: %zu bytes written to %s", ota_received_bytes, ota_partition->label);
    ESP_LOGI(TAG, "Image validated. Call esp_ota_set_boot_partition() and reboot to activate.");
    
    ota_handle = 0;
    ota_state = OTA_STATE_IDLE;
    
    xSemaphoreGive(ota_mutex);
    return ESP_OK;
}

esp_err_t ota_engine_abort(void)
{
    if (!ota_mutex) return ESP_ERR_INVALID_STATE;
    
    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    if (ota_handle) {
        esp_ota_abort(ota_handle);
        ota_handle = 0;
        ESP_LOGW(TAG, "OTA aborted");
    }
    
    ota_received_bytes = 0;
    ota_total_size = 0;
    ota_state = OTA_STATE_IDLE;
    
    xSemaphoreGive(ota_mutex);
    return ESP_OK;
}

size_t ota_engine_get_progress(void)
{
    if (ota_total_size > 0) {
        return (ota_received_bytes * 100) / ota_total_size;
    }
    return 0;
}

ota_state_t ota_engine_get_state(void)
{
    return ota_state;
}
