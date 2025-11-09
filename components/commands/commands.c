#include "commands.h"
#include "bluetooth_config.h"
#include "bsp.h"
#include "cJSON.h"
#include "device_registry.h"
#include "esp_app_format.h"
#include "esp_chip_info.h"
#include "esp_crc.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "general_config.h"
#include "lora_bands.h"
#include "lora_driver.h"
#include "system_events.h"
#include "mbedtls/base64.h"
#include "nvs_flash.h"
#include "ota_engine.h"
#include "power_mgmt.h"
#include "power_mgmt_config.h"
#include "tusb.h"
#include "u8g2.h"
#include "version.h"
#include <inttypes.h>
#include <string.h>

static const char *TAG = "COMMANDS";

// Security limits
#define MAX_COMMAND_LENGTH 8192
#define MAX_FIRMWARE_SIZE (4 * 1024 * 1024)

// JSON-RPC 2.0 error codes
#define JSONRPC_PARSE_ERROR      -32700
#define JSONRPC_INVALID_REQUEST  -32600
#define JSONRPC_METHOD_NOT_FOUND -32601
#define JSONRPC_INVALID_PARAMS   -32602
#define JSONRPC_INTERNAL_ERROR   -32603

// Response callback and request ID
static response_fn_t s_send_response = NULL;
static cJSON *s_request_id = NULL;

// JSON-RPC 2.0 response helpers
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

    // Model first
    cJSON_AddStringToObject(response, "model", "LC-Alpha"); // TODO: Get from NVS or build config

    // Hardware info
    cJSON_AddStringToObject(response, "board_id", bsp_get_board_id());

    // Firmware info
    cJSON_AddStringToObject(response, "version", LORACUE_VERSION_STRING);
    cJSON_AddStringToObject(response, "commit", LORACUE_BUILD_COMMIT_SHORT);
    cJSON_AddStringToObject(response, "branch", LORACUE_BUILD_BRANCH);
    cJSON_AddStringToObject(response, "build_date", LORACUE_BUILD_DATE);

    // Chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(response, "chip_model", CONFIG_IDF_TARGET);
    cJSON_AddNumberToObject(response, "chip_revision", chip_info.revision);
    cJSON_AddNumberToObject(response, "cpu_cores", chip_info.cores);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    cJSON_AddNumberToObject(response, "flash_size_mb", flash_size / (1024 * 1024));

    // MAC address
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(response, "mac", mac_str);

    // Runtime info
    cJSON_AddNumberToObject(response, "uptime_sec", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(response, "free_heap_kb", esp_get_free_heap_size() / 1024);

    // Partition info
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running) {
        cJSON_AddStringToObject(response, "partition", running->label);
    }

    send_jsonrpc_result(response);
}

static void handle_get_general(void)
{
    general_config_t config;
    if (general_config_get(&config) != ESP_OK) {
        send_jsonrpc_error(JSONRPC_INTERNAL_ERROR, "Failed to get config");
        return;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "name", config.device_name);
    cJSON_AddStringToObject(response, "mode", device_mode_to_string(config.device_mode));
    cJSON_AddNumberToObject(response, "brightness", config.display_brightness);
    cJSON_AddBoolToObject(response, "bluetooth", config.bluetooth_enabled);
    cJSON_AddNumberToObject(response, "slot_id", config.slot_id);

    send_jsonrpc_result(response);
}

static void handle_set_general(cJSON *config_json)
{
    general_config_t config;
    esp_err_t ret = general_config_get(&config);
    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to get current device config");
        return;
    }

    cJSON *name = cJSON_GetObjectItem(config_json, "name");
    if (name && cJSON_IsString(name)) {
        strncpy(config.device_name, name->valuestring, sizeof(config.device_name) - 1);
        config.device_name[sizeof(config.device_name) - 1] = '\0';
    }

    cJSON *mode = cJSON_GetObjectItem(config_json, "mode");
    if (mode && cJSON_IsString(mode)) {
        device_mode_t new_mode;
        if (strcmp(mode->valuestring, "PRESENTER") == 0) {
            new_mode = DEVICE_MODE_PRESENTER;
        } else if (strcmp(mode->valuestring, "PC") == 0) {
            new_mode = DEVICE_MODE_PC;
        } else {
            send_jsonrpc_error(-32602, "Invalid mode (must be PRESENTER or PC)");
            return;
        }
        config.device_mode = new_mode;
        system_events_post_mode_changed(new_mode);
    }

    cJSON *brightness = cJSON_GetObjectItem(config_json, "brightness");
    if (brightness && cJSON_IsNumber(brightness)) {
        config.display_brightness = brightness->valueint;
        extern u8g2_t u8g2;
        u8g2_SetContrast(&u8g2, config.display_brightness);
    }

    cJSON *bluetooth = cJSON_GetObjectItem(config_json, "bluetooth");
    if (bluetooth && cJSON_IsBool(bluetooth)) {
        config.bluetooth_enabled = cJSON_IsTrue(bluetooth);
        bluetooth_config_set_enabled(config.bluetooth_enabled);
    }

    cJSON *slot_id = cJSON_GetObjectItem(config_json, "slot_id");
    if (slot_id && cJSON_IsNumber(slot_id)) {
        int slot = slot_id->valueint;
        if (slot < 1 || slot > 16) {
            send_jsonrpc_error(-32602, "Invalid slot_id (must be 1-16)");
            return;
        }
        config.slot_id = slot;
    }

    ret = general_config_set(&config);
    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to save device config");
        return;
    }

    // Reload UI data provider to update device name display
    extern esp_err_t ui_data_provider_reload_config(void);
    ui_data_provider_reload_config();

    cJSON *response = cJSON_CreateString("Device config updated");
    send_jsonrpc_result(response);
}

static void handle_get_power_management(void)
{
    power_mgmt_config_t config;
    if (power_mgmt_config_get(&config) != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to get power config");
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
    if (power_mgmt_config_get(&config) != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to get current power config");
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

    if (power_mgmt_config_set(&config) != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to save power config");
        return;
    }

    cJSON *response = cJSON_CreateString("Power config updated - restart required");
    send_jsonrpc_result(response);
}

static void handle_get_lora_config(void)
{
    lora_config_t config;
    esp_err_t ret = lora_get_config(&config);

    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to get LoRa config");
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

static void handle_get_lora_key(void)
{
    lora_config_t config;
    esp_err_t ret = lora_get_config(&config);

    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to get LoRa config");
        return;
    }

    // Convert AES key to hex string
    char hex_key[65]; // 32 bytes * 2 + null terminator
    for (int i = 0; i < 32; i++) {
        sprintf(&hex_key[i * 2], "%02x", config.aes_key[i]);
    }
    hex_key[64] = '\0';

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "aes_key", hex_key);

    send_jsonrpc_result(response);
}

static void handle_set_lora_key(cJSON *json)
{
    cJSON *aes_key_json = cJSON_GetObjectItem(json, "aes_key");
    if (!aes_key_json || !cJSON_IsString(aes_key_json)) {
        send_jsonrpc_error(-32602, "Missing or invalid aes_key parameter");
        return;
    }

    const char *hex_key = aes_key_json->valuestring;
    if (strlen(hex_key) != 64) {
        send_jsonrpc_error(-32602, "aes_key must be 64 hex characters (32 bytes)");
        return;
    }

    // Parse hex string to bytes
    uint8_t key_bytes[32];
    for (int i = 0; i < 32; i++) {
        const char byte_str[3] = {hex_key[i * 2], hex_key[i * 2 + 1], '\0'};
        key_bytes[i]           = (uint8_t)strtol(byte_str, NULL, 16);
    }

    // Get current config and update key
    lora_config_t config;
    esp_err_t ret = lora_get_config(&config);
    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to get LoRa config");
        return;
    }

    memcpy(config.aes_key, key_bytes, 32);

    ret = lora_set_config(&config);
    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to save LoRa config");
        return;
    }

    cJSON *response = cJSON_CreateString("AES key updated");
    send_jsonrpc_result(response);
}

static void handle_set_lora_config(cJSON *config_json)
{
    lora_config_t config;
    esp_err_t ret = lora_get_config(&config);

    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to get current LoRa config");
        return;
    }

    cJSON *freq    = cJSON_GetObjectItem(config_json, "frequency_khz");
    cJSON *sf      = cJSON_GetObjectItem(config_json, "spreading_factor");
    cJSON *bw      = cJSON_GetObjectItem(config_json, "bandwidth_khz");
    cJSON *cr      = cJSON_GetObjectItem(config_json, "coding_rate");
    cJSON *power   = cJSON_GetObjectItem(config_json, "tx_power_dbm");
    cJSON *band_id = cJSON_GetObjectItem(config_json, "band_id");

    // Validate bandwidth (support all LoRa bandwidths in kHz)
    if (cJSON_IsNumber(bw)) {
        int bw_val = bw->valueint;
        // Accept: 7.81, 10.42, 15.63, 20.83, 31.25, 41.67, 62.5, 125, 250, 500 kHz
        // Stored as integers: 7, 10, 15, 20, 31, 41, 62, 125, 250, 500
        if (bw_val != 7 && bw_val != 10 && bw_val != 15 && bw_val != 20 &&
            bw_val != 31 && bw_val != 41 && bw_val != 62 &&
            bw_val != 125 && bw_val != 250 && bw_val != 500) {
            send_jsonrpc_error(-32602, "Invalid bandwidth. Must be 7, 10, 15, 20, 31, 41, 62, 125, 250, or 500 kHz");
            return;
        }
        config.bandwidth = bw_val;
    }

    // Validate and convert frequency
    if (cJSON_IsNumber(freq)) {
        int freq_khz = freq->valueint;
        if (freq_khz % 100 != 0) {
            send_jsonrpc_error(-32602, "Frequency must be divisible by 100 kHz");
            return;
        }
        config.frequency = freq_khz * 1000;  // Convert kHz to Hz
    }

    // Validate spreading factor
    if (cJSON_IsNumber(sf)) {
        int sf_val = sf->valueint;
        if (sf_val < 7 || sf_val > 12) {
            send_jsonrpc_error(-32602, "Spreading factor must be between 7 and 12");
            return;
        }
        config.spreading_factor = sf_val;
    }

    // Validate coding rate
    if (cJSON_IsNumber(cr)) {
        int cr_val = cr->valueint;
        if (cr_val < 5 || cr_val > 8) {
            send_jsonrpc_error(-32602, "Coding rate must be between 5 and 8 (4/5 to 4/8)");
            return;
        }
        config.coding_rate = cr_val;
    }

    if (cJSON_IsString(band_id)) {
        strncpy(config.band_id, band_id->valuestring, sizeof(config.band_id) - 1);
        config.band_id[sizeof(config.band_id) - 1] = '\0';
    }

    // Validate TX power (hardware limit only, band limit shown as warning in UI)
    if (cJSON_IsNumber(power)) {
        int power_val = power->valueint;
        if (power_val < 2 || power_val > 22) {
            send_jsonrpc_error(-32602, "TX power must be between 2 and 22 dBm");
            return;
        }
        config.tx_power = power_val;
    }

    // Validate frequency against band limits
    extern const lora_band_profile_t *lora_bands_get_profile_by_id(const char *band_id);
    const lora_band_profile_t *band = lora_bands_get_profile_by_id(config.band_id);
    if (band) {
        uint32_t freq_khz = config.frequency / 1000;
        if (freq_khz < band->optimal_freq_min_khz || freq_khz > band->optimal_freq_max_khz) {
            cJSON *error_data = cJSON_CreateObject();
            cJSON_AddStringToObject(error_data, "reason", "Frequency out of band range");
            cJSON_AddNumberToObject(error_data, "min_khz", band->optimal_freq_min_khz);
            cJSON_AddNumberToObject(error_data, "max_khz", band->optimal_freq_max_khz);
            char *error_str = cJSON_PrintUnformatted(error_data);
            send_jsonrpc_error(-32602, error_str);
            free(error_str);
            cJSON_Delete(error_data);
            return;
        }
    }

    ret = lora_set_config(&config);
    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to update LoRa configuration");
        return;
    }

    cJSON *response = cJSON_CreateString("LoRa configuration updated");
    send_jsonrpc_result(response);
}

static void handle_get_paired_devices(void)
{
    cJSON *devices_array = cJSON_CreateArray();

    // Use device_registry_list to get all paired devices
    paired_device_t devices[MAX_PAIRED_DEVICES];
    size_t count = 0;

    if (device_registry_list(devices, MAX_PAIRED_DEVICES, &count) == ESP_OK) {
        for (size_t i = 0; i < count; i++) {
            cJSON *device_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(device_obj, "name", devices[i].device_name);

            char mac_str[18];
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", devices[i].mac_address[0],
                     devices[i].mac_address[1], devices[i].mac_address[2], devices[i].mac_address[3],
                     devices[i].mac_address[4], devices[i].mac_address[5]);
            cJSON_AddStringToObject(device_obj, "mac", mac_str);

            char aes_key_str[65];
            for (int j = 0; j < 32; j++) {
                snprintf(aes_key_str + (j * 2), 3, "%02x", devices[i].aes_key[j]);
            }
            cJSON_AddStringToObject(device_obj, "aes_key", aes_key_str);

            cJSON_AddItemToArray(devices_array, device_obj);
        }
    }

    send_jsonrpc_result(devices_array);
}

static void handle_get_lora_bands(void)
{
    extern int lora_bands_get_count(void);
    extern const lora_band_profile_t *lora_bands_get_profile(int index);

    int band_count     = lora_bands_get_count();
    cJSON *bands_array = cJSON_CreateArray();

    for (int i = 0; i < band_count; i++) {
        const lora_band_profile_t *band = lora_bands_get_profile(i);
        if (band) {
            cJSON *band_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(band_obj, "id", band->id);
            cJSON_AddStringToObject(band_obj, "name", band->name);
            cJSON_AddNumberToObject(band_obj, "center_khz", band->optimal_center_khz);
            cJSON_AddNumberToObject(band_obj, "min_khz", band->optimal_freq_min_khz);
            cJSON_AddNumberToObject(band_obj, "max_khz", band->optimal_freq_max_khz);
            cJSON_AddNumberToObject(band_obj, "max_power_dbm", band->max_power_dbm);
            cJSON_AddItemToArray(bands_array, band_obj);
        }
    }

    send_jsonrpc_result(bands_array);
}

// Embedded preset JSON
extern const uint8_t lora_presets_json_start[] asm("_binary_lora_presets_json_start");
extern const uint8_t lora_presets_json_end[] asm("_binary_lora_presets_json_end");

static void handle_get_lora_presets(void)
{
    size_t json_len = lora_presets_json_end - lora_presets_json_start;
    char *json_str = malloc(json_len + 1);
    if (!json_str) {
        send_jsonrpc_error(-32603, "Memory allocation failed");
        return;
    }
    memcpy(json_str, lora_presets_json_start, json_len);
    json_str[json_len] = '\0';

    cJSON *presets = cJSON_Parse(json_str);
    free(json_str);

    if (presets) {
        send_jsonrpc_result(presets);
    } else {
        send_jsonrpc_error(-32603, "Failed to parse presets");
    }
}

static void handle_set_lora_preset(cJSON *params)
{
    cJSON *name = cJSON_GetObjectItem(params, "name");
    if (!cJSON_IsString(name)) {
        send_jsonrpc_error(-32602, "Missing or invalid 'name' parameter");
        return;
    }

    size_t json_len = lora_presets_json_end - lora_presets_json_start;
    char *json_str = malloc(json_len + 1);
    if (!json_str) {
        send_jsonrpc_error(-32603, "Memory allocation failed");
        return;
    }
    memcpy(json_str, lora_presets_json_start, json_len);
    json_str[json_len] = '\0';

    cJSON *presets = cJSON_Parse(json_str);
    free(json_str);

    if (!presets) {
        send_jsonrpc_error(-32603, "Failed to parse presets");
        return;
    }

    cJSON *preset = NULL;
    cJSON_ArrayForEach(preset, presets) {
        cJSON *preset_name = cJSON_GetObjectItem(preset, "name");
        if (cJSON_IsString(preset_name) && strcmp(preset_name->valuestring, name->valuestring) == 0) {
            break;
        }
        preset = NULL;
    }

    if (!preset) {
        cJSON_Delete(presets);
        send_jsonrpc_error(-32602, "Preset not found");
        return;
    }

    // Apply preset to current config
    handle_set_lora_config(preset);
    cJSON_Delete(presets);
}

static void handle_pair_device(cJSON *pair_json)
{
    cJSON *name      = cJSON_GetObjectItem(pair_json, "name");
    const cJSON *mac = cJSON_GetObjectItem(pair_json, "mac");
    const cJSON *key = cJSON_GetObjectItem(pair_json, "aes_key");

    if (!name || !mac || !key) {
        send_jsonrpc_error(-32602, "Missing pairing parameters");
        return;
    }

    // Validate MAC address format
    if (strlen(mac->valuestring) != 17) {
        send_jsonrpc_error(-32602, "Invalid MAC address format (expected aa:bb:cc:dd:ee:ff)");
        return;
    }

    uint8_t mac_bytes[6];
    if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac_bytes[0], &mac_bytes[1],
               &mac_bytes[2], &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
        send_jsonrpc_error(-32602, "Invalid MAC address");
        return;
    }

    uint8_t aes_key[32];
    const char *key_str = key->valuestring;
    if (strlen(key_str) != 64) {
        send_jsonrpc_error(-32602, "Invalid key length (expected 64 hex chars for AES-256)");
        return;
    }

    // Validate hex characters
    for (size_t i = 0; i < 64; i++) {
        char c = key_str[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            send_jsonrpc_error(-32602, "Invalid hex character in key");
            return;
        }
    }

    for (int i = 0; i < 32; i++) {
        if (sscanf(key_str + i * 2, "%02hhx", &aes_key[i]) != 1) {
            send_jsonrpc_error(-32602, "Invalid key format");
            return;
        }
    }

    uint16_t device_id = (mac_bytes[4] << 8) | mac_bytes[5];

    esp_err_t ret = device_registry_add(device_id, name->valuestring, mac_bytes, aes_key);
    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to add device");
        return;
    }

    ESP_LOGI(TAG, "Device paired: 0x%04X (%s)", device_id, name->valuestring);
    cJSON *response = cJSON_CreateString("Device paired successfully");
    send_jsonrpc_result(response);
}

static void handle_unpair_device(cJSON *unpair_json)
{
    const cJSON *mac = cJSON_GetObjectItem(unpair_json, "mac");
    if (!cJSON_IsString(mac)) {
        send_jsonrpc_error(-32602, "Missing MAC address parameter");
        return;
    }

    // Validate MAC address format
    if (strlen(mac->valuestring) != 17) {
        send_jsonrpc_error(-32602, "Invalid MAC address format (expected aa:bb:cc:dd:ee:ff)");
        return;
    }

    uint8_t mac_bytes[6];
    if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac_bytes[0], &mac_bytes[1],
               &mac_bytes[2], &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
        send_jsonrpc_error(-32602, "Invalid MAC address");
        return;
    }

    uint16_t device_id = (mac_bytes[4] << 8) | mac_bytes[5];
    esp_err_t ret = device_registry_remove(device_id);

    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Device not found");
        return;
    }

    ESP_LOGI(TAG, "Device unpaired: 0x%04X", device_id);
    cJSON *response = cJSON_CreateString("Device unpaired successfully");
    send_jsonrpc_result(response);
}

static void handle_device_reset(void)
{
    ESP_LOGW(TAG, "Factory reset initiated - erasing all NVS data");
    
    cJSON *response = cJSON_CreateString("Factory reset initiated, rebooting...");
    send_jsonrpc_result(response);
    
    // Give time for response to be sent
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Erase entire NVS partition
    esp_err_t ret = nvs_flash_erase();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase NVS: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "NVS erased successfully, rebooting...");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Reboot device
    esp_restart();
}

static void handle_firmware_start(cJSON *params)
{
    cJSON *size_json = cJSON_GetObjectItem(params, "size");
    if (!cJSON_IsNumber(size_json)) {
        send_jsonrpc_error(-32602, "Missing or invalid size parameter");
        return;
    }

    cJSON *sha256_json = cJSON_GetObjectItem(params, "sha256");
    if (!cJSON_IsString(sha256_json)) {
        send_jsonrpc_error(-32602, "Missing or invalid sha256 parameter");
        return;
    }

    cJSON *signature_json = cJSON_GetObjectItem(params, "signature");
    if (!cJSON_IsString(signature_json)) {
        send_jsonrpc_error(-32602, "Missing or invalid signature parameter");
        return;
    }

    size_t size = (size_t)size_json->valueint;
    if (size == 0 || size > MAX_FIRMWARE_SIZE) {
        send_jsonrpc_error(-32602, "Invalid firmware size");
        return;
    }

    // TODO: Validate sha256 format (64 hex chars)
    // TODO: Validate signature format
    // TODO: Implement SHA256 verification after binary transfer
    // TODO: Implement signature verification (Ed25519 or RSA)
    
    ESP_LOGW(TAG, "Firmware upgrade: size=%zu, sha256=%s (verification not implemented)", 
             size, sha256_json->valuestring);

    esp_err_t ret = ota_engine_start(size);
    if (ret != ESP_OK) {
        send_jsonrpc_error(-32603, "Failed to start OTA");
        return;
    }

    cJSON *result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "status", "ready");
    cJSON_AddNumberToObject(result, "size", size);
    send_jsonrpc_result(result);
    
    // After this response, switch to raw binary mode
    // The calling code (usb_cdc.c) will handle the binary transfer
}

//==============================================================================
// JSON-RPC METHOD TABLE
//==============================================================================

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
    {"lora:bands", false, {.no_params = handle_get_lora_bands}},
    {"lora:presets:list", false, {.no_params = handle_get_lora_presets}},
    {"lora:presets:set", true, {.with_params = handle_set_lora_preset}},
    {"paired:list", false, {.no_params = handle_get_paired_devices}},
    {"paired:pair", true, {.with_params = handle_pair_device}},
    {"paired:unpair", true, {.with_params = handle_unpair_device}},
    {"device:reset", false, {.no_params = handle_device_reset}},
    {"firmware:upgrade", true, {.with_params = handle_firmware_start}},
};

void commands_execute(const char *command_line, response_fn_t send_response)
{
    // Validate callback
    if (!send_response) {
        ESP_LOGE(TAG, "Response callback not set");
        return;
    }
    
    s_send_response = send_response;
    s_request_id = NULL;

    power_mgmt_update_activity();

    // Validate input size
    size_t len = strlen(command_line);
    if (len > MAX_COMMAND_LENGTH) {
        send_jsonrpc_error(JSONRPC_INVALID_REQUEST, "Request too large");
        return;
    }

    // Parse JSON-RPC request
    cJSON *request = cJSON_Parse(command_line);
    if (!request) {
        send_jsonrpc_error(JSONRPC_PARSE_ERROR, "Invalid JSON");
        return;
    }

    // Validate JSON-RPC 2.0
    cJSON *jsonrpc = cJSON_GetObjectItem(request, "jsonrpc");
    if (!jsonrpc || !cJSON_IsString(jsonrpc) || strcmp(jsonrpc->valuestring, "2.0") != 0) {
        send_jsonrpc_error(JSONRPC_INVALID_REQUEST, "Missing or invalid jsonrpc field");
        cJSON_Delete(request);
        return;
    }

    // Extract method
    cJSON *method = cJSON_GetObjectItem(request, "method");
    if (!method || !cJSON_IsString(method)) {
        send_jsonrpc_error(JSONRPC_INVALID_REQUEST, "Missing method");
        cJSON_Delete(request);
        return;
    }

    // Extract id
    s_request_id = cJSON_GetObjectItem(request, "id");

    // Extract params
    cJSON *params = cJSON_GetObjectItem(request, "params");

    // Dispatch to handler
    bool found = false;
    for (size_t i = 0; i < sizeof(method_table) / sizeof(method_table[0]); i++) {
        if (strcmp(method->valuestring, method_table[i].method) == 0) {
            if (method_table[i].has_params) {
                if (!params) {
                    send_jsonrpc_error(JSONRPC_INVALID_PARAMS, "Missing required params");
                    cJSON_Delete(request);
                    return;
                }
                method_table[i].handler.with_params(params);
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

// Legacy text protocol support (deprecated, for backward compatibility)
void commands_execute_legacy(const char *command_line, response_fn_t send_response)
{
    s_send_response = send_response;

    // Reset sleep timer on any command activity
    power_mgmt_update_activity();

    if (strcmp(command_line, "PING") == 0) {
        handle_ping();
        return;
    }

    if (strcmp(command_line, "GET_DEVICE_INFO") == 0) {
        handle_get_device_info();
        return;
    }

    if (strcmp(command_line, "GET_GENERAL") == 0) {
        handle_get_general();
        return;
    }

    if (strcmp(command_line, "GET_POWER_MANAGEMENT") == 0) {
        handle_get_power_management();
        return;
    }

    if (strcmp(command_line, "GET_LORA") == 0) {
        handle_get_lora_config();
        return;
    }

    if (strcmp(command_line, "GET_LORA_KEY") == 0) {
        handle_get_lora_key();
        return;
    }

    if (strncmp(command_line, "SET_LORA_KEY ", 13) == 0) {
        cJSON *json = cJSON_Parse(command_line + 13);
        if (json) {
            handle_set_lora_key(json);
            cJSON_Delete(json);
        } else {
            s_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strcmp(command_line, "GET_PAIRED_DEVICES") == 0) {
        handle_get_paired_devices();
        return;
    }

    if (strcmp(command_line, "GET_LORA_BANDS") == 0) {
        handle_get_lora_bands();
        return;
    }

    if (strncmp(command_line, "SET_POWER_MANAGEMENT ", 21) == 0) {
        cJSON *json = cJSON_Parse(command_line + 21);
        if (json) {
            handle_set_power_management(json);
            cJSON_Delete(json);
        } else {
            s_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "SET_GENERAL ", 12) == 0) {
        cJSON *json = cJSON_Parse(command_line + 12);
        if (json) {
            handle_set_general(json);
            cJSON_Delete(json);
        } else {
            s_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "PAIR_DEVICE ", 12) == 0) {
        cJSON *json = cJSON_Parse(command_line + 12);
        if (json) {
            handle_pair_device(json);
            cJSON_Delete(json);
        } else {
            s_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "UNPAIR_DEVICE ", 14) == 0) {
        cJSON *json = cJSON_Parse(command_line + 14);
        if (!json) {
            s_send_response("ERROR Invalid JSON");
            return;
        }

        const cJSON *mac = cJSON_GetObjectItem(json, "mac");
        if (!cJSON_IsString(mac)) {
            cJSON_Delete(json);
            s_send_response("ERROR Missing mac parameter");
            return;
        }

        uint8_t mac_bytes[6];
        if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac_bytes[0], &mac_bytes[1],
                   &mac_bytes[2], &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
            cJSON_Delete(json);
            s_send_response("ERROR Invalid MAC address");
            return;
        }

        uint16_t device_id = (mac_bytes[4] << 8) | mac_bytes[5];
        esp_err_t ret      = device_registry_remove(device_id);
        cJSON_Delete(json);

        if (ret != ESP_OK) {
            s_send_response("ERROR Device not found");
            return;
        }

        ESP_LOGI(TAG, "Device unpaired: 0x%04X", device_id);
        s_send_response("OK Device unpaired successfully");
        return;
    }

    if (strncmp(command_line, "SET_LORA ", 9) == 0) {
        cJSON *json = cJSON_Parse(command_line + 9);
        if (json) {
            handle_set_lora_config(json);
            cJSON_Delete(json);
        } else {
            s_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "FIRMWARE_UPGRADE ", 17) == 0) {
        size_t size = atoi(command_line + 17);
        if (size == 0 || size > MAX_FIRMWARE_SIZE) {
            s_send_response("ERROR Invalid size");
            return;
        }

        s_send_response("OK Ready");
        vTaskDelay(pdMS_TO_TICKS(100));

        esp_err_t ret = ota_engine_start(size);
        if (ret != ESP_OK) {
            s_send_response("ERROR OTA start failed");
            return;
        }

        size_t received = 0;
        uint8_t buf[512];
        
        while (received < size) {
            size_t to_read = (size - received) < sizeof(buf) ? (size - received) : sizeof(buf);
            size_t read = 0;
            
            while (read < to_read && tud_cdc_available()) {
                buf[read++] = tud_cdc_read_char();
            }
            
            if (read == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            
            ret = ota_engine_write(buf, read);
            if (ret != ESP_OK) {
                ota_engine_abort();
                s_send_response("ERROR OTA write failed");
                return;
            }
            
            received += read;
            
            if (received % 10240 == 0) {
                ESP_LOGI(TAG, "Progress: %zu/%zu bytes", received, size);
            }
        }

        ret = ota_engine_finish();
        if (ret != ESP_OK) {
            s_send_response("ERROR OTA finish failed");
            return;
        }

        const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
        if (!update_partition) {
            s_send_response("ERROR No update partition");
            return;
        }

        ret = esp_ota_set_boot_partition(update_partition);
        if (ret != ESP_OK) {
            s_send_response("ERROR Failed to set boot partition");
            return;
        }

        s_send_response("OK Complete, rebooting...");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
        return;
    }

    s_send_response("ERROR Unknown command");
}
