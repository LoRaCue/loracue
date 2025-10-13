#include "commands.h"
#include "cJSON.h"
#include "bluetooth_config.h"
#include "general_config.h"
#include "device_registry.h"
#include "esp_app_format.h"
#include "esp_crc.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "firmware_manifest.h"
#include "lora_bands.h"
#include "lora_driver.h"
#include "ota_compatibility.h"
#include "ota_error_screen.h"
#include "power_mgmt.h"
#include "power_mgmt_config.h"
#include "mbedtls/base64.h"
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "u8g2.h"
#include <string.h>

static const char *TAG = "COMMANDS";

// OTA update state
static esp_ota_handle_t ota_handle          = 0;
static const esp_partition_t *ota_partition = NULL;
static size_t ota_received_bytes            = 0;
static size_t ota_total_size                = 0;
static size_t ota_chunk_size                = 4096;
static size_t ota_expected_chunks           = 0;
static bool ota_force_mode                  = false;
static uint8_t ota_header_buffer[4096]      = {0};  // Buffer for manifest extraction
static bool ota_manifest_checked            = false;

// Response callback (set by commands_execute)
static response_fn_t g_send_response = NULL;

static void handle_ping(void)
{
    g_send_response("PONG");
}

static void handle_get_version(void)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "version", firmware_manifest_get_version());
    cJSON_AddStringToObject(response, "board_id", firmware_manifest_get_board_id());
    
    // Add MAC address
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(response, "mac", mac_str);

    char *json_string = cJSON_PrintUnformatted(response);
    g_send_response(json_string);
    free(json_string);
    cJSON_Delete(response);
}

static void handle_get_device_config(void)
{
    general_config_t config;
    if (general_config_get(&config) != ESP_OK) {
        g_send_response("ERROR Failed to get device config");
        return;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "name", config.device_name);
    cJSON_AddStringToObject(response, "mode", device_mode_to_string(config.device_mode));
    cJSON_AddNumberToObject(response, "brightness", config.display_brightness);
    cJSON_AddBoolToObject(response, "bluetooth", config.bluetooth_enabled);
    cJSON_AddNumberToObject(response, "slot_id", config.slot_id);

    char *json_string = cJSON_PrintUnformatted(response);
    g_send_response(json_string);
    free(json_string);
    cJSON_Delete(response);
}

static void handle_set_device_config(cJSON *config_json)
{
    general_config_t config;
    esp_err_t ret = general_config_get(&config);
    if (ret != ESP_OK) {
        g_send_response("ERROR Failed to get current device config");
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
            g_send_response("ERROR Invalid mode (must be PRESENTER or PC)");
            return;
        }
        config.device_mode = new_mode;
        extern device_mode_t current_device_mode;
        current_device_mode = new_mode;
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
            g_send_response("ERROR Invalid slot_id (must be 1-16)");
            return;
        }
        config.slot_id = slot;
    }

    ret = general_config_set(&config);
    if (ret != ESP_OK) {
        g_send_response("ERROR Failed to save device config");
        return;
    }

    g_send_response("OK Device config updated");
}

static void handle_get_power_management(void)
{
    power_mgmt_config_t config;
    if (power_mgmt_config_get(&config) != ESP_OK) {
        g_send_response("ERROR Failed to get power config");
        return;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "display_sleep_enabled", config.display_sleep_enabled);
    cJSON_AddNumberToObject(response, "display_sleep_timeout_ms", config.display_sleep_timeout_ms);
    cJSON_AddBoolToObject(response, "light_sleep_enabled", config.light_sleep_enabled);
    cJSON_AddNumberToObject(response, "light_sleep_timeout_ms", config.light_sleep_timeout_ms);
    cJSON_AddBoolToObject(response, "deep_sleep_enabled", config.deep_sleep_enabled);
    cJSON_AddNumberToObject(response, "deep_sleep_timeout_ms", config.deep_sleep_timeout_ms);

    char *json_string = cJSON_PrintUnformatted(response);
    g_send_response(json_string);
    free(json_string);
    cJSON_Delete(response);
}

static void handle_set_power_management(cJSON *config_json)
{
    power_mgmt_config_t config;
    if (power_mgmt_config_get(&config) != ESP_OK) {
        g_send_response("ERROR Failed to get current power config");
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
        g_send_response("ERROR Failed to save power config");
        return;
    }

    g_send_response("OK Power config updated - restart required");
}

static void handle_get_lora_config(void)
{
    lora_config_t config;
    esp_err_t ret = lora_get_config(&config);

    if (ret != ESP_OK) {
        g_send_response("ERROR Failed to get LoRa config");
        return;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "band_id", config.band_id);
    cJSON_AddNumberToObject(response, "frequency", config.frequency);
    cJSON_AddNumberToObject(response, "spreading_factor", config.spreading_factor);
    cJSON_AddNumberToObject(response, "bandwidth", config.bandwidth);
    cJSON_AddNumberToObject(response, "coding_rate", config.coding_rate);
    cJSON_AddNumberToObject(response, "tx_power", config.tx_power);

    char *json_string = cJSON_PrintUnformatted(response);
    g_send_response(json_string);
    free(json_string);
    cJSON_Delete(response);
}

static void handle_set_lora_config(cJSON *config_json)
{
    lora_config_t config;
    esp_err_t ret = lora_get_config(&config);

    if (ret != ESP_OK) {
        g_send_response("ERROR Failed to get current LoRa config");
        return;
    }

    cJSON *freq    = cJSON_GetObjectItem(config_json, "frequency");
    cJSON *sf      = cJSON_GetObjectItem(config_json, "spreading_factor");
    cJSON *bw      = cJSON_GetObjectItem(config_json, "bandwidth");
    cJSON *cr      = cJSON_GetObjectItem(config_json, "coding_rate");
    cJSON *power   = cJSON_GetObjectItem(config_json, "tx_power");
    cJSON *band_id = cJSON_GetObjectItem(config_json, "band_id");

    if (cJSON_IsNumber(freq))
        config.frequency = freq->valueint;
    if (cJSON_IsNumber(sf))
        config.spreading_factor = sf->valueint;
    if (cJSON_IsNumber(bw))
        config.bandwidth = bw->valueint;
    if (cJSON_IsNumber(cr))
        config.coding_rate = cr->valueint;
    if (cJSON_IsNumber(power))
        config.tx_power = power->valueint;
    if (cJSON_IsString(band_id)) {
        strncpy(config.band_id, band_id->valuestring, sizeof(config.band_id) - 1);
        config.band_id[sizeof(config.band_id) - 1] = '\0';
    }

    // Validate frequency against band limits
    extern const lora_band_profile_t *lora_bands_get_profile_by_id(const char *band_id);
    const lora_band_profile_t *band = lora_bands_get_profile_by_id(config.band_id);
    if (band) {
        uint32_t freq_khz = config.frequency / 1000;
        if (freq_khz < band->optimal_freq_min_khz || freq_khz > band->optimal_freq_max_khz) {
            cJSON *error = cJSON_CreateObject();
            cJSON_AddStringToObject(error, "status", "error");
            cJSON_AddStringToObject(error, "reason", "Frequency out of band range");
            cJSON_AddNumberToObject(error, "min_khz", band->optimal_freq_min_khz);
            cJSON_AddNumberToObject(error, "max_khz", band->optimal_freq_max_khz);
            char *error_str = cJSON_PrintUnformatted(error);
            g_send_response(error_str);
            free(error_str);
            cJSON_Delete(error);
            return;
        }
    }

    ret = lora_set_config(&config);
    if (ret == ESP_OK) {
        g_send_response("OK LoRa configuration updated");
    } else {
        g_send_response("ERROR Failed to update LoRa configuration");
    }
}

static void handle_get_paired_devices(void)
{
    cJSON *devices_array = cJSON_CreateArray();

    for (int i = 0; i < 10; i++) {
        paired_device_t device;
        uint16_t device_id = i + 1;

        if (device_registry_get(device_id, &device) == ESP_OK) {
            cJSON *device_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(device_obj, "name", device.device_name);

            char mac_str[18];
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", device.mac_address[0],
                     device.mac_address[1], device.mac_address[2], device.mac_address[3], device.mac_address[4],
                     device.mac_address[5]);
            cJSON_AddStringToObject(device_obj, "mac", mac_str);

            cJSON_AddItemToArray(devices_array, device_obj);
        }
    }

    char *json_string = cJSON_PrintUnformatted(devices_array);
    g_send_response(json_string);
    free(json_string);
    cJSON_Delete(devices_array);
}

static void handle_get_lora_bands(void)
{
    extern int lora_bands_get_count(void);
    extern const lora_band_profile_t *lora_bands_get_profile(int index);
    
    int band_count = lora_bands_get_count();
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

    char *json_string = cJSON_PrintUnformatted(bands_array);
    g_send_response(json_string);
    free(json_string);
    cJSON_Delete(bands_array);
}

static void handle_pair_device(cJSON *pair_json)
{
    cJSON *name       = cJSON_GetObjectItem(pair_json, "name");
    const cJSON *mac  = cJSON_GetObjectItem(pair_json, "mac");
    const cJSON *key  = cJSON_GetObjectItem(pair_json, "key");

    if (!name || !mac || !key) {
        g_send_response("ERROR Missing pairing parameters");
        return;
    }

    uint8_t mac_bytes[6];
    if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac_bytes[0], &mac_bytes[1],
               &mac_bytes[2], &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
        g_send_response("ERROR Invalid MAC address");
        return;
    }

    uint8_t aes_key[32];
    const char *key_str = key->valuestring;
    if (strlen(key_str) != 64) {
        g_send_response("ERROR Invalid key length (expected 64 hex chars for AES-256)");
        return;
    }

    for (int i = 0; i < 32; i++) {
        if (sscanf(key_str + i * 2, "%02hhx", &aes_key[i]) != 1) {
            g_send_response("ERROR Invalid key format");
            return;
        }
    }

    uint16_t device_id = (mac_bytes[4] << 8) | mac_bytes[5];

    esp_err_t ret = device_registry_add(device_id, name->valuestring, mac_bytes, aes_key);
    if (ret != ESP_OK) {
        g_send_response("ERROR Failed to add device");
        return;
    }

    ESP_LOGI(TAG, "Device paired: 0x%04X (%s)", device_id, name->valuestring);
    g_send_response("OK Device paired successfully");
}

static void handle_unpair_device(cJSON *unpair_json)
{
    cJSON *id = cJSON_GetObjectItem(unpair_json, "id");
    
    if (!cJSON_IsNumber(id)) {
        g_send_response("ERROR Missing device ID");
        return;
    }
    
    uint16_t device_id = id->valueint;
    
    if (device_id < 1 || device_id > MAX_PAIRED_DEVICES) {
        g_send_response("ERROR Invalid device ID");
        return;
    }
    
    esp_err_t ret = device_registry_remove(device_id);
    if (ret != ESP_OK) {
        g_send_response("ERROR Device not found");
        return;
    }
    
    ESP_LOGI(TAG, "Device unpaired: 0x%04X", device_id);
    g_send_response("OK Device unpaired successfully");
}

static void handle_update_paired_device(cJSON *update_json)
{
    cJSON *id = cJSON_GetObjectItem(update_json, "id");
    cJSON *name = cJSON_GetObjectItem(update_json, "name");
    const cJSON *mac = cJSON_GetObjectItem(update_json, "mac");
    const cJSON *key = cJSON_GetObjectItem(update_json, "key");
    
    if (!cJSON_IsNumber(id) || !cJSON_IsString(name) || 
        !cJSON_IsString(mac) || !cJSON_IsString(key)) {
        g_send_response("ERROR Missing parameters");
        return;
    }
    
    uint16_t device_id = id->valueint;
    
    if (device_id < 1 || device_id > MAX_PAIRED_DEVICES) {
        g_send_response("ERROR Invalid device ID");
        return;
    }
    
    // Check if device exists
    paired_device_t existing;
    if (device_registry_get(device_id, &existing) != ESP_OK) {
        g_send_response("ERROR Device not found");
        return;
    }
    
    // Validate MAC address
    uint8_t mac_bytes[6];
    if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", 
               &mac_bytes[0], &mac_bytes[1], &mac_bytes[2], 
               &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
        g_send_response("ERROR Invalid MAC address");
        return;
    }
    
    // Validate AES key
    uint8_t aes_key[32];
    const char *key_str = key->valuestring;
    if (strlen(key_str) != 64) {
        g_send_response("ERROR Invalid key length (expected 64 hex chars for AES-256)");
        return;
    }
    
    for (int i = 0; i < 32; i++) {
        if (sscanf(key_str + i * 2, "%02hhx", &aes_key[i]) != 1) {
            g_send_response("ERROR Invalid key format");
            return;
        }
    }
    
    // Update device (overwrites existing)
    esp_err_t ret = device_registry_add(device_id, name->valuestring, mac_bytes, aes_key);
    if (ret != ESP_OK) {
        g_send_response("ERROR Failed to update device");
        return;
    }
    
    ESP_LOGI(TAG, "Device updated: 0x%04X (%s)", device_id, name->valuestring);
    g_send_response("OK Device updated successfully");
}

static void handle_fw_update_start(cJSON *update_json)
{
    cJSON *size       = cJSON_GetObjectItem(update_json, "size");
    cJSON *chunk_size = cJSON_GetObjectItem(update_json, "chunk_size");
    cJSON *checksum   = cJSON_GetObjectItem(update_json, "checksum");
    cJSON *version    = cJSON_GetObjectItem(update_json, "version");
    cJSON *force      = cJSON_GetObjectItem(update_json, "force");

    if (!cJSON_IsNumber(size) || !cJSON_IsString(checksum) || !cJSON_IsString(version)) {
        g_send_response("ERROR Invalid firmware update parameters");
        return;
    }

    // Validate chunk_size
    if (chunk_size && cJSON_IsNumber(chunk_size)) {
        ota_chunk_size = chunk_size->valueint;
        if (ota_chunk_size < 512 || ota_chunk_size > 8192) {
            g_send_response("ERROR chunk_size must be between 512 and 8192 bytes");
            return;
        }
    } else {
        ota_chunk_size = 4096; // Default
    }

    // Check force mode
    ota_force_mode = (force && cJSON_IsTrue(force));
    if (ota_force_mode) {
        ESP_LOGW(TAG, "Force mode enabled - compatibility checks will be bypassed");
    }

    if (ota_handle) {
        esp_ota_abort(ota_handle);
        ota_handle = 0;
    }

    ota_total_size       = size->valueint;
    ota_received_bytes   = 0;
    ota_expected_chunks  = (ota_total_size + ota_chunk_size - 1) / ota_chunk_size;
    ota_manifest_checked = false;
    memset(ota_header_buffer, 0, sizeof(ota_header_buffer));

    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        g_send_response("ERROR No OTA partition available");
        return;
    }

    esp_err_t ret = esp_ota_begin(ota_partition, ota_total_size, &ota_handle);
    if (ret != ESP_OK) {
        g_send_response("ERROR Failed to begin OTA update");
        return;
    }

    ESP_LOGI(TAG, "OTA update started: %d bytes, chunk_size=%d, chunks=%d, version=%s, force=%d", 
             ota_total_size, ota_chunk_size, ota_expected_chunks, version->valuestring, ota_force_mode);
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "ok");
    cJSON_AddBoolToObject(response, "force_mode", ota_force_mode);
    cJSON_AddNumberToObject(response, "expected_chunks", ota_expected_chunks);
    char *response_str = cJSON_PrintUnformatted(response);
    g_send_response(response_str);
    free(response_str);
    cJSON_Delete(response);
}

static void handle_fw_update_data(const char *data_str)
{
    if (!ota_handle) {
        g_send_response("ERROR No active OTA session");
        return;
    }

    const char *base64_data = data_str;
    char *space_pos = strchr(data_str, ' ');
    if (space_pos) {
        base64_data = space_pos + 1;
    }

    size_t base64_len = strlen(base64_data);
    
    size_t max_decoded_len = (base64_len * 3) / 4 + 3;
    uint8_t *binary_data = malloc(max_decoded_len);
    if (!binary_data) {
        g_send_response("ERROR Memory allocation failed");
        return;
    }

    size_t data_len;
    int ret = mbedtls_base64_decode(binary_data, max_decoded_len, &data_len,
                                     (const unsigned char *)base64_data, base64_len);
    if (ret != 0) {
        free(binary_data);
        g_send_response("ERROR Invalid base64 data");
        return;
    }

    // Validate chunk size (except last chunk)
    size_t remaining = ota_total_size - ota_received_bytes;
    size_t expected_size = (remaining >= ota_chunk_size) ? ota_chunk_size : remaining;
    if (data_len > expected_size) {
        free(binary_data);
        g_send_response("ERROR Chunk size exceeds expected");
        return;
    }

    // Store first 4KB for manifest extraction
    if (!ota_manifest_checked && ota_received_bytes < sizeof(ota_header_buffer)) {
        size_t copy_len = (ota_received_bytes + data_len <= sizeof(ota_header_buffer)) 
            ? data_len 
            : (sizeof(ota_header_buffer) - ota_received_bytes);
        memcpy(ota_header_buffer + ota_received_bytes, binary_data, copy_len);
    }

    esp_err_t ota_ret = esp_ota_write(ota_handle, binary_data, data_len);
    
    if (ota_ret != ESP_OK) {
        free(binary_data);
        g_send_response("ERROR Failed to write OTA data");
        return;
    }

    ota_received_bytes += data_len;

    ESP_LOGD(TAG, "OTA chunk: %d bytes, total: %d/%d (%.1f%%)", 
             data_len, ota_received_bytes, ota_total_size, 
             (float)ota_received_bytes * 100.0f / ota_total_size);

    // Check compatibility after receiving first 4KB
    if (!ota_manifest_checked && ota_received_bytes >= sizeof(ota_header_buffer)) {
        ota_manifest_checked = true;
        
        if (!ota_force_mode) {
            esp_app_desc_t new_app_info;
            if (esp_ota_get_partition_description(ota_partition, &new_app_info) == ESP_OK) {
                if (!firmware_manifest_is_compatible(&new_app_info)) {
                    free(binary_data);
                    esp_ota_abort(ota_handle);
                    ota_handle = 0;
                    
                    // Show error on OLED
                    ota_error_screen_draw(firmware_manifest_get_board_id(), new_app_info.project_name);
                    
                    cJSON *error = cJSON_CreateObject();
                    cJSON_AddStringToObject(error, "status", "error");
                    cJSON_AddStringToObject(error, "reason", "Board mismatch");
                    cJSON_AddStringToObject(error, "current_board", firmware_manifest_get_board_id());
                    cJSON_AddStringToObject(error, "new_board", new_app_info.project_name);
                    char *error_str = cJSON_PrintUnformatted(error);
                    g_send_response(error_str);
                    free(error_str);
                    cJSON_Delete(error);
                    return;
                }
            } else {
                ESP_LOGW(TAG, "Could not read app descriptor, proceeding anyway");
            }
        } else {
            ESP_LOGW(TAG, "Force mode: skipping compatibility check");
        }
    }

    // Calculate CRC32 checksum of received chunk
    uint32_t chunk_crc = esp_crc32_le(0, binary_data, data_len);

    free(binary_data);
    
    char response[80];
    snprintf(response, sizeof(response), "OK %zu/%zu 0x%08" PRIx32, 
             ota_received_bytes, ota_total_size, chunk_crc);
    g_send_response(response);
}

static void handle_fw_update_verify(void)
{
    if (!ota_handle) {
        g_send_response("ERROR No active OTA session");
        return;
    }

    // Check if we received all data
    if (ota_received_bytes != ota_total_size) {
        ESP_LOGE(TAG, "Size mismatch: received %zu, expected %zu", ota_received_bytes, ota_total_size);
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "ERROR Size mismatch: received %zu, expected %zu", 
                 ota_received_bytes, ota_total_size);
        g_send_response(error_msg);
        esp_ota_abort(ota_handle);
        ota_handle = 0;
        return;
    }

    esp_err_t ret = esp_ota_end(ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s (0x%x)", esp_err_to_name(ret), ret);
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "ERROR OTA verification failed: %s", esp_err_to_name(ret));
        g_send_response(error_msg);
        ota_handle = 0;
        return;
    }

    ESP_LOGI(TAG, "Firmware verified successfully (%d bytes)", ota_received_bytes);
    g_send_response("OK Firmware verified");
}

static void handle_fw_update_abort(void)
{
    if (ota_handle) {
        esp_ota_abort(ota_handle);
        ota_handle = 0;
        ESP_LOGI(TAG, "OTA update aborted");
    }
    
    ota_partition = NULL;
    ota_received_bytes = 0;
    ota_total_size = 0;
    ota_manifest_checked = false;
    
    g_send_response("OK OTA aborted");
}

static void handle_fw_update_commit(void)
{
    if (!ota_partition) {
        g_send_response("ERROR No OTA partition to commit");
        return;
    }

    esp_err_t ret = esp_ota_set_boot_partition(ota_partition);
    if (ret != ESP_OK) {
        g_send_response("ERROR Failed to set boot partition");
        return;
    }

    g_send_response("OK Firmware committed - rebooting");

    ota_handle         = 0;
    ota_partition      = NULL;
    ota_received_bytes = 0;
    ota_total_size     = 0;

    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

void commands_execute(const char *command_line, response_fn_t send_response)
{
    g_send_response = send_response;
    
    // Reset sleep timer on any command activity
    power_mgmt_update_activity();

    if (strcmp(command_line, "PING") == 0) {
        handle_ping();
        return;
    }

    if (strcmp(command_line, "GET_DEVICE_INFO") == 0) {
        handle_get_version();
        return;
    }

    if (strcmp(command_line, "GET_GENERAL") == 0) {
        handle_get_device_config();
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
            g_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "SET_GENERAL ", 12) == 0) {
        cJSON *json = cJSON_Parse(command_line + 12);
        if (json) {
            handle_set_device_config(json);
            cJSON_Delete(json);
        } else {
            g_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "PAIR_DEVICE ", 12) == 0) {
        cJSON *json = cJSON_Parse(command_line + 12);
        if (json) {
            handle_pair_device(json);
            cJSON_Delete(json);
        } else {
            g_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "UNPAIR_DEVICE ", 14) == 0) {
        cJSON *json = cJSON_Parse(command_line + 14);
        if (json) {
            handle_unpair_device(json);
            cJSON_Delete(json);
        } else {
            g_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "UPDATE_PAIRED_DEVICE ", 21) == 0) {
        cJSON *json = cJSON_Parse(command_line + 21);
        if (json) {
            handle_update_paired_device(json);
            cJSON_Delete(json);
        } else {
            g_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "SET_LORA ", 9) == 0) {
        cJSON *json = cJSON_Parse(command_line + 9);
        if (json) {
            handle_set_lora_config(json);
            cJSON_Delete(json);
        } else {
            g_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "FW_UPDATE_START ", 16) == 0) {
        cJSON *json = cJSON_Parse(command_line + 16);
        if (json) {
            handle_fw_update_start(json);
            cJSON_Delete(json);
        } else {
            g_send_response("ERROR Invalid JSON");
        }
        return;
    }

    if (strncmp(command_line, "FW_UPDATE_DATA ", 15) == 0) {
        handle_fw_update_data(command_line + 15);
        return;
    }

    if (strcmp(command_line, "FW_UPDATE_VERIFY") == 0) {
        handle_fw_update_verify();
        return;
    }

    if (strcmp(command_line, "FW_UPDATE_ABORT") == 0) {
        handle_fw_update_abort();
        return;
    }

    if (strcmp(command_line, "FW_UPDATE_COMMIT") == 0) {
        handle_fw_update_commit();
        return;
    }

    g_send_response("ERROR Unknown command");
}
