/**
 * @file ble_ota_handler.c
 * @brief BLE OTA handler with streaming firmware update
 *
 * Security model:
 * - Requires bonded/paired connection
 * - BLE pairing provides transport security
 * - Only accepts OTA from authenticated devices
 */

#include "ble_ota.h"
#include "bsp.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "host/ble_gap.h"

#ifdef CONFIG_LORACUE_UI_COMPACT
#include "ui_data_update_task.h"
#include "ui_mini.h"
#endif

#ifdef CONFIG_LORACUE_UI_RICH
#include "ui_rich.h"
#endif

#define OTA_RINGBUF_SIZE 4096
#define OTA_TASK_SIZE 8192

static const char *TAG           = "ble_ota";
static RingbufHandle_t s_ringbuf = NULL;
SemaphoreHandle_t notify_sem     = NULL; // Global - required by BLE OTA library
static esp_ota_handle_t out_handle;
static TaskHandle_t s_ota_task_handle = NULL;
static uint16_t s_ota_conn_handle     = BLE_HS_CONN_HANDLE_NONE;

// Forward declaration
static void ota_task(void *arg);

bool ble_ota_ringbuf_init(uint32_t ringbuf_size)
{
    s_ringbuf = xRingbufferCreate(ringbuf_size, RINGBUF_TYPE_BYTEBUF);
    return s_ringbuf != NULL;
}

size_t write_to_ringbuf(const uint8_t *data, size_t size)
{
    BaseType_t done = xRingbufferSend(s_ringbuf, (void *)data, size, (TickType_t)portMAX_DELAY);
    return done ? size : 0;
}

void ota_recv_fw_cb(uint8_t *buf, uint32_t length)
{
    // Security check: Require bonded connection
    if (s_ota_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        struct ble_gap_conn_desc desc;
        if (ble_gap_conn_find(s_ota_conn_handle, &desc) == 0) {
            if (!desc.sec_state.bonded) {
                ESP_LOGE(TAG, "OTA rejected: device not bonded");
                return;
            }
        } else {
            ESP_LOGE(TAG, "OTA rejected: connection not found");
            return;
        }
    }

    // Create OTA task on first data reception
    if (s_ota_task_handle == NULL) {
        ESP_LOGI(TAG, "OTA transfer started from bonded device");
        xTaskCreate(&ota_task, "ota_task", OTA_TASK_SIZE, NULL, 5, &s_ota_task_handle);
    }
    write_to_ringbuf(buf, length);
}

static void ota_task(void *arg)
{
    esp_partition_t *partition_ptr = NULL;
    esp_partition_t partition;
    const esp_partition_t *next_partition = NULL;
    uint32_t recv_len                     = 0;
    uint8_t *data                         = NULL;
    size_t item_size                      = 0;
    uint32_t fw_length;
    uint8_t last_progress = 0;

    ESP_LOGI(TAG, "OTA task started");

    // Suspend UI update tasks to prevent display conflicts
#ifdef CONFIG_LORACUE_UI_COMPACT
    ESP_LOGI(TAG, "Suspending UI update tasks");
    ui_data_update_task_stop();
    ui_mini_show_ota_update();
#endif

#ifdef CONFIG_LORACUE_UI_RICH
    ui_rich_show_ota_update();
#endif

    notify_sem = xSemaphoreCreateCounting(100, 0);
    xSemaphoreGive(notify_sem);

    fw_length = esp_ble_ota_get_fw_length();
    ESP_LOGI(TAG, "Expected firmware size: %u bytes (%.1f MB)", fw_length, fw_length / 1048576.0f);

    // Get boot partition
    partition_ptr = (esp_partition_t *)esp_ota_get_boot_partition();
    if (partition_ptr == NULL || partition_ptr->type != ESP_PARTITION_TYPE_APP) {
        ESP_LOGE(TAG, "Invalid boot partition");
        goto OTA_ERROR;
    }

    // Determine next OTA partition
    if (partition_ptr->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) {
        partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
    } else {
        next_partition    = esp_ota_get_next_update_partition(partition_ptr);
        partition.subtype = next_partition ? next_partition->subtype : ESP_PARTITION_SUBTYPE_APP_OTA_0;
    }

    partition.type = ESP_PARTITION_TYPE_APP;
    partition_ptr  = (esp_partition_t *)esp_partition_find_first(partition.type, partition.subtype, NULL);
    if (partition_ptr == NULL) {
        ESP_LOGE(TAG, "OTA partition not found");
        goto OTA_ERROR;
    }

    memcpy(&partition, partition_ptr, sizeof(esp_partition_t));
    ESP_LOGI(TAG, "Target partition: %s (subtype %d)", partition_ptr->label, partition_ptr->subtype);

    // Begin OTA
    if (esp_ota_begin(&partition, OTA_SIZE_UNKNOWN, &out_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed");
        goto OTA_ERROR;
    }

    ESP_LOGI(TAG, "Receiving firmware...");

    // Stream firmware data directly to flash
    while (recv_len < fw_length) {
        data = (uint8_t *)xRingbufferReceive(s_ringbuf, &item_size, (TickType_t)portMAX_DELAY);
        xSemaphoreTake(notify_sem, portMAX_DELAY);

        if (item_size != 0) {
            if (recv_len + item_size > fw_length) {
                ESP_LOGE(TAG, "Received more data than expected");
                vRingbufferReturnItem(s_ringbuf, (void *)data);
                goto OTA_ERROR;
            }

            // Write directly to flash
            if (esp_ota_write(out_handle, data, item_size) != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_write failed");
                vRingbufferReturnItem(s_ringbuf, (void *)data);
                goto OTA_ERROR;
            }

            recv_len += item_size;
            vRingbufferReturnItem(s_ringbuf, (void *)data);

            // Update progress
            uint8_t progress = (recv_len * 100) / fw_length;
            if (progress != last_progress) {
                last_progress = progress;
#ifdef CONFIG_LORACUE_UI_COMPACT
                ui_mini_update_ota_progress(progress);
#endif
                ESP_LOGI(TAG, "Progress: %u%%", progress);
            }
        }
        xSemaphoreGive(notify_sem);
    }

    ESP_LOGI(TAG, "Firmware received successfully");

    if (esp_ota_end(out_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed");
        goto OTA_ERROR;
    }

    if (esp_ota_set_boot_partition(&partition) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed");
        goto OTA_ERROR;
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  OTA Update Successful!");
    ESP_LOGI(TAG, "  Rebooting in 2 seconds...");
    ESP_LOGI(TAG, "========================================");

    vSemaphoreDelete(notify_sem);
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

OTA_ERROR:
    ESP_LOGE(TAG, "========================================");
    ESP_LOGE(TAG, "  OTA Update Failed!");
    ESP_LOGE(TAG, "========================================");
    if (notify_sem)
        vSemaphoreDelete(notify_sem);

    s_ota_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t ble_ota_handler_init(void)
{
    if (!ble_ota_ringbuf_init(OTA_RINGBUF_SIZE)) {
        ESP_LOGE(TAG, "Ring buffer init failed");
        return ESP_FAIL;
    }

    // Register callback - task will be created when OTA transfer starts
    esp_ble_ota_recv_fw_data_callback(ota_recv_fw_cb);

    ESP_LOGI(TAG, "BLE OTA handler initialized (task starts on transfer)");

    return ESP_OK;
}

void ble_ota_handler_set_connection(uint16_t conn_handle)
{
    s_ota_conn_handle = conn_handle;
}
