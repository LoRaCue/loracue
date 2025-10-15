#include "commands.h"
#include "cJSON.h"
#include "bluetooth_config.h"
#include "general_config.h"
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
#include "lora_bands.h"
#include "lora_driver.h"
#include "ota_engine.h"
#include "version.h"
#include "power_mgmt.h"
#include "power_mgmt_config.h"
#include "version.h"
#include "mbedtls/base64.h"
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "u8g2.h"
#include <string.h>
#include "xmodem.h"

static const char *TAG = "COMMANDS";

// Response callback (set by commands_execute)
static response_fn_t g_send_response = NULL;

static void handle_ping(void)
{
    general_config_t config;
    general_config_get(&config);
    
    char response[128];
    snprintf(response, sizeof(response), "PONG %s v%s", config.device_name, LORACUE_VERSION_FULL);
    g_send_response(response);
}

static void handle_get_device_info(void)
{
    cJSON *response = cJSON_CreateObject();
    
    // Firmware info
    const esp_app_desc_t *app_desc = esp_app_get_description();
    cJSON_AddStringToObject(response, "board_id", app_desc->project_name);
    cJSON_AddStringToObject(response, "version", LORACUE_VERSION_STRING);
    cJSON_AddStringToObject(response, "commit", LORACUE_BUILD_COMMIT_SHORT);
    cJSON_AddStringToObject(response, "branch", LORACUE_BUILD_BRANCH);
    cJSON_AddStringToObject(response, "build_date", LORACUE_BUILD_DATE);
    
    // Hardware info
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
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(response, "mac", mac_str);
    
    // Runtime info
    cJSON_AddNumberToObject(response, "uptime_sec", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(response, "free_heap_kb", esp_get_free_heap_size() / 1024);
    
    // Partition info
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running) {
        cJSON_AddStringToObject(response, "partition", running->label);
    }

    char *json_string = cJSON_PrintUnformatted(response);
    g_send_response(json_string);
    free(json_string);
    cJSON_Delete(response);
}

static void handle_get_general(void)
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

static void handle_set_general(cJSON *config_json)
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
    
    // Use device_registry_list to get all paired devices
    paired_device_t devices[MAX_PAIRED_DEVICES];
    size_t count = 0;
    
    if (device_registry_list(devices, MAX_PAIRED_DEVICES, &count) == ESP_OK) {
        for (size_t i = 0; i < count; i++) {
            cJSON *device_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(device_obj, "name", devices[i].device_name);

            char mac_str[18];
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                     devices[i].mac_address[0], devices[i].mac_address[1], devices[i].mac_address[2],
                     devices[i].mac_address[3], devices[i].mac_address[4], devices[i].mac_address[5]);
            cJSON_AddStringToObject(device_obj, "mac", mac_str);

            char aes_key_str[65];
            for (int j = 0; j < 32; j++) {
                snprintf(aes_key_str + (j * 2), 3, "%02x", devices[i].aes_key[j]);
            }
            cJSON_AddStringToObject(device_obj, "aes_key", aes_key_str);

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
    const cJSON *key  = cJSON_GetObjectItem(pair_json, "aes_key");

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

static void handle_update_paired_device(cJSON *update_json)
{
    const cJSON *mac = cJSON_GetObjectItem(update_json, "mac");
    cJSON *name = cJSON_GetObjectItem(update_json, "name");
    const cJSON *key = cJSON_GetObjectItem(update_json, "aes_key");
    
    if (!cJSON_IsString(mac) || !cJSON_IsString(name) || !cJSON_IsString(key)) {
        g_send_response("ERROR Missing parameters");
        return;
    }
    
    // Parse MAC address
    uint8_t mac_bytes[6];
    if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", 
               &mac_bytes[0], &mac_bytes[1], &mac_bytes[2], 
               &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
        g_send_response("ERROR Invalid MAC address");
        return;
    }
    
    uint16_t device_id = (mac_bytes[4] << 8) | mac_bytes[5];
    
    // Check if device exists
    paired_device_t existing;
    if (device_registry_get(device_id, &existing) != ESP_OK) {
        g_send_response("ERROR Device not found");
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
    
    // Update device (overwrites existing, MAC stays the same)
    esp_err_t ret = device_registry_add(device_id, name->valuestring, mac_bytes, aes_key);
    if (ret != ESP_OK) {
        g_send_response("ERROR Failed to update device");
        return;
    }
    
    ESP_LOGI(TAG, "Device updated: 0x%04X (%s)", device_id, name->valuestring);
    g_send_response("OK Device updated successfully");
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
            handle_set_general(json);
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
        if (!json) {
            g_send_response("ERROR Invalid JSON");
            return;
        }
        
        const cJSON *mac = cJSON_GetObjectItem(json, "mac");
        if (!cJSON_IsString(mac)) {
            cJSON_Delete(json);
            g_send_response("ERROR Missing mac parameter");
            return;
        }
        
        uint8_t mac_bytes[6];
        if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                   &mac_bytes[0], &mac_bytes[1], &mac_bytes[2],
                   &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
            cJSON_Delete(json);
            g_send_response("ERROR Invalid MAC address");
            return;
        }
        
        uint16_t device_id = (mac_bytes[4] << 8) | mac_bytes[5];
        esp_err_t ret = device_registry_remove(device_id);
        cJSON_Delete(json);
        
        if (ret != ESP_OK) {
            g_send_response("ERROR Device not found");
            return;
        }
        
        ESP_LOGI(TAG, "Device unpaired: 0x%04X", device_id);
        g_send_response("OK Device unpaired successfully");
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

    if (strncmp(command_line, "FIRMWARE_UPGRADE ", 17) == 0) {
        size_t size = atoi(command_line + 17);
        if (size == 0 || size > 4*1024*1024) {
            g_send_response("ERROR Invalid size");
            return;
        }
        
        // USB-UART/USB-CDC: Use XMODEM-1K
        // BLE: Intercepted by ble_ota_handle_control() before reaching here
        g_send_response("OK Ready for XMODEM");
        vTaskDelay(pdMS_TO_TICKS(100));
        
        esp_err_t ret = xmodem_receive(size);
        if (ret == ESP_OK) {
            const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
            if (!update_partition) {
                g_send_response("ERROR No update partition found");
                return;
            }
            
            ESP_LOGI(TAG, "Setting boot partition: %s (0x%lx)", 
                     update_partition->label, update_partition->address);
            
            ret = esp_ota_set_boot_partition(update_partition);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(ret));
                g_send_response("ERROR Failed to set boot partition");
                return;
            }
            
            ESP_LOGW(TAG, "Boot partition set. Device will boot from %s after restart", 
                     update_partition->label);
            g_send_response("OK Firmware uploaded, rebooting...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        } else {
            ESP_LOGE(TAG, "XMODEM upload failed: %s", esp_err_to_name(ret));
            g_send_response("ERROR Upload failed");
        }
        return;
    }

    g_send_response("ERROR Unknown command");
}
