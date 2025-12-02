#include "commands.h"
#include "bsp.h"
#include "cJSON.h"
#include "commands_api.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "power_mgmt.h"
#include "version.h"
#include <string.h>

// JSON-RPC 2.0 error codes
#define JSONRPC_PARSE_ERROR -32700
#define JSONRPC_INVALID_REQUEST -32600
#define JSONRPC_METHOD_NOT_FOUND -32601
#define JSONRPC_INVALID_PARAMS -32602
#define JSONRPC_INTERNAL_ERROR -32603

#define BYTES_PER_MB (1024 * 1024)
#define BYTES_PER_KB 1024
#define US_PER_SEC 1000000
#define SLOT_ID_MIN 1
#define SLOT_ID_MAX 16
#define MAX_COMMAND_LENGTH 8192
#define RESET_DELAY_MS 500

static response_fn_t s_send_response = NULL;
static cJSON *s_request_id           = NULL;

static void send_jsonrpc_result(cJSON *result)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddItemToObject(response, "result", result);
    if (s_request_id) {
        cJSON_AddItemToObject(response, "id", cJSON_Duplicate(s_request_id, 1));
    }

    char *json_str = cJSON_PrintUnformatted(response);
    if (json_str) {
        s_send_response(json_str);
        free(json_str);
    }
    cJSON_Delete(response);
}

static void send_jsonrpc_error(int code, const char *message)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");

    cJSON *error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);
    cJSON_AddItemToObject(response, "error", error);

    if (s_request_id) {
        cJSON_AddItemToObject(response, "id", cJSON_Duplicate(s_request_id, 1));
    } else {
        cJSON_AddNullToObject(response, "id");
    }

    char *json_str = cJSON_PrintUnformatted(response);
    if (json_str) {
        s_send_response(json_str);
        free(json_str);
    }
    cJSON_Delete(response);
}

static void handle_ping(void)
{
    send_jsonrpc_result(cJSON_CreateString("pong"));
}

static void handle_get_device_info(void)
{
    cJSON *response = cJSON_CreateObject();

    cJSON_AddStringToObject(response, "model", bsp_get_model_name());
    cJSON_AddStringToObject(response, "board_id", bsp_get_board_id());
    cJSON_AddStringToObject(response, "version", LORACUE_VERSION_STRING);
    cJSON_AddStringToObject(response, "commit", LORACUE_BUILD_COMMIT_SHORT);
    cJSON_AddStringToObject(response, "branch", LORACUE_BUILD_BRANCH);
    cJSON_AddStringToObject(response, "build_date", LORACUE_BUILD_DATE);

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(response, "chip_model", CONFIG_IDF_TARGET);
    cJSON_AddNumberToObject(response, "chip_revision", chip_info.revision);
    cJSON_AddNumberToObject(response, "cpu_cores", chip_info.cores);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    cJSON_AddNumberToObject(response, "flash_size_mb", flash_size / BYTES_PER_MB);

    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(response, "mac", mac_str);

    cJSON_AddNumberToObject(response, "uptime_sec", esp_timer_get_time() / US_PER_SEC);
    cJSON_AddNumberToObject(response, "free_heap_kb", esp_get_free_heap_size() / BYTES_PER_KB);

    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running) {
        cJSON_AddStringToObject(response, "partition", running->label);
    }

    send_jsonrpc_result(response);
}

static void handle_get_general(void)
{
    general_config_t config;
    if (cmd_get_general_config(&config) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to get config");
        return;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "name", config.device_name);
    cJSON_AddStringToObject(response, "mode", device_mode_to_string(config.device_mode));
    cJSON_AddNumberToObject(response, "contrast", config.display_contrast);
    cJSON_AddBoolToObject(response, "bluetooth", config.bluetooth_enabled);
    cJSON_AddBoolToObject(response, "bluetooth_pairing", config.bluetooth_pairing_enabled);
    cJSON_AddNumberToObject(response, "slot_id", config.slot_id);

    send_jsonrpc_result(response);
}

static void handle_set_general(cJSON *config_json)
{
    general_config_t config;
    if (cmd_get_general_config(&config) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to get current config");
        return;
    }

    cJSON *name = cJSON_GetObjectItem(config_json, "name");
    if (name && cJSON_IsString(name)) {
        strncpy(config.device_name, name->valuestring, sizeof(config.device_name) - 1);
        config.device_name[sizeof(config.device_name) - 1] = '\0';
    }

    cJSON *mode = cJSON_GetObjectItem(config_json, "mode");
    if (mode && cJSON_IsString(mode)) {
        if (strcmp(mode->valuestring, "PRESENTER") == 0) {
            config.device_mode = DEVICE_MODE_PRESENTER;
        } else if (strcmp(mode->valuestring, "PC") == 0) {
            config.device_mode = DEVICE_MODE_PC;
        } else {
            send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Invalid mode");
            return;
        }
    }

    cJSON *contrast = cJSON_GetObjectItem(config_json, "contrast");
    if (contrast && cJSON_IsNumber(contrast)) {
        config.display_contrast = (uint8_t)contrast->valueint;
    }

    cJSON *bluetooth = cJSON_GetObjectItem(config_json, "bluetooth");
    if (bluetooth && cJSON_IsBool(bluetooth)) {
        config.bluetooth_enabled = cJSON_IsTrue(bluetooth);
    }

    cJSON *bluetooth_pairing = cJSON_GetObjectItem(config_json, "bluetooth_pairing");
    if (bluetooth_pairing && cJSON_IsBool(bluetooth_pairing)) {
        config.bluetooth_pairing_enabled = cJSON_IsTrue(bluetooth_pairing);
    }

    cJSON *slot_id = cJSON_GetObjectItem(config_json, "slot_id");
    if (slot_id && cJSON_IsNumber(slot_id)) {
        int slot = slot_id->valueint;
        if (slot < SLOT_ID_MIN || slot > SLOT_ID_MAX) {
            send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Invalid slot_id (1-16)");
            return;
        }
        config.slot_id = slot;
    }

    if (cmd_set_general_config(&config) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to save config");
        return;
    }

    send_jsonrpc_result(cJSON_CreateString("Config updated"));
}

static void handle_get_power_management(void)
{
    power_mgmt_config_t config;
    if (cmd_get_power_config(&config) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to get power config");
        return;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "display_sleep_enabled", config.display_sleep_enabled);
    cJSON_AddNumberToObject(response, "display_sleep_timeout_ms", config.display_sleep_timeout_ms);
    cJSON_AddBoolToObject(response, "light_sleep_enabled", config.light_sleep_enabled);
    cJSON_AddNumberToObject(response, "light_sleep_timeout_ms", config.light_sleep_timeout_ms);
    cJSON_AddBoolToObject(response, "deep_sleep_enabled", config.deep_sleep_enabled);
    cJSON_AddNumberToObject(response, "deep_sleep_timeout_ms", config.deep_sleep_timeout_ms);

    send_jsonrpc_result(response);
}

static void handle_set_power_management(cJSON *config_json)
{
    power_mgmt_config_t config;
    if (cmd_get_power_config(&config) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to get current power config");
        return;
    }

    cJSON *item;
    if ((item = cJSON_GetObjectItem(config_json, "display_sleep_enabled")) && cJSON_IsBool(item))
        config.display_sleep_enabled = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(config_json, "display_sleep_timeout_ms")) && cJSON_IsNumber(item))
        config.display_sleep_timeout_ms = item->valueint;
    if ((item = cJSON_GetObjectItem(config_json, "light_sleep_enabled")) && cJSON_IsBool(item))
        config.light_sleep_enabled = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(config_json, "light_sleep_timeout_ms")) && cJSON_IsNumber(item))
        config.light_sleep_timeout_ms = item->valueint;
    if ((item = cJSON_GetObjectItem(config_json, "deep_sleep_enabled")) && cJSON_IsBool(item))
        config.deep_sleep_enabled = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(config_json, "deep_sleep_timeout_ms")) && cJSON_IsNumber(item))
        config.deep_sleep_timeout_ms = item->valueint;

    if (cmd_set_power_config(&config) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to save power config");
        return;
    }

    send_jsonrpc_result(cJSON_CreateString("Power config updated"));
}

static void handle_get_lora_config(void)
{
    lora_config_t config;
    if (cmd_get_lora_config(&config) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to get LoRa config");
        return;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "band_id", config.band_id);
    cJSON_AddNumberToObject(response, "frequency_khz", config.frequency / 1000);
    cJSON_AddNumberToObject(response, "spreading_factor", config.spreading_factor);
    cJSON_AddNumberToObject(response, "bandwidth_khz", config.bandwidth);
    cJSON_AddNumberToObject(response, "coding_rate", config.coding_rate);
    cJSON_AddNumberToObject(response, "tx_power_dbm", config.tx_power);

    send_jsonrpc_result(response);
}

static void handle_set_lora_config(cJSON *config_json)
{
    lora_config_t config;
    if (cmd_get_lora_config(&config) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to get current LoRa config");
        return;
    }

    cJSON *freq    = cJSON_GetObjectItem(config_json, "frequency_khz");
    cJSON *sf      = cJSON_GetObjectItem(config_json, "spreading_factor");
    cJSON *bw      = cJSON_GetObjectItem(config_json, "bandwidth_khz");
    cJSON *cr      = cJSON_GetObjectItem(config_json, "coding_rate");
    cJSON *power   = cJSON_GetObjectItem(config_json, "tx_power_dbm");
    cJSON *band_id = cJSON_GetObjectItem(config_json, "band_id");

    if (cJSON_IsNumber(bw))
        config.bandwidth = bw->valueint;
    if (cJSON_IsNumber(freq))
        config.frequency = freq->valueint * 1000;
    if (cJSON_IsNumber(sf))
        config.spreading_factor = sf->valueint;
    if (cJSON_IsNumber(cr))
        config.coding_rate = cr->valueint;
    if (cJSON_IsString(band_id)) {
        strncpy(config.band_id, band_id->valuestring, sizeof(config.band_id) - 1);
        config.band_id[sizeof(config.band_id) - 1] = '\0';
    }
    if (cJSON_IsNumber(power))
        config.tx_power = power->valueint;

    esp_err_t ret = cmd_set_lora_config(&config);
    if (ret == ESP_ERR_INVALID_ARG) {
        send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Invalid LoRa parameters");
        return;
    } else if (ret != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to save LoRa config");
        return;
    }

    send_jsonrpc_result(cJSON_CreateString("LoRa config updated"));
}

static void handle_get_lora_key(void)
{
    lora_config_t config;
    if (cmd_get_lora_config(&config) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to get LoRa config");
        return;
    }

    char hex_key[65];
    for (int i = 0; i < 32; i++) {
        snprintf(&hex_key[i * 2], 3, "%02x", config.aes_key[i]);
    }
    hex_key[64] = '\0';

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "aes_key", hex_key);
    send_jsonrpc_result(response);
}

static void handle_set_lora_key(cJSON *json)
{
    cJSON *aes_key_json = cJSON_GetObjectItem(json, "aes_key");
    if (!aes_key_json || !cJSON_IsString(aes_key_json) || strlen(aes_key_json->valuestring) != 64) {
        send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Invalid aes_key (must be 64 hex chars)");
        return;
    }

    uint8_t key_bytes[32];
    const char *hex_key = aes_key_json->valuestring;
    for (int i = 0; i < 32; i++) {
        const char byte_str[3] = {hex_key[i * 2], hex_key[i * 2 + 1], '\0'};
        key_bytes[i]           = (uint8_t)strtol(byte_str, NULL, 16);
    }

    if (cmd_set_lora_key(key_bytes) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to set key");
        return;
    }

    send_jsonrpc_result(cJSON_CreateString("Key updated"));
}

static void handle_get_paired_devices(void)
{
    paired_device_t devices[SLOT_ID_MAX]; // Assume max 16
    size_t count = 0;

    // Using arbitrary max limit that should cover expected MAX_PAIRED_DEVICES
    if (cmd_get_paired_devices(devices, SLOT_ID_MAX, &count) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to get devices");
        return;
    }

    cJSON *array = cJSON_CreateArray();
    for (size_t i = 0; i < count; i++) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "name", devices[i].device_name);

        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", devices[i].mac_address[0],
                 devices[i].mac_address[1], devices[i].mac_address[2], devices[i].mac_address[3],
                 devices[i].mac_address[4], devices[i].mac_address[5]);
        cJSON_AddStringToObject(obj, "mac", mac_str);

        cJSON_AddItemToArray(array, obj);
    }
    send_jsonrpc_result(array);
}

static void handle_pair_device(cJSON *params)
{
    cJSON *name      = cJSON_GetObjectItem(params, "name");
    const cJSON *mac = cJSON_GetObjectItem(params, "mac");
    const cJSON *key = cJSON_GetObjectItem(params, "aes_key");

    if (!name || !mac || !key) {
        send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Missing params");
        return;
    }

    uint8_t mac_bytes[6];
    if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac_bytes[0], &mac_bytes[1],
               &mac_bytes[2], &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
        send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Invalid MAC");
        return;
    }

    uint8_t key_bytes[32];
    const char *k = key->valuestring;
    if (strlen(k) != 64) {
        send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Invalid Key Length");
        return;
    }
    for (int i = 0; i < 32; i++) {
        if (sscanf(k + i * 2, "%02hhx", &key_bytes[i]) != 1) {
            send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Invalid Key Hex");
            return;
        }
    }

    if (cmd_pair_device(name->valuestring, mac_bytes, key_bytes) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Pairing failed");
        return;
    }

    send_jsonrpc_result(cJSON_CreateString("Paired"));
}

static void handle_unpair_device(cJSON *params)
{
    const cJSON *mac = cJSON_GetObjectItem(params, "mac");
    if (!mac) {
        send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Missing MAC");
        return;
    }

    uint8_t mac_bytes[6];
    if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac_bytes[0], &mac_bytes[1],
               &mac_bytes[2], &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
        send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Invalid MAC");
        return;
    }

    if (cmd_unpair_device(mac_bytes) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Unpair failed");
        return;
    }
    send_jsonrpc_result(cJSON_CreateString("Unpaired"));
}

static void handle_device_reset(void)
{
    send_jsonrpc_result(cJSON_CreateString("Reset initiated"));
    // Allow time to send response
    vTaskDelay(pdMS_TO_TICKS(RESET_DELAY_MS));
    cmd_factory_reset();
}

static void handle_firmware_start(cJSON *params)
{
    cJSON *size = cJSON_GetObjectItem(params, "size");
    cJSON *sha  = cJSON_GetObjectItem(params, "sha256");
    cJSON *sig  = cJSON_GetObjectItem(params, "signature");

    if (!size || !sha || !sig) {
        send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Missing params");
        return;
    }

    esp_err_t ret = cmd_firmware_upgrade_start((size_t)size->valueint, sha->valuestring, sig->valuestring);
    if (ret != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to start OTA");
        return;
    }

    cJSON *res = cJSON_CreateObject();
    cJSON_AddStringToObject(res, "status", "ready");
    cJSON_AddNumberToObject(res, "size", size->valueint);
    send_jsonrpc_result(res);
}

// Dispatch table
typedef void (*method_handler_no_params_t)(void);
typedef void (*method_handler_with_params_t)(cJSON *params);

typedef struct {
    const char *method;
    bool has_params;
    union {
        method_handler_no_params_t no_params;
        method_handler_with_params_t with_params;
    } handler;
} jsonrpc_method_t;

static const jsonrpc_method_t method_table[] = {
    {"ping", false, {.no_params = handle_ping}},
    {"device:info", false, {.no_params = handle_get_device_info}},
    {"general:get", false, {.no_params = handle_get_general}},
    {"general:set", true, {.with_params = handle_set_general}},
    {"power:get", false, {.no_params = handle_get_power_management}},
    {"power:set", true, {.with_params = handle_set_power_management}},
    {"lora:get", false, {.no_params = handle_get_lora_config}},
    {"lora:set", true, {.with_params = handle_set_lora_config}},
    {"lora:key:get", false, {.no_params = handle_get_lora_key}},
    {"lora:key:set", true, {.with_params = handle_set_lora_key}},
    {"paired:list", false, {.no_params = handle_get_paired_devices}},
    {"paired:pair", true, {.with_params = handle_pair_device}},
    {"paired:unpair", true, {.with_params = handle_unpair_device}},
    {"device:reset", false, {.no_params = handle_device_reset}},
    {"firmware:upgrade", true, {.with_params = handle_firmware_start}},
};

void commands_execute(const char *command_line, response_fn_t send_response)
{
    if (!send_response)
        return;
    s_send_response = send_response;
    s_request_id    = NULL;

    power_mgmt_update_activity();

    if (strlen(command_line) > MAX_COMMAND_LENGTH) {
        send_jsonrpc_error(JSONRPC_INVALID_REQUEST, "Request too large");
        return;
    }

    cJSON *request = cJSON_Parse(command_line);
    if (!request) {
        send_jsonrpc_error(JSONRPC_PARSE_ERROR, "Invalid JSON");
        return;
    }

    cJSON *method = cJSON_GetObjectItem(request, "method");
    if (!method || !cJSON_IsString(method)) {
        send_jsonrpc_error(JSONRPC_INVALID_REQUEST, "Invalid method");
        cJSON_Delete(request);
        return;
    }

    s_request_id  = cJSON_GetObjectItem(request, "id");
    cJSON *params = cJSON_GetObjectItem(request, "params");

    bool found = false;
    for (size_t i = 0; i < sizeof(method_table) / sizeof(method_table[0]); i++) {
        if (strcmp(method->valuestring, method_table[i].method) == 0) {
            if (method_table[i].has_params) {
                if (!params) {
                    send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Missing params");
                } else {
                    method_table[i].handler.with_params(params);
                }
            } else {
                method_table[i].handler.no_params();
            }
            found = true;
            break;
        }
    }

    if (!found) {
        send_jsonrpc_error(JSONRPC_METHOD_NOT_FOUND, "Method not found");
    }

    cJSON_Delete(request);
}
