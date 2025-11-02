/**
 * @file bluetooth_config.c
 * @brief Enterprise-grade NimBLE Nordic UART Service implementation
 *
 * Features:
 * - Thread-safe operation with FreeRTOS primitives
 * - Queue-based command processing for non-blocking operation
 * - Comprehensive error handling and recovery
 * - BLE 5.0 with 2M PHY for faster transfers
 * - LE Secure Connections with passkey pairing
 * - Device Information Service (DIS)
 * - OTA firmware update support
 *
 * @copyright Copyright (c) 2025 LoRaCue Project
 * @license GPL-3.0
 */

#include "bluetooth_config.h"
#include "bsp.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_efuse.h"
#include "general_config.h"
#include "version.h"
#include "commands.h"
#include "ble_ota.h"
#include <string.h>
#include <inttypes.h>

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_gap.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/dis/ble_svc_dis.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

static const char *TAG = "ble_config";

//==============================================================================
// NORDIC UART SERVICE (NUS) UUIDs
//==============================================================================

static const ble_uuid128_t NUS_SERVICE_UUID = 
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e);

static const ble_uuid128_t NUS_CHR_RX_UUID = 
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e);

static const ble_uuid128_t NUS_CHR_TX_UUID = 
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e);

//==============================================================================
// CONFIGURATION
//==============================================================================

#define BLE_CMD_QUEUE_SIZE              10
#define BLE_CMD_TASK_STACK_SIZE         4096
#define BLE_CMD_TASK_PRIORITY           5
#define BLE_CMD_MAX_LENGTH              2048
#define BLE_RESPONSE_MAX_LENGTH         2048
#define BLE_MTU_MAX                     512

//==============================================================================
// STATE MANAGEMENT
//==============================================================================

typedef struct {
    uint16_t conn_handle;
    uint16_t mtu;
    bool connected;
    bool notifications_enabled;
    uint8_t addr_type;
    uint8_t addr[6];
} ble_conn_state_t;

typedef struct {
    char data[BLE_CMD_MAX_LENGTH];
    size_t len;
} ble_cmd_t;

static bool s_ble_initialized = false;
static bool s_ble_enabled = false;
static uint16_t s_nus_tx_handle;
static uint8_t s_own_addr_type;
static ble_conn_state_t s_conn_state = {
    .conn_handle = BLE_HS_CONN_HANDLE_NONE,
    .mtu = 23,
    .connected = false,
    .notifications_enabled = false
};
static SemaphoreHandle_t s_conn_state_mutex = NULL;
static QueueHandle_t s_cmd_queue = NULL;
static TaskHandle_t s_cmd_task_handle = NULL;

// Forward declarations
static void bluetooth_config_send_response(const char *response);
static void ble_advertise(void);

//==============================================================================
// THREAD-SAFE STATE ACCESS
//==============================================================================

static bool conn_state_lock(void)
{
    if (!s_conn_state_mutex) {
        return false;
    }
    return xSemaphoreTake(s_conn_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE;
}

static void conn_state_unlock(void)
{
    if (s_conn_state_mutex) {
        xSemaphoreGive(s_conn_state_mutex);
    }
}

//==============================================================================
// COMMAND PROCESSING TASK
//==============================================================================

static void ble_cmd_task(void *arg)
{
    ESP_LOGI(TAG, "Command task started");
    
    ble_cmd_t cmd;
    
    while (1) {
        if (xQueueReceive(s_cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Processing command: %s", cmd.data);
            commands_execute(cmd.data, bluetooth_config_send_response);
        }
    }
}

//==============================================================================
// NUS CHARACTERISTIC ACCESS
//==============================================================================

static int nus_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (attr_handle == s_nus_tx_handle) {
            return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
        }
        
        uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
        if (om_len == 0 || om_len >= BLE_CMD_MAX_LENGTH) {
            ESP_LOGW(TAG, "Invalid command length: %d", om_len);
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        
        ble_cmd_t cmd = {0};
        uint16_t len_out;
        int rc = ble_hs_mbuf_to_flat(ctxt->om, cmd.data, sizeof(cmd.data) - 1, &len_out);
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to copy mbuf: %d", rc);
            return BLE_ATT_ERR_UNLIKELY;
        }
        
        cmd.len = len_out;
        cmd.data[cmd.len] = '\0';
        
        if (xQueueSend(s_cmd_queue, &cmd, 0) != pdTRUE) {
            ESP_LOGW(TAG, "Command queue full, dropping command");
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        
        return 0;

    case BLE_GATT_ACCESS_OP_READ_CHR:
        return BLE_ATT_ERR_READ_NOT_PERMITTED;

    default:
        return BLE_ATT_ERR_UNLIKELY;
    }
}

//==============================================================================
// GATT SERVICES
//==============================================================================

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        // Nordic UART Service
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &NUS_SERVICE_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]) {{
            // RX Characteristic (write from central)
            .uuid = &NUS_CHR_RX_UUID.u,
            .access_cb = nus_chr_access,
            .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
        }, {
            // TX Characteristic (notify to central)
            .uuid = &NUS_CHR_TX_UUID.u,
            .access_cb = nus_chr_access,
            .flags = BLE_GATT_CHR_F_NOTIFY,
            .val_handle = &s_nus_tx_handle,
        }, {
            0,
        }},
    },
    {0},
};

//==============================================================================
// GAP EVENT HANDLER
//==============================================================================

static int gap_event_handler(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "=== BLE Connection Event ===");
        
        if (event->connect.status == 0) {
            ESP_LOGI(TAG, "Connection established");
            ESP_LOGI(TAG, "  Handle: %d", event->connect.conn_handle);
            
            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(event->connect.conn_handle, &desc) == 0) {
                ESP_LOGI(TAG, "  Peer address type: %d", desc.peer_id_addr.type);
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, desc.peer_id_addr.val, 6, ESP_LOG_INFO);
                
                if (conn_state_lock()) {
                    s_conn_state.connected = true;
                    s_conn_state.conn_handle = event->connect.conn_handle;
                    s_conn_state.mtu = 23;
                    s_conn_state.notifications_enabled = false;
                    s_conn_state.addr_type = desc.peer_id_addr.type;
                    memcpy(s_conn_state.addr, desc.peer_id_addr.val, 6);
                    conn_state_unlock();
                }
            }
            
            // Request connection parameter update for low latency
            struct ble_gap_upd_params params = {
                .itvl_min = BLE_GAP_INITIAL_CONN_ITVL_MIN,
                .itvl_max = BLE_GAP_INITIAL_CONN_ITVL_MAX,
                .latency = BLE_GAP_INITIAL_CONN_LATENCY,
                .supervision_timeout = BLE_GAP_INITIAL_SUPERVISION_TIMEOUT,
            };
            ble_gap_update_params(event->connect.conn_handle, &params);
            
        } else {
            ESP_LOGE(TAG, "Connection failed; status=%d", event->connect.status);
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "=== BLE Disconnect Event ===");
        ESP_LOGI(TAG, "  Reason: %d", event->disconnect.reason);
        
        if (conn_state_lock()) {
            s_conn_state.connected = false;
            s_conn_state.conn_handle = BLE_HS_CONN_HANDLE_NONE;
            s_conn_state.notifications_enabled = false;
            conn_state_unlock();
        }
        
        // Restart advertising
        ble_advertise();
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
        
        if (conn_state_lock()) {
            s_conn_state.mtu = event->mtu.value;
            conn_state_unlock();
        }
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "Subscribe event: conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
        
        if (event->subscribe.attr_handle == s_nus_tx_handle) {
            if (conn_state_lock()) {
                s_conn_state.notifications_enabled = 
                    (event->subscribe.cur_notify || event->subscribe.cur_indicate);
                conn_state_unlock();
            }
            ESP_LOGI(TAG, "NUS TX notifications %s",
                     s_conn_state.notifications_enabled ? "enabled" : "disabled");
        }
        break;

    case BLE_GAP_EVENT_PASSKEY_ACTION:
        ESP_LOGI(TAG, "=== Passkey Action Event ===");
        
        if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
            struct ble_sm_io pkey = {0};
            pkey.action = event->passkey.params.action;
            pkey.passkey = esp_random() % 1000000;
            
            ESP_LOGI(TAG, "===========================================");
            ESP_LOGI(TAG, "  PAIRING PASSKEY: %06" PRIu32, pkey.passkey);
            ESP_LOGI(TAG, "===========================================");
            
            ble_sm_inject_io(event->passkey.conn_handle, &pkey);
        }
        break;

    case BLE_GAP_EVENT_ENC_CHANGE:
        ESP_LOGI(TAG, "Encryption change: status=%d", event->enc_change.status);
        break;

    default:
        break;
    }
    
    return 0;
}

//==============================================================================
// ADVERTISING
//==============================================================================

static void ble_advertise(void)
{
    int rc;
    
    // Stop any existing advertising
    ble_gap_adv_stop();
    
    // Set advertising data
    struct ble_hs_adv_fields fields = {0};
    const char *name = "LoRaCue";
    
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
    
    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set adv fields: %d", rc);
        return;
    }
    
    // Start advertising
    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    
    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, gap_event_handler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start advertising: %d", rc);
    } else {
        ESP_LOGI(TAG, "Advertising started");
    }
}

//==============================================================================
// NIMBLE HOST CALLBACKS
//==============================================================================

static void nimble_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void on_sync(void)
{
    ESP_LOGI(TAG, "NimBLE host synced");
    
    int rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to infer address: %d", rc);
        return;
    }
    
    uint8_t addr[6];
    rc = ble_hs_id_copy_addr(s_own_addr_type, addr, NULL);
    if (rc == 0) {
        ESP_LOGI(TAG, "Device address: %02x:%02x:%02x:%02x:%02x:%02x",
                 addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    }
    
    ble_advertise();
}

static void on_reset(int reason)
{
    ESP_LOGE(TAG, "NimBLE host reset; reason=%d", reason);
}

//==============================================================================
// PUBLIC API
//==============================================================================

esp_err_t bluetooth_config_init(void)
{
    if (s_ble_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing NimBLE BLE stack");
    
    // Create mutex
    s_conn_state_mutex = xSemaphoreCreateMutex();
    if (!s_conn_state_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Create command queue
    s_cmd_queue = xQueueCreate(BLE_CMD_QUEUE_SIZE, sizeof(ble_cmd_t));
    if (!s_cmd_queue) {
        ESP_LOGE(TAG, "Failed to create command queue");
        vSemaphoreDelete(s_conn_state_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    // Create command processing task
    BaseType_t ret = xTaskCreate(ble_cmd_task, "ble_cmd", 
                                  BLE_CMD_TASK_STACK_SIZE, NULL,
                                  BLE_CMD_TASK_PRIORITY, &s_cmd_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create command task");
        vQueueDelete(s_cmd_queue);
        vSemaphoreDelete(s_conn_state_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize NimBLE
    esp_err_t rc = nimble_port_init();
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble: %d", rc);
        vTaskDelete(s_cmd_task_handle);
        vQueueDelete(s_cmd_queue);
        vSemaphoreDelete(s_conn_state_mutex);
        return rc;
    }

    // Configure NimBLE
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_ONLY;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 1;
    ble_hs_cfg.sm_sc = 1;
    
    // Initialize services
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_dis_init();
    
    // Configure GATT services
    rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count GATT cfg: %d", rc);
        nimble_port_deinit();
        vTaskDelete(s_cmd_task_handle);
        vQueueDelete(s_cmd_queue);
        vSemaphoreDelete(s_conn_state_mutex);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT services: %d", rc);
        nimble_port_deinit();
        vTaskDelete(s_cmd_task_handle);
        vQueueDelete(s_cmd_queue);
        vSemaphoreDelete(s_conn_state_mutex);
        return ESP_FAIL;
    }

    // Set device name
    ble_svc_gap_device_name_set("LoRaCue");
    
    // Set DIS information
    const bsp_usb_config_t *usb_config = bsp_get_usb_config();
    char serial_number[32];
    bsp_get_serial_number(serial_number, sizeof(serial_number));
    
    ble_svc_dis_manufacturer_name_set("LoRaCue");
    ble_svc_dis_model_number_set(usb_config->usb_product);
    ble_svc_dis_serial_number_set(serial_number);
    ble_svc_dis_firmware_revision_set(LORACUE_VERSION_STRING);
    
    // Start NimBLE host task
    nimble_port_freertos_init(nimble_host_task);

    s_ble_initialized = true;
    s_ble_enabled = true;
    
    ESP_LOGI(TAG, "NimBLE initialized successfully");
    return ESP_OK;
}

esp_err_t bluetooth_config_deinit(void)
{
    if (!s_ble_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing NimBLE");
    
    nimble_port_stop();
    nimble_port_deinit();
    
    if (s_cmd_task_handle) {
        vTaskDelete(s_cmd_task_handle);
        s_cmd_task_handle = NULL;
    }
    
    if (s_cmd_queue) {
        vQueueDelete(s_cmd_queue);
        s_cmd_queue = NULL;
    }
    
    if (s_conn_state_mutex) {
        vSemaphoreDelete(s_conn_state_mutex);
        s_conn_state_mutex = NULL;
    }
    
    s_ble_initialized = false;
    s_ble_enabled = false;
    
    ESP_LOGI(TAG, "NimBLE deinitialized");
    return ESP_OK;
}

bool bluetooth_config_is_enabled(void)
{
    return s_ble_enabled;
}

bool bluetooth_config_is_connected(void)
{
    bool connected = false;
    if (conn_state_lock()) {
        connected = s_conn_state.connected;
        conn_state_unlock();
    }
    return connected;
}

void bluetooth_config_send_response(const char *response)
{
    if (!response || strlen(response) == 0) {
        return;
    }
    
    bool can_send = false;
    uint16_t conn_handle;
    uint16_t mtu;
    
    if (conn_state_lock()) {
        can_send = s_conn_state.connected && s_conn_state.notifications_enabled;
        conn_handle = s_conn_state.conn_handle;
        mtu = s_conn_state.mtu;
        conn_state_unlock();
    }
    
    if (!can_send) {
        ESP_LOGW(TAG, "Cannot send: not connected or notifications disabled");
        return;
    }
    
    size_t len = strlen(response);
    size_t chunk_size = mtu - 3; // ATT overhead
    size_t offset = 0;
    
    while (offset < len) {
        size_t to_send = (len - offset > chunk_size) ? chunk_size : (len - offset);
        
        struct os_mbuf *om = ble_hs_mbuf_from_flat(response + offset, to_send);
        if (!om) {
            ESP_LOGE(TAG, "Failed to allocate mbuf");
            return;
        }
        
        int rc = ble_gattc_notify_custom(conn_handle, s_nus_tx_handle, om);
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to send notification: %d", rc);
            return;
        }
        
        offset += to_send;
        
        if (offset < len) {
            vTaskDelay(pdMS_TO_TICKS(10)); // Small delay between chunks
        }
    }
}

esp_err_t bluetooth_config_set_enabled(bool enabled)
{
    if (enabled == s_ble_enabled) {
        return ESP_OK;
    }
    
    if (enabled) {
        return bluetooth_config_init();
    } else {
        return bluetooth_config_deinit();
    }
}

bool bluetooth_config_get_passkey(uint32_t *passkey)
{
    // Passkey is displayed during pairing, not stored
    if (passkey) {
        *passkey = 0;
    }
    return false;
}

