/**
 * @file bluetooth_config.c
 * @brief Enterprise-grade Bluetooth LE configuration service using NimBLE stack
 *
 * This module provides a comprehensive BLE interface for device configuration,
 * firmware updates, and command execution. It implements:
 * - Nordic UART Service (NUS) for command/response communication
 * - Device Information Service (DIS) for device metadata
 * - Custom OTA Service for secure firmware updates
 * - BLE 5.0 with 2M PHY for faster OTA transfers
 * - LE Secure Connections (BLE 4.2+) with passkey pairing
 *
 * Architecture:
 * - Thread-safe operation with FreeRTOS primitives
 * - Queue-based command processing for non-blocking operation
 * - Comprehensive error handling and recovery
 * - Extensive logging for debugging and monitoring
 *
 * @copyright Copyright (c) 2025 LoRaCue Project
 * @license GPL-3.0
 */

#include "bluetooth_config.h"
#include "bsp.h"
#include "esp_log.h"
#include "esp_random.h"
#include "general_config.h"
#include "version.h"
#include <string.h>

//==============================================================================
// NIMBLE HARDWARE IMPLEMENTATION
//==============================================================================

// NimBLE Stack Headers
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

// Application Headers
#include "ble_ota.h"
#include "ble_ota_handler.h"
#include "commands.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"

//==============================================================================
// CONSTANTS AND CONFIGURATION
//==============================================================================

static const char *TAG = "bluetooth_config";

// Nordic UART Service (NUS) UUIDs - 128-bit custom UUIDs
// Base: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
static const ble_uuid128_t BLE_UUID_NUS_SERVICE = 
    BLE_UUID128_INIT(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                     0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E);

static const ble_uuid128_t BLE_UUID_NUS_TX_CHAR = 
    BLE_UUID128_INIT(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                     0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E);

static const ble_uuid128_t BLE_UUID_NUS_RX_CHAR = 
    BLE_UUID128_INIT(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                     0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E);

// Device Information Service (DIS) - Standard 16-bit UUIDs
#define BLE_UUID_DIS_SERVICE                0x180A
#define BLE_UUID_DIS_MANUFACTURER_NAME      0x2A29
#define BLE_UUID_DIS_MODEL_NUMBER           0x2A24
#define BLE_UUID_DIS_FIRMWARE_REVISION      0x2A26
#define BLE_UUID_DIS_HARDWARE_REVISION      0x2A27

// BLE Configuration
#define BLE_DEVICE_NAME_PREFIX              "LoRaCue"
#define BLE_DEVICE_NAME_MAX_LEN             31  // BLE name limit
#define BLE_ADV_INTERVAL_MIN                0x0020  // 20ms
#define BLE_ADV_INTERVAL_MAX                0x0040  // 40ms
#define BLE_CONN_INTERVAL_MIN               0x0006  // 7.5ms
#define BLE_CONN_INTERVAL_MAX               0x0010  // 20ms
#define BLE_CONN_LATENCY                    0
#define BLE_CONN_SUPERVISION_TIMEOUT        0x0100  // 1.6s
#define BLE_MTU_SIZE                        512     // Preferred MTU

// UART Service Configuration
#define BLE_UART_RX_QUEUE_SIZE              10
#define BLE_UART_TASK_STACK_SIZE            4096
#define BLE_UART_TASK_PRIORITY              5
#define BLE_UART_CMD_MAX_LENGTH             2048
#define BLE_UART_RX_BUFFER_SIZE             2048

// Security Configuration
#define BLE_SM_IO_CAP                       BLE_HS_IO_DISPLAY_ONLY
#define BLE_SM_BONDING                      1
#define BLE_SM_MITM                         1
#define BLE_SM_SC                           1  // Secure Connections
#define BLE_SM_KEYPRESS                     0
#define BLE_SM_OOB_FLAG                     0

// Timeouts and Retries
#define BLE_STATE_MUTEX_TIMEOUT_MS          100
#define BLE_NOTIFICATION_TIMEOUT_MS         1000
#define BLE_MAX_NOTIFICATION_RETRIES        3

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief BLE UART message structure for queue-based command processing
 */
typedef struct {
    char data[BLE_UART_CMD_MAX_LENGTH];  ///< Command data
    size_t len;                           ///< Command length
} ble_uart_msg_t;

/**
 * @brief BLE connection state structure
 */
typedef struct {
    bool connected;                       ///< Connection active
    bool notifications_enabled;           ///< TX notifications enabled
    uint16_t conn_handle;                 ///< Connection handle
    uint16_t mtu;                         ///< Negotiated MTU size
    uint8_t addr[6];                      ///< Peer address
    uint8_t addr_type;                    ///< Peer address type
} ble_conn_state_t;

/**
 * @brief BLE pairing state structure
 */
typedef struct {
    bool in_progress;                     ///< Pairing active
    uint32_t passkey;                     ///< 6-digit passkey
    uint16_t conn_handle;                 ///< Connection being paired
} ble_pairing_state_t;

//==============================================================================
// GLOBAL STATE
//==============================================================================

// Module state
static bool s_ble_initialized = false;
static bool s_ble_enabled = false;

// Connection state
static ble_conn_state_t s_conn_state = {0};
static SemaphoreHandle_t s_conn_state_mutex = NULL;

// Pairing state
static ble_pairing_state_t s_pairing_state = {0};
static SemaphoreHandle_t s_pairing_state_mutex = NULL;

// UART service state
static QueueHandle_t s_uart_rx_queue = NULL;
static TaskHandle_t s_uart_task_handle = NULL;
static char s_uart_rx_buffer[BLE_UART_RX_BUFFER_SIZE];
static size_t s_uart_rx_buffer_len = 0;

// Service handles
static uint16_t s_nus_tx_handle = 0;
static uint16_t s_nus_rx_handle = 0;

//==============================================================================
// FORWARD DECLARATIONS
//==============================================================================

// Core BLE callbacks
static int ble_gap_event_handler(struct ble_gap_event *event, void *arg);
static int ble_gatt_access_handler(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg);

// Service initialization
static esp_err_t ble_services_init(void);
static esp_err_t ble_advertising_start(void);

// UART service
static void ble_uart_task(void *arg);
static void ble_uart_send_response(const char *response);
static esp_err_t ble_uart_send_notification(const uint8_t *data, size_t len);

// Utility functions
static void ble_print_addr(const uint8_t *addr);
static const char *ble_gap_event_name(int event_type);
static const char *ble_sm_io_cap_name(uint8_t io_cap);

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Print BLE address in human-readable format
 * @param addr 6-byte BLE address
 */
static void ble_print_addr(const uint8_t *addr)
{
    if (!addr) {
        ESP_LOGW(TAG, "NULL address");
        return;
    }
    ESP_LOGI(TAG, "Address: %02X:%02X:%02X:%02X:%02X:%02X",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

/**
 * @brief Get human-readable name for GAP event type
 * @param event_type GAP event type
 * @return Event name string
 */
static const char *ble_gap_event_name(int event_type)
{
    switch (event_type) {
        case BLE_GAP_EVENT_CONNECT:             return "CONNECT";
        case BLE_GAP_EVENT_DISCONNECT:          return "DISCONNECT";
        case BLE_GAP_EVENT_CONN_UPDATE:         return "CONN_UPDATE";
        case BLE_GAP_EVENT_CONN_UPDATE_REQ:     return "CONN_UPDATE_REQ";
        case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:    return "L2CAP_UPDATE_REQ";
        case BLE_GAP_EVENT_TERM_FAILURE:        return "TERM_FAILURE";
        case BLE_GAP_EVENT_DISC:                return "DISC";
        case BLE_GAP_EVENT_DISC_COMPLETE:       return "DISC_COMPLETE";
        case BLE_GAP_EVENT_ADV_COMPLETE:        return "ADV_COMPLETE";
        case BLE_GAP_EVENT_ENC_CHANGE:          return "ENC_CHANGE";
        case BLE_GAP_EVENT_PASSKEY_ACTION:      return "PASSKEY_ACTION";
        case BLE_GAP_EVENT_NOTIFY_RX:           return "NOTIFY_RX";
        case BLE_GAP_EVENT_NOTIFY_TX:           return "NOTIFY_TX";
        case BLE_GAP_EVENT_SUBSCRIBE:           return "SUBSCRIBE";
        case BLE_GAP_EVENT_MTU:                 return "MTU";
        case BLE_GAP_EVENT_IDENTITY_RESOLVED:   return "IDENTITY_RESOLVED";
        case BLE_GAP_EVENT_REPEAT_PAIRING:      return "REPEAT_PAIRING";
        case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE: return "PHY_UPDATE_COMPLETE";
        default:                                 return "UNKNOWN";
    }
}

/**
 * @brief Get human-readable name for SM I/O capability
 * @param io_cap I/O capability value
 * @return Capability name string
 */
static const char *ble_sm_io_cap_name(uint8_t io_cap)
{
    switch (io_cap) {
        case BLE_HS_IO_DISPLAY_ONLY:      return "DISPLAY_ONLY";
        case BLE_HS_IO_DISPLAY_YESNO:     return "DISPLAY_YESNO";
        case BLE_HS_IO_KEYBOARD_ONLY:     return "KEYBOARD_ONLY";
        case BLE_HS_IO_NO_INPUT_OUTPUT:   return "NO_INPUT_OUTPUT";
        case BLE_HS_IO_KEYBOARD_DISPLAY:  return "KEYBOARD_DISPLAY";
        default:                          return "UNKNOWN";
    }
}

/**
 * @brief Safely acquire connection state mutex
 * @return true if mutex acquired, false on timeout
 */
static bool ble_conn_state_lock(void)
{
    if (!s_conn_state_mutex) {
        ESP_LOGE(TAG, "Connection state mutex not initialized");
        return false;
    }
    
    if (xSemaphoreTake(s_conn_state_mutex, pdMS_TO_TICKS(BLE_STATE_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire connection state mutex (timeout)");
        return false;
    }
    
    return true;
}

/**
 * @brief Release connection state mutex
 */
static void ble_conn_state_unlock(void)
{
    if (s_conn_state_mutex) {
        xSemaphoreGive(s_conn_state_mutex);
    }
}

/**
 * @brief Safely acquire pairing state mutex
 * @return true if mutex acquired, false on timeout
 */
static bool ble_pairing_state_lock(void)
{
    if (!s_pairing_state_mutex) {
        ESP_LOGE(TAG, "Pairing state mutex not initialized");
        return false;
    }
    
    if (xSemaphoreTake(s_pairing_state_mutex, pdMS_TO_TICKS(BLE_STATE_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire pairing state mutex (timeout)");
        return false;
    }
    
    return true;
}

/**
 * @brief Release pairing state mutex
 */
static void ble_pairing_state_unlock(void)
{
    if (s_pairing_state_mutex) {
        xSemaphoreGive(s_pairing_state_mutex);
    }
}


//==============================================================================
// UART SERVICE IMPLEMENTATION
//==============================================================================

/**
 * @brief UART RX task - processes commands from BLE queue
 * 
 * This task runs continuously, waiting for commands from the BLE UART RX queue.
 * Commands are executed via the commands module, with responses sent back over BLE.
 * 
 * @param arg Unused task parameter
 */
static void ble_uart_task(void *arg)
{
    (void)arg;
    
    ESP_LOGI(TAG, "BLE UART task started (stack=%d, priority=%d)",
             BLE_UART_TASK_STACK_SIZE, BLE_UART_TASK_PRIORITY);
    
    ble_uart_msg_t msg;
    
    while (1) {
        // Block waiting for commands
        if (xQueueReceive(s_uart_rx_queue, &msg, portMAX_DELAY) == pdTRUE) {
            // Null-terminate for safety
            msg.data[msg.len] = '\0';
            
            ESP_LOGD(TAG, "Processing BLE command: '%s' (len=%zu)", msg.data, msg.len);
            
            // Execute command with BLE response callback
            commands_execute(msg.data, ble_uart_send_response);
        }
    }
}

/**
 * @brief Send response string over BLE UART TX characteristic
 * 
 * This function is called by the commands module to send responses back to the
 * BLE client. It automatically appends a newline and handles MTU fragmentation.
 * 
 * @param response Response string (null-terminated)
 */
static void ble_uart_send_response(const char *response)
{
    if (!response) {
        ESP_LOGW(TAG, "Attempted to send NULL response");
        return;
    }
    
    size_t resp_len = strlen(response);
    if (resp_len == 0) {
        ESP_LOGD(TAG, "Empty response, skipping");
        return;
    }
    
    // Check connection state
    if (!ble_conn_state_lock()) {
        ESP_LOGW(TAG, "Cannot send response - failed to acquire state lock");
        return;
    }
    
    bool can_send = s_conn_state.connected && s_conn_state.notifications_enabled;
    uint16_t mtu = s_conn_state.mtu;
    
    ble_conn_state_unlock();
    
    if (!can_send) {
        ESP_LOGD(TAG, "Cannot send response - not ready (connected=%d, notif=%d)",
                 s_conn_state.connected, s_conn_state.notifications_enabled);
        return;
    }
    
    // Allocate buffer for response + newline
    size_t total_len = resp_len + 1;  // +1 for '\n'
    size_t max_payload = mtu - 3;     // -3 for ATT header
    
    if (total_len > max_payload) {
        ESP_LOGW(TAG, "Response too large (%zu bytes), truncating to %zu bytes",
                 total_len, max_payload);
        total_len = max_payload;
        resp_len = total_len - 1;
    }
    
    uint8_t *buffer = malloc(total_len);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes for response", total_len);
        return;
    }
    
    memcpy(buffer, response, resp_len);
    buffer[resp_len] = '\n';
    
    esp_err_t ret = ble_uart_send_notification(buffer, total_len);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send response notification: %s", esp_err_to_name(ret));
    }
    
    free(buffer);
}

/**
 * @brief Send notification over BLE UART TX characteristic
 * 
 * Low-level function to send data via BLE notification. Handles retries and
 * error recovery.
 * 
 * @param data Data buffer to send
 * @param len Data length
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t ble_uart_send_notification(const uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!ble_conn_state_lock()) {
        return ESP_ERR_TIMEOUT;
    }
    
    uint16_t conn_handle = s_conn_state.conn_handle;
    bool connected = s_conn_state.connected;
    
    ble_conn_state_unlock();
    
    if (!connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Prepare notification
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (!om) {
        ESP_LOGE(TAG, "Failed to allocate mbuf for notification");
        return ESP_ERR_NO_MEM;
    }
    
    // Send notification with retries
    int rc;
    for (int retry = 0; retry < BLE_MAX_NOTIFICATION_RETRIES; retry++) {
        rc = ble_gattc_notify_custom(conn_handle, s_nus_tx_handle, om);
        
        if (rc == 0) {
            ESP_LOGD(TAG, "Notification sent successfully (len=%zu)", len);
            return ESP_OK;
        }
        
        if (rc == BLE_HS_ENOTCONN) {
            ESP_LOGW(TAG, "Connection lost during notification");
            os_mbuf_free_chain(om);
            return ESP_ERR_INVALID_STATE;
        }
        
        if (rc == BLE_HS_ENOMEM) {
            ESP_LOGW(TAG, "Out of memory for notification, retry %d/%d",
                     retry + 1, BLE_MAX_NOTIFICATION_RETRIES);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        
        ESP_LOGW(TAG, "Notification failed: rc=%d, retry %d/%d",
                 rc, retry + 1, BLE_MAX_NOTIFICATION_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    os_mbuf_free_chain(om);
    ESP_LOGE(TAG, "Notification failed after %d retries: rc=%d",
             BLE_MAX_NOTIFICATION_RETRIES, rc);
    return ESP_FAIL;
}


//==============================================================================
// GATT SERVICE DEFINITIONS
//==============================================================================

/**
 * @brief GATT characteristic access callback for NUS RX characteristic
 * 
 * Handles write operations to the RX characteristic (data from client to device).
 * Implements command buffering and newline-delimited command parsing.
 * 
 * @param conn_handle Connection handle
 * @param attr_handle Attribute handle
 * @param ctxt GATT access context
 * @param arg User argument (unused)
 * @return 0 on success, BLE_ATT_ERR_* on error
 */
static int ble_nus_rx_access(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        ESP_LOGW(TAG, "Unexpected GATT operation on RX characteristic: %d", ctxt->op);
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    // Get write data
    uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
    if (om_len == 0) {
        return 0;  // Empty write, ignore
    }
    
    ESP_LOGD(TAG, "RX characteristic write: len=%d", om_len);
    
    // Copy data from mbuf chain
    uint8_t temp_buf[BLE_UART_CMD_MAX_LENGTH];
    int rc = ble_hs_mbuf_to_flat(ctxt->om, temp_buf, sizeof(temp_buf), NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to copy mbuf data: rc=%d", rc);
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    // Check for OTA commands - reject and direct to OTA service
    if (om_len >= 17 && strncmp((char *)temp_buf, "FIRMWARE_UPGRADE ", 17) == 0) {
        const char *error_msg = "ERROR Use dedicated OTA GATT service (UUID "
                                "49589A79-7CC5-465D-BFF1-FE37C5065000) for firmware upgrades\n";
        ble_uart_send_notification((const uint8_t *)error_msg, strlen(error_msg));
        return 0;
    }
    
    // Process received data byte by byte, looking for newlines
    for (uint16_t i = 0; i < om_len; i++) {
        char c = temp_buf[i];
        
        if (c == '\n' || c == '\r') {
            // Command complete
            if (s_uart_rx_buffer_len > 0) {
                ble_uart_msg_t msg;
                msg.len = s_uart_rx_buffer_len;
                memcpy(msg.data, s_uart_rx_buffer, s_uart_rx_buffer_len);
                
                if (xQueueSend(s_uart_rx_queue, &msg, 0) != pdTRUE) {
                    ESP_LOGW(TAG, "BLE UART queue full, dropping command");
                }
                
                s_uart_rx_buffer_len = 0;
            }
        } else if (s_uart_rx_buffer_len < sizeof(s_uart_rx_buffer) - 1) {
            s_uart_rx_buffer[s_uart_rx_buffer_len++] = c;
        } else {
            ESP_LOGW(TAG, "Command buffer overflow, dropping data");
            s_uart_rx_buffer_len = 0;
        }
    }
    
    return 0;
}

/**
 * @brief GATT characteristic access callback for DIS characteristics
 * 
 * Handles read operations for Device Information Service characteristics.
 * 
 * @param conn_handle Connection handle
 * @param attr_handle Attribute handle
 * @param ctxt GATT access context
 * @param arg User argument (characteristic type)
 * @return 0 on success, BLE_ATT_ERR_* on error
 */
static int ble_dis_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    
    uint16_t uuid16 = (uint16_t)(uintptr_t)arg;
    
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        ESP_LOGW(TAG, "Unexpected GATT operation on DIS characteristic: %d", ctxt->op);
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    const char *value = NULL;
    char hw_name_buf[32];
    
    switch (uuid16) {
        case BLE_UUID_DIS_MANUFACTURER_NAME:
            value = "LoRaCue";
            break;
            
        case BLE_UUID_DIS_MODEL_NUMBER: {
            const bsp_usb_config_t *usb_config = bsp_get_usb_config();
            value = usb_config->usb_product;
            break;
        }
        
        case BLE_UUID_DIS_FIRMWARE_REVISION:
            value = LORACUE_VERSION_STRING;
            break;
            
        case BLE_UUID_DIS_HARDWARE_REVISION: {
            const char *board_id = bsp_get_board_id();
            if (strcmp(board_id, "heltec_v3") == 0) {
                value = "Heltec LoRa V3";
            } else if (strcmp(board_id, "wokwi") == 0) {
                value = "Wokwi Simulator";
            } else {
                snprintf(hw_name_buf, sizeof(hw_name_buf), "%.31s", board_id);
                value = hw_name_buf;
            }
            break;
        }
        
        default:
            ESP_LOGW(TAG, "Unknown DIS characteristic UUID: 0x%04X", uuid16);
            return BLE_ATT_ERR_UNLIKELY;
    }
    
    if (value) {
        int rc = os_mbuf_append(ctxt->om, value, strlen(value));
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to append DIS value: rc=%d", rc);
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    }
    
    return 0;
}

/**
 * @brief GATT service definitions
 * 
 * Defines all GATT services and characteristics for the device:
 * - Nordic UART Service (NUS) for command/response
 * - Device Information Service (DIS) for device metadata
 * - OTA Service (provided by espressif/ble_ota component)
 */
static const struct ble_gatt_svc_def gatt_services[] = {
    {
        // Nordic UART Service (NUS)
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &BLE_UUID_NUS_SERVICE.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                // TX Characteristic (device to client, notify)
                .uuid = &BLE_UUID_NUS_TX_CHAR.u,
                .access_cb = NULL,  // Notify-only, no access callback needed
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_nus_tx_handle,
            },
            {
                // RX Characteristic (client to device, write)
                .uuid = &BLE_UUID_NUS_RX_CHAR.u,
                .access_cb = ble_nus_rx_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .val_handle = &s_nus_rx_handle,
            },
            {
                0,  // End of characteristics
            }
        },
    },
    {
        // Device Information Service (DIS)
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_UUID_DIS_SERVICE),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                // Manufacturer Name
                .uuid = BLE_UUID16_DECLARE(BLE_UUID_DIS_MANUFACTURER_NAME),
                .access_cb = ble_dis_access,
                .arg = (void *)(uintptr_t)BLE_UUID_DIS_MANUFACTURER_NAME,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                // Model Number
                .uuid = BLE_UUID16_DECLARE(BLE_UUID_DIS_MODEL_NUMBER),
                .access_cb = ble_dis_access,
                .arg = (void *)(uintptr_t)BLE_UUID_DIS_MODEL_NUMBER,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                // Firmware Revision
                .uuid = BLE_UUID16_DECLARE(BLE_UUID_DIS_FIRMWARE_REVISION),
                .access_cb = ble_dis_access,
                .arg = (void *)(uintptr_t)BLE_UUID_DIS_FIRMWARE_REVISION,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                // Hardware Revision
                .uuid = BLE_UUID16_DECLARE(BLE_UUID_DIS_HARDWARE_REVISION),
                .access_cb = ble_dis_access,
                .arg = (void *)(uintptr_t)BLE_UUID_DIS_HARDWARE_REVISION,
                .flags = BLE_GATT_CHR_F_READ,
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


//==============================================================================
// GAP EVENT HANDLER
//==============================================================================

/**
 * @brief GAP event handler - handles all BLE connection and security events
 * 
 * This is the central event handler for all GAP (Generic Access Profile) events.
 * It manages connection lifecycle, security/pairing, MTU negotiation, and more.
 * 
 * @param event GAP event structure
 * @param arg User argument (unused)
 * @return 0 on success, non-zero on error
 */
static int ble_gap_event_handler(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    
    ESP_LOGD(TAG, "GAP event: %s (%d)", ble_gap_event_name(event->type), event->type);
    
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "=== BLE Connection Event ===");
            
            if (event->connect.status == 0) {
                // Connection successful
                ESP_LOGI(TAG, "Connection established successfully");
                ESP_LOGI(TAG, "  Handle: %d", event->connect.conn_handle);
                
                struct ble_gap_conn_desc desc;
                if (ble_gap_conn_find(event->connect.conn_handle, &desc) == 0) {
                    ESP_LOGI(TAG, "  Peer address type: %d", desc.peer_id_addr.type);
                    ESP_LOG_BUFFER_HEX_LEVEL(TAG, desc.peer_id_addr.val, 6, ESP_LOG_INFO);
                    
                    if (ble_conn_state_lock()) {
                        s_conn_state.connected = true;
                        s_conn_state.conn_handle = event->connect.conn_handle;
                        s_conn_state.mtu = 23;  // Default ATT MTU
                        s_conn_state.notifications_enabled = false;
                        s_conn_state.addr_type = desc.peer_id_addr.type;
                        memcpy(s_conn_state.addr, desc.peer_id_addr.val, 6);
                        ble_conn_state_unlock();
                    }
                }
                
                // Update connection parameters for low latency
                struct ble_gap_upd_params params = {
                    .itvl_min = BLE_CONN_INTERVAL_MIN,
                    .itvl_max = BLE_CONN_INTERVAL_MAX,
                    .latency = BLE_CONN_LATENCY,
                    .supervision_timeout = BLE_CONN_SUPERVISION_TIMEOUT,
                };
                
                int rc = ble_gap_update_params(event->connect.conn_handle, &params);
                if (rc != 0) {
                    ESP_LOGW(TAG, "Failed to update connection parameters: rc=%d", rc);
                }
                
#if CONFIG_BT_NIMBLE_PHY_2M
                // Request 2M PHY for faster OTA transfers (BLE 5.0)
                rc = ble_gap_set_prefered_le_phy(event->connect.conn_handle,
                                                  BLE_GAP_LE_PHY_2M_MASK,
                                                  BLE_GAP_LE_PHY_2M_MASK,
                                                  BLE_GAP_LE_PHY_CODED_ANY);
                if (rc == 0) {
                    ESP_LOGI(TAG, "Requested 2M PHY for faster transfers");
                } else {
                    ESP_LOGD(TAG, "2M PHY request failed (peer may not support BLE 5.0): rc=%d", rc);
                }
#endif
                
            } else {
                // Connection failed
                ESP_LOGW(TAG, "Connection failed: status=%d", event->connect.status);
                
                // Restart advertising
                ble_advertising_start();
            }
            break;
            
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "=== BLE Disconnection Event ===");
            ESP_LOGI(TAG, "  Reason: %d", event->disconnect.reason);
            ESP_LOGI(TAG, "  Handle: %d", event->disconnect.conn.conn_handle);
            
            if (ble_conn_state_lock()) {
                s_conn_state.connected = false;
                s_conn_state.notifications_enabled = false;
                s_conn_state.conn_handle = 0;
                s_conn_state.mtu = 23;
                memset(s_conn_state.addr, 0, 6);
                ble_conn_state_unlock();
            }
            
            // Clear RX buffer
            s_uart_rx_buffer_len = 0;
            
            // Restart advertising
            ESP_LOGI(TAG, "Restarting advertising...");
            ble_advertising_start();
            break;
            
        case BLE_GAP_EVENT_CONN_UPDATE:
            ESP_LOGI(TAG, "Connection parameters updated");
            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(event->conn_update.conn_handle, &desc) == 0) {
                ESP_LOGI(TAG, "  Interval: %d", desc.conn_itvl);
                ESP_LOGI(TAG, "  Latency: %d", desc.conn_latency);
                ESP_LOGI(TAG, "  Timeout: %d", desc.supervision_timeout);
            }
            break;
            
        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(TAG, "MTU update: channel_id=%d, mtu=%d",
                     event->mtu.channel_id, event->mtu.value);
            
            if (ble_conn_state_lock()) {
                s_conn_state.mtu = event->mtu.value;
                ble_conn_state_unlock();
            }
            break;
            
        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG, "Subscribe event: conn_handle=%d, attr_handle=%d, reason=%d, "
                     "prevn=%d, curn=%d, previ=%d, curi=%d",
                     event->subscribe.conn_handle,
                     event->subscribe.attr_handle,
                     event->subscribe.reason,
                     event->subscribe.prev_notify,
                     event->subscribe.cur_notify,
                     event->subscribe.prev_indicate,
                     event->subscribe.cur_indicate);
            
            // Check if this is for our TX characteristic
            if (event->subscribe.attr_handle == s_nus_tx_handle) {
                if (ble_conn_state_lock()) {
                    s_conn_state.notifications_enabled = event->subscribe.cur_notify;
                    ble_conn_state_unlock();
                }
                
                ESP_LOGI(TAG, "TX notifications %s",
                         event->subscribe.cur_notify ? "ENABLED" : "DISABLED");
            }
            break;
            
        case BLE_GAP_EVENT_ENC_CHANGE:
            ESP_LOGI(TAG, "Encryption change: status=%d", event->enc_change.status);
            
            if (event->enc_change.status == 0) {
                struct ble_gap_conn_desc desc;
                int rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
                if (rc == 0) {
                    ESP_LOGI(TAG, "Connection encrypted: sec_state.encrypted=%d, "
                             "sec_state.authenticated=%d, sec_state.bonded=%d",
                             desc.sec_state.encrypted,
                             desc.sec_state.authenticated,
                             desc.sec_state.bonded);
                }
            }
            break;
            
        case BLE_GAP_EVENT_PASSKEY_ACTION:
            ESP_LOGI(TAG, "=== Passkey Action Event ===");
            ESP_LOGI(TAG, "  Action: %d", event->passkey.params.action);
            
            if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
                // Display passkey to user
                struct ble_sm_io pkey = {0};
                pkey.action = event->passkey.params.action;
                pkey.passkey = esp_random() % 1000000;  // 6-digit passkey
                
                ESP_LOGI(TAG, "===========================================");
                ESP_LOGI(TAG, "  PAIRING PASSKEY: %06" PRIu32, pkey.passkey);
                ESP_LOGI(TAG, "===========================================");
                
                // Store passkey for UI display
                if (ble_pairing_state_lock()) {
                    s_pairing_state.in_progress = true;
                    s_pairing_state.passkey = pkey.passkey;
                    s_pairing_state.conn_handle = event->passkey.conn_handle;
                    ble_pairing_state_unlock();
                }
                
                int rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                if (rc != 0) {
                    ESP_LOGE(TAG, "Failed to inject passkey: rc=%d", rc);
                }
            }
            break;
            
        case BLE_GAP_EVENT_REPEAT_PAIRING:
            ESP_LOGI(TAG, "Repeat pairing detected");
            
            // Delete old bond and allow re-pairing
            int rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, 
                                       &(struct ble_gap_conn_desc){0});
            if (rc == 0) {
                ESP_LOGI(TAG, "Allowing re-pairing");
                return BLE_GAP_REPEAT_PAIRING_RETRY;
            }
            break;
            
        case BLE_GAP_EVENT_NOTIFY_TX:
            ESP_LOGD(TAG, "Notification TX complete: status=%d, attr_handle=%d",
                     event->notify_tx.status, event->notify_tx.attr_handle);
            break;
            
        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "Advertising complete: reason=%d", event->adv_complete.reason);
            
            // Restart advertising if it stopped unexpectedly
            if (event->adv_complete.reason != 0) {
                ESP_LOGW(TAG, "Advertising stopped unexpectedly, restarting...");
                ble_advertising_start();
            }
            break;
            
        default:
            ESP_LOGD(TAG, "Unhandled GAP event: %d", event->type);
            break;
    }
    
    return 0;
}


//==============================================================================
// ADVERTISING AND SERVICE INITIALIZATION
//==============================================================================

/**
 * @brief Start BLE advertising
 * 
 * Configures and starts BLE advertising with device name and service UUIDs.
 * Uses connectable undirected advertising.
 * 
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t ble_advertising_start(void)
{
    ESP_LOGI(TAG, "Starting BLE advertising...");
    
    // Get device name from configuration
    general_config_t config;
    general_config_get(&config);
    
    char ble_name[BLE_DEVICE_NAME_MAX_LEN + 1];
    snprintf(ble_name, sizeof(ble_name), "%s %.23s", 
             BLE_DEVICE_NAME_PREFIX, config.device_name);
    
    // Set device name
    int rc = ble_svc_gap_device_name_set(ble_name);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set device name: rc=%d", rc);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Device name: %s", ble_name);
    
    // Configure advertising parameters
    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,  // Undirected connectable
        .disc_mode = BLE_GAP_DISC_MODE_GEN,  // General discoverable
        .itvl_min = BLE_ADV_INTERVAL_MIN,
        .itvl_max = BLE_ADV_INTERVAL_MAX,
        .channel_map = 0,                     // All channels
        .filter_policy = 0,                   // No filter
        .high_duty_cycle = 0,
    };
    
    // Start advertising
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event_handler, NULL);
    
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start advertising: rc=%d", rc);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Advertising started successfully");
    return ESP_OK;
}

/**
 * @brief Initialize GATT services
 * 
 * Registers all GATT services and characteristics with the NimBLE stack.
 * 
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t ble_services_init(void)
{
    ESP_LOGI(TAG, "Initializing GATT services...");
    
    // Register GATT services
    int rc = ble_gatts_count_cfg(gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count GATT services: rc=%d", rc);
        return ESP_FAIL;
    }
    
    rc = ble_gatts_add_svcs(gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT services: rc=%d", rc);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "GATT services registered successfully");
    ESP_LOGI(TAG, "  NUS TX handle: %d", s_nus_tx_handle);
    ESP_LOGI(TAG, "  NUS RX handle: %d", s_nus_rx_handle);
    
    return ESP_OK;
}

/**
 * @brief NimBLE host task
 * 
 * This task runs the NimBLE host stack. It must run continuously for BLE
 * to function properly.
 * 
 * @param param Unused parameter
 */
static void ble_host_task(void *param)
{
    (void)param;
    
    ESP_LOGI(TAG, "NimBLE host task started");
    
    // This function blocks until nimble_port_stop() is called
    nimble_port_run();
    
    ESP_LOGI(TAG, "NimBLE host task exiting");
    nimble_port_freertos_deinit();
}

/**
 * @brief NimBLE stack synchronization callback
 * 
 * Called when the NimBLE host and controller are synchronized and ready.
 * This is where we start advertising and enable services.
 */
static void ble_on_sync(void)
{
    ESP_LOGI(TAG, "=== NimBLE Stack Synchronized ===");
    
    // Ensure we have a public address
    int rc = ble_hs_id_infer_auto(0, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to determine address type: rc=%d", rc);
        return;
    }
    
    // Get and log our address
    uint8_t addr[6] = {0};
    rc = ble_hs_id_copy_addr(BLE_ADDR_PUBLIC, addr, NULL);
    if (rc == 0) {
        ESP_LOGI(TAG, "Device address: %02X:%02X:%02X:%02X:%02X:%02X",
                 addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    }
    
    // Start advertising
    ble_advertising_start();
}

/**
 * @brief NimBLE stack reset callback
 * 
 * Called when the NimBLE stack resets (usually due to an error).
 * We log the reason and attempt recovery.
 * 
 * @param reason Reset reason code
 */
static void ble_on_reset(int reason)
{
    ESP_LOGW(TAG, "=== NimBLE Stack Reset ===");
    ESP_LOGW(TAG, "  Reason: %d", reason);
    
    // Clear connection state
    if (ble_conn_state_lock()) {
        memset(&s_conn_state, 0, sizeof(s_conn_state));
        ble_conn_state_unlock();
    }
    
    // Clear pairing state
    if (ble_pairing_state_lock()) {
        memset(&s_pairing_state, 0, sizeof(s_pairing_state));
        ble_pairing_state_unlock();
    }
}


//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

/**
 * @brief Initialize Bluetooth configuration service
 * 
 * This is the main initialization function that sets up the entire BLE stack:
 * - Initializes NimBLE controller and host
 * - Configures security (pairing, bonding, encryption)
 * - Registers GATT services
 * - Creates UART processing task
 * - Starts advertising
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bluetooth_config_init(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Bluetooth Initialization (NimBLE)");
    ESP_LOGI(TAG, "========================================");
    
    // Check if already initialized
    if (s_ble_initialized) {
        ESP_LOGW(TAG, "Bluetooth already initialized");
        return ESP_OK;
    }
    
    // Check if Bluetooth should be enabled
    general_config_t config;
    general_config_get(&config);
    
    if (!config.bluetooth_enabled) {
        ESP_LOGI(TAG, "Bluetooth disabled in configuration");
        return ESP_OK;
    }
    
    //--------------------------------------------------------------------------
    // Step 1: Create synchronization primitives
    //--------------------------------------------------------------------------
    ESP_LOGI(TAG, "[1/8] Creating synchronization primitives...");
    
    s_conn_state_mutex = xSemaphoreCreateMutex();
    if (!s_conn_state_mutex) {
        ESP_LOGE(TAG, "Failed to create connection state mutex");
        return ESP_ERR_NO_MEM;
    }
    
    s_pairing_state_mutex = xSemaphoreCreateMutex();
    if (!s_pairing_state_mutex) {
        ESP_LOGE(TAG, "Failed to create pairing state mutex");
        vSemaphoreDelete(s_conn_state_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    //--------------------------------------------------------------------------
    // Step 2: Create UART RX queue and task
    //--------------------------------------------------------------------------
    ESP_LOGI(TAG, "[2/8] Creating BLE UART infrastructure...");
    
    s_uart_rx_queue = xQueueCreate(BLE_UART_RX_QUEUE_SIZE, sizeof(ble_uart_msg_t));
    if (!s_uart_rx_queue) {
        ESP_LOGE(TAG, "Failed to create UART RX queue");
        vSemaphoreDelete(s_conn_state_mutex);
        vSemaphoreDelete(s_pairing_state_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    BaseType_t task_ret = xTaskCreate(ble_uart_task, "ble_uart",
                                       BLE_UART_TASK_STACK_SIZE, NULL,
                                       BLE_UART_TASK_PRIORITY, &s_uart_task_handle);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART task");
        vQueueDelete(s_uart_rx_queue);
        vSemaphoreDelete(s_conn_state_mutex);
        vSemaphoreDelete(s_pairing_state_mutex);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "  Queue size: %d messages", BLE_UART_RX_QUEUE_SIZE);
    ESP_LOGI(TAG, "  Task stack: %d bytes", BLE_UART_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "  Task priority: %d", BLE_UART_TASK_PRIORITY);
    
    //--------------------------------------------------------------------------
    // Step 3: Initialize NVS (required for bonding)
    //--------------------------------------------------------------------------
    ESP_LOGI(TAG, "[3/8] Initializing NVS for bonding...");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erasing, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    //--------------------------------------------------------------------------
    // Step 4: Initialize NimBLE controller
    //--------------------------------------------------------------------------
    ESP_LOGI(TAG, "[4/8] Initializing NimBLE controller...");
    
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NimBLE port init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    //--------------------------------------------------------------------------
    // Step 5: Configure NimBLE host
    //--------------------------------------------------------------------------
    ESP_LOGI(TAG, "[5/8] Configuring NimBLE host...");
    
    // Set sync and reset callbacks
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.gatts_register_cb = NULL;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    
    // Configure security manager
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP;
    ble_hs_cfg.sm_bonding = BLE_SM_BONDING;
    ble_hs_cfg.sm_mitm = BLE_SM_MITM;
    ble_hs_cfg.sm_sc = BLE_SM_SC;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    
    ESP_LOGI(TAG, "  Security configuration:");
    ESP_LOGI(TAG, "    I/O Capability: %s", ble_sm_io_cap_name(BLE_SM_IO_CAP));
    ESP_LOGI(TAG, "    Bonding: %s", BLE_SM_BONDING ? "enabled" : "disabled");
    ESP_LOGI(TAG, "    MITM: %s", BLE_SM_MITM ? "enabled" : "disabled");
    ESP_LOGI(TAG, "    Secure Connections: %s", BLE_SM_SC ? "enabled" : "disabled");
    
    //--------------------------------------------------------------------------
    // Step 6: Initialize GATT services
    //--------------------------------------------------------------------------
    ESP_LOGI(TAG, "[6/8] Initializing GATT services...");
    
    // Initialize default GAP and GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    // Initialize our custom services
    ret = ble_services_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize services: %s", esp_err_to_name(ret));
        return ret;
    }
    
    //--------------------------------------------------------------------------
    // Step 7: Set preferred MTU
    //--------------------------------------------------------------------------
    ESP_LOGI(TAG, "[7/8] Setting preferred MTU to %d bytes...", BLE_MTU_SIZE);
    
    int rc = ble_att_set_preferred_mtu(BLE_MTU_SIZE);
    if (rc != 0) {
        ESP_LOGW(TAG, "Failed to set preferred MTU: rc=%d", rc);
    }
    
    //--------------------------------------------------------------------------
    // Step 8: Start NimBLE host task
    //--------------------------------------------------------------------------
    ESP_LOGI(TAG, "[8/8] Starting NimBLE host task...");
    
    nimble_port_freertos_init(ble_host_task);
    
    // Mark as initialized
    s_ble_initialized = true;
    s_ble_enabled = true;
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Bluetooth Initialization Complete");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Waiting for NimBLE stack synchronization...");
    
    // Initialize OTA handler
    ESP_LOGI(TAG, "Initializing BLE OTA handler...");
    if (ble_ota_handler_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize OTA handler");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * @brief Enable or disable Bluetooth
 * 
 * Updates the configuration setting for Bluetooth enable/disable.
 * Note: This does not dynamically start/stop the BLE stack, only updates
 * the configuration for the next boot.
 * 
 * @param enabled true to enable, false to disable
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bluetooth_config_set_enabled(bool enabled)
{
    general_config_t config;
    general_config_get(&config);
    config.bluetooth_enabled = enabled;
    return general_config_set(&config);
}

/**
 * @brief Check if Bluetooth is enabled
 * 
 * @return true if Bluetooth is initialized and enabled, false otherwise
 */
bool bluetooth_config_is_enabled(void)
{
    return s_ble_enabled;
}

/**
 * @brief Check if a BLE client is connected
 * 
 * @return true if connected, false otherwise
 */
bool bluetooth_config_is_connected(void)
{
    bool connected = false;
    
    if (ble_conn_state_lock()) {
        connected = s_conn_state.connected;
        ble_conn_state_unlock();
    }
    
    return connected;
}

/**
 * @brief Get current pairing passkey
 * 
 * If pairing is in progress, returns the 6-digit passkey that should be
 * displayed to the user for verification.
 * 
 * @param passkey Output buffer for passkey (6 digits)
 * @return true if pairing in progress and passkey available, false otherwise
 */
bool bluetooth_config_get_passkey(uint32_t *passkey)
{
    if (!passkey) {
        return false;
    }
    
    bool has_passkey = false;
    
    if (ble_pairing_state_lock()) {
        if (s_pairing_state.in_progress) {
            *passkey = s_pairing_state.passkey;
            has_passkey = true;
        }
        ble_pairing_state_unlock();
    }
    
    return has_passkey;
}
