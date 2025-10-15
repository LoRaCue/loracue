#include "bluetooth_config.h"
#include "general_config.h"
#include "esp_log.h"
#include "version.h"
#include <string.h>

#ifdef SIMULATOR_BUILD
// Simulator stubs - Bluetooth not available
static const char *TAG = "BT_CONFIG";

esp_err_t bluetooth_config_init(void)
{
    ESP_LOGI(TAG, "Bluetooth not available in simulator build");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bluetooth_config_set_enabled(bool enabled)
{
    return ESP_OK;
}

bool bluetooth_config_is_enabled(void)
{
    return false;
}

bool bluetooth_config_is_connected(void)
{
    return false;
}

bool bluetooth_config_get_passkey(const uint32_t *passkey)
{
    (void)passkey;
    return false;
}

#else
// Hardware implementation
#include "commands.h"
#include "ble_ota.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "BT_CONFIG";

// Nordic UART Service UUIDs (128-bit)
// Base UUID: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
static const uint8_t UART_SERVICE_UUID[16] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E
};
static const uint8_t UART_TX_CHAR_UUID[16] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E
};
static const uint8_t UART_RX_CHAR_UUID[16] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E
};

#define GATTS_APP_ID            0
#define GATTS_OTA_APP_ID        1
#define GATTS_NUM_HANDLE        8
#define GATTS_OTA_NUM_HANDLE    10

static bool ble_enabled         = false;
static bool ble_connected       = false;
static uint16_t conn_id         = 0;
static uint16_t gatts_if_global = 0;
static uint16_t tx_char_handle  = 0;
static uint16_t rx_char_handle  = 0;

// OTA service handles (non-static for ble_ota.c access)
uint16_t ota_service_handle = 0;
uint16_t ota_control_handle = 0;
uint16_t ota_data_handle = 0;
uint16_t ota_progress_handle = 0;
esp_gatt_if_t ota_gatts_if = 0;

// Pairing state
static bool pairing_in_progress = false;
static uint32_t pairing_passkey = 0;

// Command buffer
static char rx_buffer[16384];
static size_t rx_len = 0;

// Service handles
static uint16_t service_handle = 0;

static void send_response(const char *response)
{
    if (!ble_connected || tx_char_handle == 0) {
        return;
    }

    size_t len = strlen(response);
    esp_ble_gatts_send_indicate(gatts_if_global, conn_id, tx_char_handle, len, (uint8_t *)response, false);
    
    // Send newline
    esp_ble_gatts_send_indicate(gatts_if_global, conn_id, tx_char_handle, 1, (uint8_t *)"\n", false);
}

static void process_command(const char *command_line)
{
    ESP_LOGI(TAG, "Processing command: %s", command_line);
    commands_execute(command_line, send_response);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
            .adv_int_min       = 0x20,
            .adv_int_max       = 0x40,
            .adv_type          = ADV_TYPE_IND,
            .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
            .channel_map       = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        });
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Advertising started");
        } else {
            ESP_LOGE(TAG, "Advertising start failed: status %d", param->adv_start_cmpl.status);
        }
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Advertising stopped");
        } else {
            ESP_LOGE(TAG, "Advertising stop failed: status %d", param->adv_stop_cmpl.status);
        }
        break;

    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
        pairing_passkey      = param->ble_security.key_notif.passkey;
        pairing_in_progress  = true;
        ESP_LOGI(TAG, "Pairing passkey: %06" PRIu32, pairing_passkey);
        break;

    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        pairing_in_progress = false;
        if (param->ble_security.auth_cmpl.success) {
            ESP_LOGI(TAG, "Pairing successful");
        } else {
            ESP_LOGW(TAG, "Pairing failed");
        }
        break;

    default:
        break;
    }
}

static void ota_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ota_gatts_if = gatts_if;
        
        // Create OTA service (128-bit UUID)
        esp_gatt_srvc_id_t ota_service_id = {
            .is_primary = true,
            .id.inst_id = 0x00,
            .id.uuid.len = ESP_UUID_LEN_128,
        };
        memcpy(ota_service_id.id.uuid.uuid.uuid128, OTA_SERVICE_UUID, 16);
        esp_ble_gatts_create_service(gatts_if, &ota_service_id, GATTS_OTA_NUM_HANDLE);
        break;

    case ESP_GATTS_CREATE_EVT:
        ota_service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(ota_service_handle);

        // Add Control characteristic (Write + Indicate)
        esp_bt_uuid_t control_uuid = {
            .len = ESP_UUID_LEN_128,
        };
        memcpy(control_uuid.uuid.uuid128, OTA_CONTROL_CHAR_UUID, 16);
        esp_ble_gatts_add_char(ota_service_handle, &control_uuid,
                               ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
                               ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_INDICATE,
                               NULL, NULL);
        break;

    case ESP_GATTS_ADD_CHAR_EVT:
        if (memcmp(param->add_char.char_uuid.uuid.uuid128, OTA_CONTROL_CHAR_UUID, 16) == 0) {
            ota_control_handle = param->add_char.attr_handle;
            
            // Add Data characteristic (Write without response)
            esp_bt_uuid_t data_uuid = {
                .len = ESP_UUID_LEN_128,
            };
            memcpy(data_uuid.uuid.uuid128, OTA_DATA_CHAR_UUID, 16);
            esp_ble_gatts_add_char(ota_service_handle, &data_uuid,
                                   ESP_GATT_PERM_WRITE_ENCRYPTED,
                                   ESP_GATT_CHAR_PROP_BIT_WRITE_NR,
                                   NULL, NULL);
        } else if (memcmp(param->add_char.char_uuid.uuid.uuid128, OTA_DATA_CHAR_UUID, 16) == 0) {
            ota_data_handle = param->add_char.attr_handle;
            
            // Add Progress characteristic (Read + Notify)
            esp_bt_uuid_t progress_uuid = {
                .len = ESP_UUID_LEN_128,
            };
            memcpy(progress_uuid.uuid.uuid128, OTA_PROGRESS_CHAR_UUID, 16);
            esp_ble_gatts_add_char(ota_service_handle, &progress_uuid,
                                   ESP_GATT_PERM_READ_ENCRYPTED,
                                   ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                   NULL, NULL);
        } else if (memcmp(param->add_char.char_uuid.uuid.uuid128, OTA_PROGRESS_CHAR_UUID, 16) == 0) {
            ota_progress_handle = param->add_char.attr_handle;
            ESP_LOGI(TAG, "OTA service ready (UUID: 49589A79-7CC5-465D-BFF1-FE37C5065000)");
        }
        break;

    case ESP_GATTS_WRITE_EVT:
        if (param->write.handle == ota_control_handle) {
            ble_ota_handle_control_write(param->write.value, param->write.len);
        } else if (param->write.handle == ota_data_handle) {
            ble_ota_handle_data_write(param->write.value, param->write.len);
        }
        break;

    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "UART GATT app registered (app_id=%d, status=%d, gatts_if=%d)", 
                 param->reg.app_id, param->reg.status, gatts_if);
        gatts_if_global = gatts_if;
        
        // Set device name
        general_config_t config;
        general_config_get(&config);
        char ble_name[128];  // "LoRaCue-" (8) + device_name (31) + "-v" (2) + version (up to 80) + null (1)
        snprintf(ble_name, sizeof(ble_name), "LoRaCue-%s-v%s", config.device_name, LORACUE_VERSION_STRING);
        esp_ble_gap_set_device_name(ble_name);

        // Configure advertising data
        esp_ble_adv_data_t adv_data = {
            .set_scan_rsp        = false,
            .include_name        = true,
            .include_txpower     = true,
            .min_interval        = 0x0006,
            .max_interval        = 0x0010,
            .appearance          = 0x00,
            .manufacturer_len    = 0,
            .p_manufacturer_data = NULL,
            .service_data_len    = 0,
            .p_service_data      = NULL,
            .service_uuid_len    = 0,
            .p_service_uuid      = NULL,
            .flag                = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
        };
        esp_ble_gap_config_adv_data(&adv_data);

        ESP_LOGI(TAG, "Creating Nordic UART service (UUID: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E)...");
        // Create UART service
        esp_gatt_srvc_id_t service_id = {
            .is_primary = true,
            .id.inst_id = 0x00,
            .id.uuid.len = ESP_UUID_LEN_128,
        };
        memcpy(service_id.id.uuid.uuid.uuid128, UART_SERVICE_UUID, 16);
        esp_ble_gatts_create_service(gatts_if, &service_id, GATTS_NUM_HANDLE);
        break;

    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "UART service created (handle=%d, status=%d)", 
                 param->create.service_handle, param->create.status);
        service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(service_handle);

        // Add TX characteristic (notify)
        esp_bt_uuid_t tx_uuid = {
            .len = ESP_UUID_LEN_128,
        };
        memcpy(tx_uuid.uuid.uuid128, UART_TX_CHAR_UUID, 16);
        esp_ble_gatts_add_char(service_handle, &tx_uuid,
                               ESP_GATT_PERM_READ,
                               ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                               NULL, NULL);
        break;

    case ESP_GATTS_ADD_CHAR_EVT:
        if (memcmp(param->add_char.char_uuid.uuid.uuid128, UART_TX_CHAR_UUID, 16) == 0) {
            tx_char_handle = param->add_char.attr_handle;
            
            // Add RX characteristic (write)
            esp_bt_uuid_t rx_uuid = {
                .len = ESP_UUID_LEN_128,
            };
            memcpy(rx_uuid.uuid.uuid128, UART_RX_CHAR_UUID, 16);
            esp_ble_gatts_add_char(service_handle, &rx_uuid,
                                   ESP_GATT_PERM_WRITE,
                                   ESP_GATT_CHAR_PROP_BIT_WRITE,
                                   NULL, NULL);
        } else if (memcmp(param->add_char.char_uuid.uuid.uuid128, UART_RX_CHAR_UUID, 16) == 0) {
            rx_char_handle = param->add_char.attr_handle;
            ESP_LOGI(TAG, "UART service ready");
        }
        break;

    case ESP_GATTS_CONNECT_EVT:
        conn_id       = param->connect.conn_id;
        ble_connected = true;
        ble_ota_set_connection(conn_id);
        ESP_LOGI(TAG, "Client connected");
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ble_connected = false;
        conn_id = 0;
        rx_len = 0;
        ble_ota_handle_disconnect();
        ESP_LOGI(TAG, "Client disconnected, restarting advertising");
        esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
            .adv_int_min       = 0x20,
            .adv_int_max       = 0x40,
            .adv_type          = ADV_TYPE_IND,
            .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
            .channel_map       = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        });
        break;

    case ESP_GATTS_WRITE_EVT:
        if (param->write.handle == rx_char_handle) {
            // Check for OTA commands - reject and direct to OTA service
            if (param->write.len >= 17 && strncmp((char*)param->write.value, "FIRMWARE_UPGRADE ", 17) == 0) {
                const char *error_msg = "ERROR Use dedicated OTA GATT service (UUID 49589A79-7CC5-465D-BFF1-FE37C5065000) for firmware upgrades\n";
                esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, tx_char_handle, 
                                           strlen(error_msg), (uint8_t*)error_msg, false);
            } else {
                // Process as text command
                for (int i = 0; i < param->write.len; i++) {
                    char c = param->write.value[i];
                    
                    if (c == '\n' || c == '\r') {
                        if (rx_len > 0) {
                            rx_buffer[rx_len] = '\0';
                            process_command(rx_buffer);
                            rx_len = 0;
                        }
                    } else if (rx_len < sizeof(rx_buffer) - 1) {
                        rx_buffer[rx_len++] = c;
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}

esp_err_t bluetooth_config_init(void)
{
    // TODO: Migrate to BLE 5.0 Extended Advertising API
    // Currently using BLE 4.2 legacy advertising (CONFIG_BT_BLE_42_ADV_EN)
    // See: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_gap_ble.html
    
    ESP_LOGI(TAG, "=== Bluetooth Initialization Starting ===");
    
    // Check if Bluetooth should be enabled
    general_config_t config;
    general_config_get(&config);
    
    if (!config.bluetooth_enabled) {
        ESP_LOGI(TAG, "Bluetooth disabled in config");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Releasing Classic BT memory...");
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    ESP_LOGI(TAG, "Initializing BT controller...");
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Enabling BLE mode...");
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Initializing Bluedroid stack...");
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Enabling Bluedroid stack...");
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Configuring BLE security (passkey display)...");
    // Set security with passkey display
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    ESP_LOGI(TAG, "Registering GAP and GATTS callbacks...");
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    
    ESP_LOGI(TAG, "Registering UART GATT app (ID=%d)...", GATTS_APP_ID);
    esp_ble_gatts_app_register(GATTS_APP_ID);
    
    ESP_LOGI(TAG, "Registering OTA GATT app (ID=%d)...", GATTS_OTA_APP_ID);
    // Register OTA GATT app
    esp_ble_gatts_register_callback(ota_gatts_event_handler);
    esp_ble_gatts_app_register(GATTS_OTA_APP_ID);

    ESP_LOGI(TAG, "Setting local MTU to 500 bytes...");
    esp_ble_gatt_set_local_mtu(500);

    ESP_LOGI(TAG, "Initializing OTA service...");
    ble_ota_service_init(0);  // Will be set in OTA event handler
    ble_enabled = true;
    
    ESP_LOGI(TAG, "=== Bluetooth Initialization Complete ===");
    ESP_LOGI(TAG, "Waiting for GATT app registration callbacks...");

    return ESP_OK;
}

esp_err_t bluetooth_config_set_enabled(bool enabled)
{
    general_config_t config;
    general_config_get(&config);
    config.bluetooth_enabled = enabled;
    return general_config_set(&config);
}

bool bluetooth_config_is_enabled(void)
{
    return ble_enabled;
}

bool bluetooth_config_is_connected(void)
{
    return ble_connected;
}

bool bluetooth_config_get_passkey(uint32_t *passkey)
{
    if (pairing_in_progress && passkey) {
        *passkey = pairing_passkey;
        return true;
    }
    return false;
}

#endif // SIMULATOR_BUILD
