#include "ble_ota.h"

#ifdef SIMULATOR_BUILD

esp_err_t ble_ota_service_init(esp_gatt_if_t gatts_if)
{
    (void)gatts_if;
    return ESP_ERR_NOT_SUPPORTED;
}
void ble_ota_handle_control_write(const uint8_t *data, uint16_t len)
{
    (void)data;
    (void)len;
}
void ble_ota_handle_data_write(const uint8_t *data, uint16_t len)
{
    (void)data;
    (void)len;
}
void ble_ota_send_response(uint8_t response, const char *message)
{
    (void)response;
    (void)message;
}
void ble_ota_update_progress(uint8_t percentage)
{
    (void)percentage;
}
void ble_ota_handle_disconnect(void) {}

#else

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "ota_engine.h"
#include <string.h>

static const char *TAG = "BLE_OTA";

#define OTA_TIMEOUT_MS 30000 // 30 seconds without data = timeout

// External handles set by bluetooth_config.c
extern uint16_t ota_control_handle;
extern uint16_t ota_progress_handle;
extern esp_gatt_if_t ota_gatts_if;

typedef enum { BLE_OTA_STATE_IDLE, BLE_OTA_STATE_ACTIVE, BLE_OTA_STATE_FINISHING } ble_ota_state_t;

static ble_ota_state_t ota_state       = BLE_OTA_STATE_IDLE;
static uint16_t ota_conn_id            = 0;
static size_t expected_size            = 0;
static uint8_t current_progress        = 0;
static TimerHandle_t ota_timeout_timer = NULL;

static void ota_timeout_callback(TimerHandle_t timer)
{
    (void)timer;
    ESP_LOGE(TAG, "OTA timeout - no data received for %d seconds", OTA_TIMEOUT_MS / 1000);

    if (ota_state == BLE_OTA_STATE_ACTIVE) {
        ota_engine_abort();
        ble_ota_send_response(OTA_RESP_ERROR, "Timeout");
        ota_state        = BLE_OTA_STATE_IDLE;
        ota_conn_id      = 0;
        expected_size    = 0;
        current_progress = 0;
    }
}

void ble_ota_send_response(uint8_t response, const char *message)
{
    if (!ota_gatts_if || !ota_control_handle || ota_conn_id == 0)
        return;

    uint8_t data[128];
    data[0] = response;

    if (message) {
        size_t msg_len = strlen(message);
        if (msg_len > 127)
            msg_len = 127;
        memcpy(data + 1, message, msg_len);
        esp_ble_gatts_send_indicate(ota_gatts_if, ota_conn_id, ota_control_handle, msg_len + 1, data, false);
    } else {
        esp_ble_gatts_send_indicate(ota_gatts_if, ota_conn_id, ota_control_handle, 1, data, false);
    }
}

void ble_ota_update_progress(uint8_t percentage)
{
    if (!ota_gatts_if || !ota_progress_handle || ota_conn_id == 0)
        return;

    current_progress = percentage;
    esp_ble_gatts_send_indicate(ota_gatts_if, ota_conn_id, ota_progress_handle, 1, &percentage, false);
}

void ble_ota_handle_disconnect(void)
{
    if (ota_state != BLE_OTA_STATE_IDLE) {
        ESP_LOGW(TAG, "BLE disconnected during OTA, aborting");
        ota_engine_abort();

        if (ota_timeout_timer) {
            xTimerStop(ota_timeout_timer, 0);
        }

        ota_state        = BLE_OTA_STATE_IDLE;
        expected_size    = 0;
        current_progress = 0;
    }

    // Always reset connection ID on disconnect
    ota_conn_id = 0;
}

void ble_ota_handle_control_write(const uint8_t *data, uint16_t len)
{
    // FIXME: BLE OTA implemented but not working
    // Full implementation exists but has bugs preventing successful firmware updates

    esp_err_t ret;

    if (len < 1) {
        ble_ota_send_response(OTA_RESP_ERROR, "Invalid command");
        return;
    }

    uint8_t cmd = data[0];

    switch (cmd) {
        case OTA_CMD_START:
            if (ota_state != BLE_OTA_STATE_IDLE) {
                ble_ota_send_response(OTA_RESP_ERROR, "OTA already in progress");
                return;
            }

            if (len < 5) {
                ble_ota_send_response(OTA_RESP_ERROR, "Missing size");
                return;
            }

            expected_size = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];

            if (expected_size == 0 || expected_size > 4 * 1024 * 1024) {
                ble_ota_send_response(OTA_RESP_ERROR, "Invalid size (max 4MB)");
                return;
            }

            ret = ota_engine_start(expected_size);
            if (ret != ESP_OK) {
                ble_ota_send_response(OTA_RESP_ERROR, "OTA start failed");
                ESP_LOGE(TAG, "OTA start failed: %s", esp_err_to_name(ret));
                return;
            }

            ota_state        = BLE_OTA_STATE_ACTIVE;
            current_progress = 0;

            // Start timeout timer
            if (!ota_timeout_timer) {
                ota_timeout_timer =
                    xTimerCreate("ota_timeout", pdMS_TO_TICKS(OTA_TIMEOUT_MS), pdFALSE, NULL, ota_timeout_callback);
            }
            if (ota_timeout_timer) {
                xTimerStart(ota_timeout_timer, 0);
            }

            ble_ota_send_response(OTA_RESP_READY, NULL);
            ESP_LOGI(TAG, "OTA started via BLE: %zu bytes", expected_size);
            break;

        case OTA_CMD_ABORT:
            if (ota_state != BLE_OTA_STATE_IDLE) {
                ota_engine_abort();

                if (ota_timeout_timer) {
                    xTimerStop(ota_timeout_timer, 0);
                }

                ota_state        = BLE_OTA_STATE_IDLE;
                expected_size    = 0;
                current_progress = 0;

                ble_ota_send_response(OTA_RESP_READY, "Aborted");
                ESP_LOGI(TAG, "OTA aborted");
            } else {
                ble_ota_send_response(OTA_RESP_ERROR, "No OTA in progress");
            }
            break;

        case OTA_CMD_FINISH:
            if (ota_state != BLE_OTA_STATE_ACTIVE) {
                ble_ota_send_response(OTA_RESP_ERROR, "No OTA in progress");
                return;
            }

            ota_state = BLE_OTA_STATE_FINISHING;

            if (ota_timeout_timer) {
                xTimerStop(ota_timeout_timer, 0);
            }

            ret = ota_engine_finish();
            if (ret == ESP_OK) {
                const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
                if (!update_partition) {
                    ble_ota_send_response(OTA_RESP_ERROR, "No update partition");
                    ESP_LOGE(TAG, "No update partition found");
                    ota_state   = BLE_OTA_STATE_IDLE;
                    ota_conn_id = 0;
                    return;
                }

                ESP_LOGI(TAG, "Setting boot partition: %s (0x%lx)", update_partition->label, update_partition->address);

                ret = esp_ota_set_boot_partition(update_partition);
                if (ret != ESP_OK) {
                    ble_ota_send_response(OTA_RESP_ERROR, "Failed to set boot partition");
                    ESP_LOGE(TAG, "Set boot partition failed: %s", esp_err_to_name(ret));
                    ota_state   = BLE_OTA_STATE_IDLE;
                    ota_conn_id = 0;
                    return;
                }

                ESP_LOGW(TAG, "Boot partition set. Device will boot from %s after restart", update_partition->label);
                ble_ota_send_response(OTA_RESP_COMPLETE, "Rebooting...");
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            } else {
                ble_ota_send_response(OTA_RESP_ERROR, "OTA validation failed");
                ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(ret));
            }

            ota_state        = BLE_OTA_STATE_IDLE;
            ota_conn_id      = 0;
            expected_size    = 0;
            current_progress = 0;
            break;

        default:
            ble_ota_send_response(OTA_RESP_ERROR, "Unknown command");
            break;
    }
}

void ble_ota_handle_data_write(const uint8_t *data, uint16_t len)
{
    if (ota_state != BLE_OTA_STATE_ACTIVE) {
        ESP_LOGW(TAG, "OTA not active, ignoring data");
        return;
    }

    // Reset timeout timer on data received
    if (ota_timeout_timer) {
        xTimerReset(ota_timeout_timer, 0);
    }

    esp_err_t ret = ota_engine_write(data, len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(ret));
        ota_engine_abort();
        ble_ota_send_response(OTA_RESP_ERROR, "Write failed");

        if (ota_timeout_timer) {
            xTimerStop(ota_timeout_timer, 0);
        }

        ota_state        = BLE_OTA_STATE_IDLE;
        ota_conn_id      = 0;
        expected_size    = 0;
        current_progress = 0;
        return;
    }

    uint8_t progress = (uint8_t)ota_engine_get_progress();
    if (progress != current_progress && progress % 5 == 0) {
        ble_ota_update_progress(progress);
    }
}

esp_err_t ble_ota_service_init(esp_gatt_if_t gatts_if)
{
    (void)gatts_if;
    ESP_LOGI(TAG, "BLE OTA service initialized");
    return ESP_OK;
}

void ble_ota_set_connection(uint16_t conn_id)
{
    ota_conn_id = conn_id;
}

#endif
