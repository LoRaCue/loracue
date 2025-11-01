/**
 * @file ble_ota.c
 * @brief BLE OTA (Over-The-Air) firmware update service - NimBLE implementation
 * 
 * Provides a custom GATT service for secure firmware updates over BLE.
 * Implements chunked data transfer with progress reporting and timeout handling.
 * 
 * @copyright Copyright (c) 2025 LoRaCue Project
 * @license GPL-3.0
 */

#include "ble_ota.h"

#ifdef SIMULATOR_BUILD
//==============================================================================
// SIMULATOR STUB IMPLEMENTATION
//==============================================================================

esp_err_t ble_ota_service_init(uint16_t conn_handle)
{
    (void)conn_handle;
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

void ble_ota_set_connection(uint16_t conn_handle)
{
    (void)conn_handle;
}

#else
//==============================================================================
// NIMBLE HARDWARE IMPLEMENTATION
//==============================================================================

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include "ota_engine.h"
#include <string.h>

static const char *TAG = "ble_ota";

//==============================================================================
// CONSTANTS
//==============================================================================

#define OTA_TIMEOUT_MS 30000  // 30 seconds without data = timeout

// OTA Service UUID: 49589A79-7CC5-465D-BFF1-FE37C5065000
static const ble_uuid128_t BLE_UUID_OTA_SERVICE = 
    BLE_UUID128_INIT(0x00, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
                     0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49);

// Control Characteristic: 49589A79-7CC5-465D-BFF1-FE37C5065001
static const ble_uuid128_t BLE_UUID_OTA_CONTROL = 
    BLE_UUID128_INIT(0x01, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
                     0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49);

// Data Characteristic: 49589A79-7CC5-465D-BFF1-FE37C5065002
static const ble_uuid128_t BLE_UUID_OTA_DATA = 
    BLE_UUID128_INIT(0x02, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
                     0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49);

// Progress Characteristic: 49589A79-7CC5-465D-BFF1-FE37C5065003
static const ble_uuid128_t BLE_UUID_OTA_PROGRESS = 
    BLE_UUID128_INIT(0x03, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
                     0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49);

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

typedef enum {
    BLE_OTA_STATE_IDLE,
    BLE_OTA_STATE_ACTIVE,
    BLE_OTA_STATE_FINISHING
} ble_ota_state_t;

//==============================================================================
// GLOBAL STATE
//==============================================================================

static ble_ota_state_t g_ota_state = BLE_OTA_STATE_IDLE;
static uint16_t g_ota_conn_handle = 0;
static size_t g_expected_size = 0;
static uint8_t g_current_progress = 0;
static TimerHandle_t g_ota_timeout_timer = NULL;

// Characteristic handles
static uint16_t g_ota_control_handle = 0;
static uint16_t g_ota_data_handle = 0;
static uint16_t g_ota_progress_handle = 0;


//==============================================================================
// FORWARD DECLARATIONS
//==============================================================================

static void ota_timeout_callback(TimerHandle_t timer);
static int ble_ota_control_access(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg);
static int ble_ota_data_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

//==============================================================================
// TIMEOUT HANDLER
//==============================================================================

/**
 * @brief OTA timeout callback - aborts OTA if no data received
 * @param timer Timer handle
 */
static void ota_timeout_callback(TimerHandle_t timer)
{
    (void)timer;
    
    ESP_LOGE(TAG, "OTA timeout - no data received for %d seconds", OTA_TIMEOUT_MS / 1000);
    
    if (g_ota_state == BLE_OTA_STATE_ACTIVE) {
        ota_engine_abort();
        ble_ota_send_response(OTA_RESP_ERROR, "Timeout");
        
        g_ota_state = BLE_OTA_STATE_IDLE;
        g_ota_conn_handle = 0;
        g_expected_size = 0;
        g_current_progress = 0;
    }
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

/**
 * @brief Send response over OTA control characteristic
 * @param response Response code
 * @param message Optional message string
 */
void ble_ota_send_response(uint8_t response, const char *message)
{
    if (g_ota_conn_handle == 0 || g_ota_control_handle == 0) {
        ESP_LOGW(TAG, "Cannot send response - not connected");
        return;
    }
    
    uint8_t data[128];
    data[0] = response;
    size_t len = 1;
    
    if (message) {
        size_t msg_len = strlen(message);
        if (msg_len > 127) {
            msg_len = 127;
        }
        memcpy(data + 1, message, msg_len);
        len += msg_len;
    }
    
    // Prepare notification
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (!om) {
        ESP_LOGE(TAG, "Failed to allocate mbuf for OTA response");
        return;
    }
    
    int rc = ble_gattc_indicate_custom(g_ota_conn_handle, g_ota_control_handle, om);
    if (rc != 0) {
        ESP_LOGW(TAG, "Failed to send OTA response: rc=%d", rc);
        os_mbuf_free_chain(om);
    }
}

/**
 * @brief Update OTA progress
 * @param percentage Progress percentage (0-100)
 */
void ble_ota_update_progress(uint8_t percentage)
{
    if (g_ota_conn_handle == 0 || g_ota_progress_handle == 0) {
        return;
    }
    
    g_current_progress = percentage;
    
    struct os_mbuf *om = ble_hs_mbuf_from_flat(&percentage, 1);
    if (!om) {
        return;
    }
    
    int rc = ble_gattc_notify_custom(g_ota_conn_handle, g_ota_progress_handle, om);
    if (rc != 0) {
        os_mbuf_free_chain(om);
    }
}

/**
 * @brief Handle BLE disconnection during OTA
 */
void ble_ota_handle_disconnect(void)
{
    if (g_ota_state != BLE_OTA_STATE_IDLE) {
        ESP_LOGW(TAG, "BLE disconnected during OTA, aborting");
        ota_engine_abort();
        
        if (g_ota_timeout_timer) {
            xTimerStop(g_ota_timeout_timer, 0);
        }
        
        g_ota_state = BLE_OTA_STATE_IDLE;
        g_expected_size = 0;
        g_current_progress = 0;
    }
    
    g_ota_conn_handle = 0;
}

/**
 * @brief Set OTA connection handle
 * @param conn_handle Connection handle
 */
void ble_ota_set_connection(uint16_t conn_handle)
{
    g_ota_conn_handle = conn_handle;
    ESP_LOGI(TAG, "OTA connection handle set: %d", conn_handle);
}

/**
 * @brief Handle OTA control characteristic write
 * @param data Command data
 * @param len Data length
 */
void ble_ota_handle_control_write(const uint8_t *data, uint16_t len)
{
    // FIXME: BLE OTA implemented but not fully tested
    // Full implementation exists but may have bugs preventing successful firmware updates
    
    if (len < 1) {
        ble_ota_send_response(OTA_RESP_ERROR, "Invalid command");
        return;
    }
    
    uint8_t cmd = data[0];
    esp_err_t ret;
    
    switch (cmd) {
        case OTA_CMD_START:
            if (g_ota_state != BLE_OTA_STATE_IDLE) {
                ble_ota_send_response(OTA_RESP_ERROR, "OTA already in progress");
                return;
            }
            
            if (len < 5) {
                ble_ota_send_response(OTA_RESP_ERROR, "Missing size");
                return;
            }
            
            g_expected_size = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
            
            if (g_expected_size == 0 || g_expected_size > 4 * 1024 * 1024) {
                ble_ota_send_response(OTA_RESP_ERROR, "Invalid size (max 4MB)");
                return;
            }
            
            ret = ota_engine_start(g_expected_size);
            if (ret != ESP_OK) {
                ble_ota_send_response(OTA_RESP_ERROR, "OTA start failed");
                ESP_LOGE(TAG, "OTA start failed: %s", esp_err_to_name(ret));
                return;
            }
            
            g_ota_state = BLE_OTA_STATE_ACTIVE;
            g_current_progress = 0;
            
            // Start timeout timer
            if (!g_ota_timeout_timer) {
                g_ota_timeout_timer = xTimerCreate("ota_timeout", pdMS_TO_TICKS(OTA_TIMEOUT_MS),
                                                   pdFALSE, NULL, ota_timeout_callback);
            }
            if (g_ota_timeout_timer) {
                xTimerStart(g_ota_timeout_timer, 0);
            }
            
            ble_ota_send_response(OTA_RESP_READY, NULL);
            ESP_LOGI(TAG, "OTA started via BLE: %zu bytes", g_expected_size);
            break;
            
        case OTA_CMD_ABORT:
            if (g_ota_state != BLE_OTA_STATE_IDLE) {
                ota_engine_abort();
                
                if (g_ota_timeout_timer) {
                    xTimerStop(g_ota_timeout_timer, 0);
                }
                
                g_ota_state = BLE_OTA_STATE_IDLE;
                g_expected_size = 0;
                g_current_progress = 0;
                
                ble_ota_send_response(OTA_RESP_READY, "Aborted");
                ESP_LOGI(TAG, "OTA aborted");
            } else {
                ble_ota_send_response(OTA_RESP_ERROR, "No OTA in progress");
            }
            break;
            
        case OTA_CMD_FINISH:
            if (g_ota_state != BLE_OTA_STATE_ACTIVE) {
                ble_ota_send_response(OTA_RESP_ERROR, "No OTA in progress");
                return;
            }
            
            g_ota_state = BLE_OTA_STATE_FINISHING;
            
            if (g_ota_timeout_timer) {
                xTimerStop(g_ota_timeout_timer, 0);
            }
            
            ret = ota_engine_finish();
            if (ret == ESP_OK) {
                const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
                if (!update_partition) {
                    ble_ota_send_response(OTA_RESP_ERROR, "No update partition");
                    ESP_LOGE(TAG, "No update partition found");
                    g_ota_state = BLE_OTA_STATE_IDLE;
                    g_ota_conn_handle = 0;
                    return;
                }
                
                ESP_LOGI(TAG, "Setting boot partition: %s (0x%lx)",
                         update_partition->label, update_partition->address);
                
                ret = esp_ota_set_boot_partition(update_partition);
                if (ret != ESP_OK) {
                    ble_ota_send_response(OTA_RESP_ERROR, "Failed to set boot partition");
                    ESP_LOGE(TAG, "Set boot partition failed: %s", esp_err_to_name(ret));
                    g_ota_state = BLE_OTA_STATE_IDLE;
                    g_ota_conn_handle = 0;
                    return;
                }
                
                ESP_LOGW(TAG, "Boot partition set. Device will boot from %s after restart",
                         update_partition->label);
                ble_ota_send_response(OTA_RESP_COMPLETE, "Rebooting...");
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            } else {
                ble_ota_send_response(OTA_RESP_ERROR, "OTA validation failed");
                ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(ret));
            }
            
            g_ota_state = BLE_OTA_STATE_IDLE;
            g_ota_conn_handle = 0;
            g_expected_size = 0;
            g_current_progress = 0;
            break;
            
        default:
            ble_ota_send_response(OTA_RESP_ERROR, "Unknown command");
            break;
    }
}

/**
 * @brief Handle OTA data characteristic write
 * @param data Firmware data chunk
 * @param len Data length
 */
void ble_ota_handle_data_write(const uint8_t *data, uint16_t len)
{
    if (g_ota_state != BLE_OTA_STATE_ACTIVE) {
        ESP_LOGW(TAG, "OTA not active, ignoring data");
        return;
    }
    
    // Reset timeout timer on data received
    if (g_ota_timeout_timer) {
        xTimerReset(g_ota_timeout_timer, 0);
    }
    
    esp_err_t ret = ota_engine_write(data, len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(ret));
        ota_engine_abort();
        ble_ota_send_response(OTA_RESP_ERROR, "Write failed");
        
        if (g_ota_timeout_timer) {
            xTimerStop(g_ota_timeout_timer, 0);
        }
        
        g_ota_state = BLE_OTA_STATE_IDLE;
        g_ota_conn_handle = 0;
        g_expected_size = 0;
        g_current_progress = 0;
        return;
    }
    
    uint8_t progress = (uint8_t)ota_engine_get_progress();
    if (progress != g_current_progress && progress % 5 == 0) {
        ble_ota_update_progress(progress);
    }
}


//==============================================================================
// GATT SERVICE IMPLEMENTATION
//==============================================================================

/**
 * @brief OTA control characteristic access callback
 */
static int ble_ota_control_access(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
        uint8_t temp_buf[256];
        
        if (om_len > sizeof(temp_buf)) {
            ESP_LOGW(TAG, "OTA control write too large: %d bytes", om_len);
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        
        int rc = ble_hs_mbuf_to_flat(ctxt->om, temp_buf, sizeof(temp_buf), NULL);
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to copy mbuf data: rc=%d", rc);
            return BLE_ATT_ERR_UNLIKELY;
        }
        
        ble_ota_handle_control_write(temp_buf, om_len);
        return 0;
    }
    
    return BLE_ATT_ERR_UNLIKELY;
}

/**
 * @brief OTA data characteristic access callback
 */
static int ble_ota_data_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
        uint8_t temp_buf[512];
        
        if (om_len > sizeof(temp_buf)) {
            ESP_LOGW(TAG, "OTA data write too large: %d bytes", om_len);
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        
        int rc = ble_hs_mbuf_to_flat(ctxt->om, temp_buf, sizeof(temp_buf), NULL);
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to copy mbuf data: rc=%d", rc);
            return BLE_ATT_ERR_UNLIKELY;
        }
        
        ble_ota_handle_data_write(temp_buf, om_len);
        return 0;
    }
    
    return BLE_ATT_ERR_UNLIKELY;
}

/**
 * @brief OTA GATT service definition
 */
static const struct ble_gatt_svc_def ota_gatt_service[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &BLE_UUID_OTA_SERVICE.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                // Control Characteristic (Write + Indicate)
                .uuid = &BLE_UUID_OTA_CONTROL.u,
                .access_cb = ble_ota_control_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_INDICATE,
                .val_handle = &g_ota_control_handle,
            },
            {
                // Data Characteristic (Write without response)
                .uuid = &BLE_UUID_OTA_DATA.u,
                .access_cb = ble_ota_data_access,
                .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
                .val_handle = &g_ota_data_handle,
            },
            {
                // Progress Characteristic (Read + Notify)
                .uuid = &BLE_UUID_OTA_PROGRESS.u,
                .access_cb = NULL,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &g_ota_progress_handle,
            },
            {
                0,  // End of characteristics
            }
        },
    },
    {
        0,  // End of services
    },
};

/**
 * @brief Initialize OTA service
 * @param conn_handle Initial connection handle (0 if not connected)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_ota_service_init(uint16_t conn_handle)
{
    ESP_LOGI(TAG, "Initializing BLE OTA service...");
    
    g_ota_conn_handle = conn_handle;
    
    // Register OTA GATT service
    int rc = ble_gatts_count_cfg(ota_gatt_service);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count OTA GATT service: rc=%d", rc);
        return ESP_FAIL;
    }
    
    rc = ble_gatts_add_svcs(ota_gatt_service);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add OTA GATT service: rc=%d", rc);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "BLE OTA service initialized successfully");
    ESP_LOGI(TAG, "  Service UUID: 49589A79-7CC5-465D-BFF1-FE37C5065000");
    ESP_LOGI(TAG, "  Control handle: %d", g_ota_control_handle);
    ESP_LOGI(TAG, "  Data handle: %d", g_ota_data_handle);
    ESP_LOGI(TAG, "  Progress handle: %d", g_ota_progress_handle);
    
    return ESP_OK;
}

#endif // SIMULATOR_BUILD
