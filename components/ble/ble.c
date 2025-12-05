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

#include "ble.h"
#include "ble_ota.h"
#include "ble_ota_handler.h"
#include "ble_ota_integration.h"
#include "bsp.h"
#include "commands.h"
#include "esp_efuse.h"
#include "esp_log.h"
#include "esp_random.h"
#include "general_config.h"
#include "sdkconfig.h"
#include "version.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "os/os_mbuf.h"
#include "services/dis/ble_svc_dis.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "store/config/ble_store_config.h"

// Forward declaration - defined in NimBLE port
void ble_store_config_init(void);

static const char *TAG = "ble";

//==============================================================================
// NORDIC UART SERVICE (NUS) UUIDs
//==============================================================================

static const ble_uuid128_t NUS_SERVICE_UUID =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e);

static const ble_uuid128_t NUS_CHR_RX_UUID =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e);

static const ble_uuid128_t NUS_CHR_TX_UUID =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e);

//==============================================================================
// CONFIGURATION
//==============================================================================

#define BLE_CMD_QUEUE_SIZE 10
#define BLE_CMD_TASK_STACK_SIZE 6144
#define BLE_CMD_TASK_PRIORITY 5
#define BLE_CMD_MAX_LENGTH 2048
#define BLE_RESPONSE_MAX_LENGTH 2048
#define BLE_MTU_MAX 512
#define BLE_DEFAULT_MTU 23
#define BLE_MUTEX_WAIT_MS 100
#define BLE_ADV_START_DELAY_MS 100
#define BLE_CHUNK_DELAY_MS 10
#define BLE_ADV_TASK_STACK_SIZE 3072
#define BLE_ADV_TASK_PRIORITY 5

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
    bool pairing_active;
    uint32_t passkey;
} ble_conn_state_t;

typedef struct {
    char data[BLE_CMD_MAX_LENGTH];
    size_t len;
} ble_cmd_t;

static bool s_ble_initialized = false;
static bool s_ble_enabled     = false;
static uint16_t s_nus_tx_handle;
static uint8_t s_own_addr_type;
static ble_conn_state_t s_conn_state        = {.conn_handle           = BLE_HS_CONN_HANDLE_NONE,
                                               .mtu                   = BLE_DEFAULT_MTU,
                                               .connected             = false,
                                               .notifications_enabled = false,
                                               .pairing_active        = false,
                                               .passkey               = 0};
static SemaphoreHandle_t s_conn_state_mutex = NULL;
static QueueHandle_t s_cmd_queue            = NULL;
static TaskHandle_t s_cmd_task_handle       = NULL;

// Forward declarations
static void ble_send_response(const char *response);
static void ble_advertise(void);

//==============================================================================
// THREAD-SAFE STATE ACCESS
//==============================================================================

static bool conn_state_lock(void)
{
    if (s_conn_state_mutex == NULL) {
        return false;
    }
    return xSemaphoreTake(s_conn_state_mutex, pdMS_TO_TICKS(BLE_MUTEX_WAIT_MS)) == pdTRUE;
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
            commands_execute(cmd.data, ble_send_response);
        }
    }
}

//==============================================================================
// NUS CHARACTERISTIC ACCESS
//==============================================================================

static int nus_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
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

            cmd.len           = len_out;
            cmd.data[cmd.len] = '\0';

            // Strip trailing CR/LF (same as UART handler)
            while (cmd.len > 0 && (cmd.data[cmd.len - 1] == '\r' || cmd.data[cmd.len - 1] == '\n')) {
                cmd.data[--cmd.len] = '\0';
            }

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
        .type            = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid            = &NUS_SERVICE_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]){{
                                                           // RX Characteristic (write from central)
                                                           .uuid      = &NUS_CHR_RX_UUID.u,
                                                           .access_cb = nus_chr_access,
                                                           .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                                                       },
                                                       {
                                                           // TX Characteristic (notify to central)
                                                           .uuid       = &NUS_CHR_TX_UUID.u,
                                                           .access_cb  = nus_chr_access,
                                                           .flags      = BLE_GATT_CHR_F_NOTIFY,
                                                           .val_handle = &s_nus_tx_handle,
                                                       },
                                                       {
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
                        s_conn_state.connected             = true;
                        s_conn_state.conn_handle           = event->connect.conn_handle;
                        s_conn_state.mtu                   = BLE_DEFAULT_MTU;
                        s_conn_state.notifications_enabled = false;
                        s_conn_state.addr_type             = desc.peer_id_addr.type;
                        memcpy(s_conn_state.addr, desc.peer_id_addr.val, 6);
                        conn_state_unlock();
                    }

                    // Set connection handle for OTA operations
                    ble_ota_set_connection_handle(event->connect.conn_handle);
                    ble_ota_handler_set_connection(event->connect.conn_handle);
                }

                // Request connection parameter update for low latency
                struct ble_gap_upd_params params = {
                    .itvl_min            = BLE_GAP_INITIAL_CONN_ITVL_MIN,
                    .itvl_max            = BLE_GAP_INITIAL_CONN_ITVL_MAX,
                    .latency             = BLE_GAP_INITIAL_CONN_LATENCY,
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
                s_conn_state.connected             = false;
                s_conn_state.conn_handle           = BLE_HS_CONN_HANDLE_NONE;
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
            ESP_LOGI(TAG, "MTU update: channel_id=%d, mtu=%d", event->mtu.channel_id, event->mtu.value);

            if (conn_state_lock()) {
                s_conn_state.mtu = event->mtu.value;
                conn_state_unlock();
            }
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG, "Subscribe event: conn_handle=%d attr_handle=%d", event->subscribe.conn_handle,
                     event->subscribe.attr_handle);

            if (event->subscribe.attr_handle == s_nus_tx_handle) {
                if (conn_state_lock()) {
                    s_conn_state.notifications_enabled = (event->subscribe.cur_notify || event->subscribe.cur_indicate);
                    conn_state_unlock();
                }
                ESP_LOGI(TAG, "NUS TX notifications %s", s_conn_state.notifications_enabled ? "enabled" : "disabled");
            }
            break;

        case BLE_GAP_EVENT_PASSKEY_ACTION:
            ESP_LOGI(TAG, "=== Passkey Action Event ===");

            if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
                struct ble_sm_io pkey = {0};
                pkey.action           = event->passkey.params.action;
                pkey.passkey          = esp_random() % 1000000;

                ESP_LOGI(TAG, "===========================================");
                ESP_LOGI(TAG, "  PAIRING PASSKEY: %06" PRIu32, pkey.passkey);
                ESP_LOGI(TAG, "===========================================");

                // Store passkey for UI display
                if (conn_state_lock()) {
                    s_conn_state.pairing_active = true;
                    s_conn_state.passkey        = pkey.passkey;
                    conn_state_unlock();
                }

                ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            }
            break;

        case BLE_GAP_EVENT_ENC_CHANGE:
            ESP_LOGI(TAG, "Encryption change: status=%d", event->enc_change.status);

            // Clear pairing state when encryption established
            if (event->enc_change.status == 0 && conn_state_lock()) {
                s_conn_state.pairing_active = false;
                s_conn_state.passkey        = 0;
                conn_state_unlock();
            }
            break;

        default:
            break;
    }

    return 0;
}

//==============================================================================
// VERSION HELPERS
//==============================================================================

static uint8_t get_release_type(void)
{
    const char *version = LORACUE_VERSION_STRING;

    if (strstr(version, "-alpha"))
        return RELEASE_TYPE_ALPHA;
    if (strstr(version, "-beta"))
        return RELEASE_TYPE_BETA;
    if (strchr(version, '-'))
        return RELEASE_TYPE_DEV;

    return RELEASE_TYPE_STABLE;
}

static uint16_t get_build_number(void)
{
    const char *version = LORACUE_VERSION_STRING;
    const char *dot     = strrchr(version, '.');

    if (dot && *(dot + 1) >= '0' && *(dot + 1) <= '9') {
        return (uint16_t)atoi(dot + 1);
    }
    return 0;
}

//==============================================================================
// ADVERTISING
//==============================================================================

static void ble_advertise(void)
{
    int rc;

    // Get device configuration for name
    general_config_t config;
    general_config_get(&config);

    // Build advertising name: "LoRaCue <device_name>" (max 31 bytes for BLE)
    char adv_name[32];
    int len = snprintf(adv_name, sizeof(adv_name), "LoRaCue %.22s", config.device_name);
    if (len >= (int)sizeof(adv_name)) {
        len = sizeof(adv_name) - 1;
    }
    uint8_t name_len = (uint8_t)len;

    ESP_LOGI(TAG, "Starting extended advertising as '%s'", adv_name);

    // Configure extended advertising instance
    struct ble_gap_ext_adv_params adv_params = {0};
    adv_params.connectable                   = 1;
    adv_params.scannable                     = 1;
    adv_params.legacy_pdu                    = 1; // Use legacy PDU for backward compatibility
    adv_params.itvl_min                      = BLE_GAP_ADV_FAST_INTERVAL1_MIN;
    adv_params.itvl_max                      = BLE_GAP_ADV_FAST_INTERVAL1_MAX;

    uint8_t instance = 0;
    rc               = ble_gap_ext_adv_configure(instance, &adv_params, NULL, gap_event_handler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to configure ext adv: %d", rc);
        return;
    }

    // Prepare service data
#ifdef CONFIG_LORACUE_MODEL_NAME
    const char *model_name = CONFIG_LORACUE_MODEL_NAME;
#else
    const char *model_name = "LC-Alpha"; // Default fallback
#endif
    size_t model_len     = strlen(model_name); // No null terminator in advertisement
    uint16_t build_flags = BUILD_NUMBER(get_build_number()) | get_release_type();

    // Build advertising data with flags, service data, and complete local name
    uint8_t adv_data[31]; // Use legacy size for compatibility
    uint8_t pos = 0;

    // Flags (3 bytes)
    adv_data[pos++] = 0x02;
    adv_data[pos++] = 0x01;
    adv_data[pos++] = 0x06;

    // Manufacturer Specific Data (2 + 2 + 5 + model_len bytes)
    uint8_t mfg_data_len = 2 + 5 + model_len; // Company ID + version + model
    // cppcheck-suppress knownConditionTrueFalse
    if (pos + 2 + mfg_data_len <= 31) {
        adv_data[pos++] = 1 + mfg_data_len; // Length
        adv_data[pos++] = 0xFF;             // Manufacturer Specific Data

        // Company ID (0xFFFF for development/testing)
        adv_data[pos++] = 0xFF;
        adv_data[pos++] = 0xFF;

        // Version and build info (5 bytes)
        adv_data[pos++] = LORACUE_VERSION_MAJOR;
        adv_data[pos++] = LORACUE_VERSION_MINOR;
        adv_data[pos++] = LORACUE_VERSION_PATCH;
        adv_data[pos++] = build_flags & 0xFF;
        adv_data[pos++] = (build_flags >> 8) & 0xFF;

        // Model name (null-terminated)
        memcpy(&adv_data[pos], model_name, model_len);
        pos += model_len;
    }

    ESP_LOGI(TAG, "Advertising data size: %d bytes (max 31)", pos);

    struct os_mbuf *data = ble_hs_mbuf_from_flat(adv_data, pos);
    if (data) {
        rc = ble_gap_ext_adv_set_data(instance, data);
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to set ext adv data: %d", rc);
            return;
        }
    } else {
        ESP_LOGE(TAG, "Failed to allocate mbuf for adv data");
        return;
    }

    // Set scan response with device name
    uint8_t scan_rsp[31];
    uint8_t scan_pos = 0;

    // Complete Local Name
    scan_rsp[scan_pos++] = name_len;
    scan_rsp[scan_pos++] = 0x09; // Complete Local Name
    memcpy(&scan_rsp[scan_pos], adv_name, name_len);
    scan_pos += name_len;

    struct os_mbuf *scan_data = ble_hs_mbuf_from_flat(scan_rsp, scan_pos);
    if (scan_data) {
        rc = ble_gap_ext_adv_rsp_set_data(instance, scan_data);
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to set scan response: %d", rc);
        }
    }

    // Start advertising indefinitely
    rc = ble_gap_ext_adv_start(instance, 0, 0);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start ext adv: %d", rc);
    } else {
        ESP_LOGI(TAG, "Extended advertising started: %s v%d.%d.%d (build %d, type %d)", model_name,
                 LORACUE_VERSION_MAJOR, LORACUE_VERSION_MINOR, LORACUE_VERSION_PATCH, get_build_number(),
                 get_release_type());
    }
}

static void start_advertising_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(BLE_ADV_START_DELAY_MS));
    ble_advertise();
    vTaskDelete(NULL);
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
        ESP_LOGI(TAG, "Device address: %02x:%02x:%02x:%02x:%02x:%02x", addr[5], addr[4], addr[3], addr[2], addr[1],
                 addr[0]);
    }

    // Initialize BLE OTA handler (streaming task) now that NimBLE is ready
    rc = ble_ota_handler_init();
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init BLE OTA handler: %d", rc);
    } else {
        ESP_LOGI(TAG, "BLE OTA handler initialized");
    }

    // Start advertising from separate task (can't call ble_gap_adv_start from sync callback)
    xTaskCreate(start_advertising_task, "ble_adv", BLE_ADV_TASK_STACK_SIZE, NULL, BLE_ADV_TASK_PRIORITY, NULL);
}

static void on_reset(int reason)
{
    ESP_LOGE(TAG, "NimBLE host reset; reason=%d", reason);
}

//==============================================================================
// PUBLIC API
//==============================================================================

esp_err_t ble_init(void)
{
    if (s_ble_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing NimBLE BLE stack");

    // Create mutex if not already created
    if (!s_conn_state_mutex) {
        s_conn_state_mutex = xSemaphoreCreateMutex();
        if (!s_conn_state_mutex) {
            ESP_LOGE(TAG, "Failed to create mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    // Create command queue
    s_cmd_queue = xQueueCreate(BLE_CMD_QUEUE_SIZE, sizeof(ble_cmd_t));
    if (!s_cmd_queue) {
        ESP_LOGE(TAG, "Failed to create command queue");
        // Do not delete mutex here as it might be in use by other tasks
        return ESP_ERR_NO_MEM;
    }

    // Create command processing task
    BaseType_t ret =
        xTaskCreate(ble_cmd_task, "ble_cmd", BLE_CMD_TASK_STACK_SIZE, NULL, BLE_CMD_TASK_PRIORITY, &s_cmd_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create command task");
        vQueueDelete(s_cmd_queue);
        s_cmd_queue = NULL;
        // Do not delete mutex here
        return ESP_ERR_NO_MEM;
    }

    // Initialize NimBLE
    esp_err_t rc = nimble_port_init();
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble: %d", rc);
        vTaskDelete(s_cmd_task_handle);
        vQueueDelete(s_cmd_queue);
        s_cmd_queue = NULL;
        // Do not delete mutex here
        return rc;
    }

    // Initialize bonding storage
    ble_store_config_init();

    // Configure NimBLE
    ble_hs_cfg.sync_cb    = on_sync;
    ble_hs_cfg.reset_cb   = on_reset;
    ble_hs_cfg.sm_io_cap  = BLE_SM_IO_CAP_DISP_ONLY;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm    = 1;
    ble_hs_cfg.sm_sc      = 1;

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
        s_cmd_queue = NULL;
        vSemaphoreDelete(s_conn_state_mutex);
        s_conn_state_mutex = NULL;
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT services: %d", rc);
        nimble_port_deinit();
        vTaskDelete(s_cmd_task_handle);
        vQueueDelete(s_cmd_queue);
        s_cmd_queue = NULL;
        vSemaphoreDelete(s_conn_state_mutex);
        s_conn_state_mutex = NULL;
        return ESP_FAIL;
    }

    // Register BLE OTA services
    rc = ble_ota_register_services();
    if (rc != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register OTA services, continuing without OTA");
    }

    // Set device name from configuration (max 31 bytes for BLE)
    general_config_t config;
    general_config_get(&config);
    char gap_name[32];
    snprintf(gap_name, sizeof(gap_name), "LoRaCue %.22s", config.device_name);
    ble_svc_gap_device_name_set(gap_name);

    // Set DIS information
    const bsp_usb_config_t *usb_config = bsp_get_usb_config();
    char serial_number[32];
    bsp_get_serial_number(serial_number, sizeof(serial_number));

    ble_svc_dis_manufacturer_name_set("LoRaCue");
    ble_svc_dis_model_number_set(usb_config->usb_product);
    ble_svc_dis_serial_number_set(serial_number);
    ble_svc_dis_firmware_revision_set(LORACUE_VERSION_STRING);

    // Start NimBLE host task (OTA will be initialized in on_sync callback)
    nimble_port_freertos_init(nimble_host_task);

    s_ble_initialized = true;
    s_ble_enabled     = true;

    ESP_LOGI(TAG, "NimBLE initialized successfully");
    return ESP_OK;
}

esp_err_t ble_deinit(void)
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
    s_ble_enabled     = false;

    ESP_LOGI(TAG, "NimBLE deinitialized");
    return ESP_OK;
}

bool ble_is_enabled(void)
{
    return s_ble_enabled;
}

bool ble_is_connected(void)
{
    bool connected = false;
    if (conn_state_lock()) {
        connected = s_conn_state.connected;
        conn_state_unlock();
    }
    return connected;
}

static void ble_send_long_notification(uint16_t conn_handle, uint16_t attr_handle, const char *data, size_t len,
                                       uint16_t mtu)
{
    size_t chunk_size = mtu - 3; // ATT overhead
    size_t offset     = 0;

    while (offset < len) {
        size_t to_send = (len - offset > chunk_size) ? chunk_size : (len - offset);

        struct os_mbuf *om = ble_hs_mbuf_from_flat(data + offset, to_send);
        if (!om) {
            ESP_LOGE(TAG, "Failed to allocate mbuf");
            return;
        }

        int rc = ble_gattc_notify_custom(conn_handle, attr_handle, om);
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to send notification: %d", rc);
            return;
        }

        offset += to_send;

        if (offset < len) {
            vTaskDelay(pdMS_TO_TICKS(BLE_CHUNK_DELAY_MS)); // Small delay between chunks
        }
    }
}

void ble_send_response(const char *response)
{
    if (!response || strlen(response) == 0) {
        return;
    }

    bool can_send = false;
    uint16_t conn_handle;
    uint16_t mtu;

    if (conn_state_lock()) {
        can_send    = s_conn_state.connected && s_conn_state.notifications_enabled;
        conn_handle = s_conn_state.conn_handle;
        mtu         = s_conn_state.mtu;
        conn_state_unlock();
    }

    if (!can_send) {
        ESP_LOGW(TAG, "Cannot send: not connected or notifications disabled");
        return;
    }

    ble_send_long_notification(conn_handle, s_nus_tx_handle, response, strlen(response), mtu);
}

esp_err_t ble_set_enabled(bool enabled)
{
    if (enabled == s_ble_enabled) {
        return ESP_OK;
    }

    if (enabled) {
        return ble_init();
    } else {
        return ble_deinit();
    }
}

bool ble_get_passkey(uint32_t *passkey)
{
    bool active = false;

    if (conn_state_lock()) {
        active = s_conn_state.pairing_active;
        if (passkey && active) {
            *passkey = s_conn_state.passkey;
        }
        conn_state_unlock();
    }

    return active;
}
