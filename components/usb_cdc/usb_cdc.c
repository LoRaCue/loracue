#include "usb_cdc.h"
#include "esp_log.h"
#include "cJSON.h"
#include "tinyusb.h"
#include "tusb.h"
#include "class/cdc/cdc_device.h"
#include "device_config.h"
#include "device_registry.h"
#include "lora_driver.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_app_format.h"
#include "u8g2.h"
#include <string.h>

static const char *TAG = "USB_CDC";
static char rx_buffer[512];
static size_t rx_len = 0;

// OTA update state
static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *ota_partition = NULL;
static size_t ota_received_bytes = 0;
static size_t ota_total_size = 0;

static void send_response(const char *response)
{
    tud_cdc_write_str(response);
    tud_cdc_write_str("\n");
    tud_cdc_write_flush();
}

static void handle_ping(void)
{
    send_response("PONG");
}

static void handle_get_version(void)
{
    send_response("v0.1.0-alpha.39");
}

static void handle_get_device_name(void)
{
    device_config_t config;
    device_config_get(&config);
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "device_name", config.device_name);
    
    char *json_string = cJSON_Print(response);
    send_response(json_string);
    free(json_string);
    cJSON_Delete(response);
}

static void handle_set_device_name(const char *name)
{
    if (!name || strlen(name) == 0 || strlen(name) >= 32) {
        send_response("ERROR Invalid device name");
        return;
    }
    
    device_config_t config;
    device_config_get(&config);
    strncpy(config.device_name, name, sizeof(config.device_name) - 1);
    config.device_name[sizeof(config.device_name) - 1] = '\0';
    
    esp_err_t ret = device_config_set(&config);
    if (ret == ESP_OK) {
        send_response("OK Device name updated");
    } else {
        send_response("ERROR Failed to save device name");
    }
}

static void handle_set_brightness(const char *value_str)
{
    int brightness = atoi(value_str);
    if (brightness < 0 || brightness > 255) {
        send_response("ERROR Brightness must be 0-255");
        return;
    }
    
    device_config_t config;
    device_config_get(&config);
    config.display_brightness = (uint8_t)brightness;
    
    esp_err_t ret = device_config_set(&config);
    if (ret != ESP_OK) {
        send_response("ERROR Failed to save brightness");
        return;
    }
    
    // Apply immediately
    extern u8g2_t u8g2;
    u8g2_SetContrast(&u8g2, config.display_brightness);
    
    send_response("OK Brightness updated");
}

static void handle_get_lora_config(void)
{
    lora_config_t config;
    esp_err_t ret = lora_get_config(&config);
    
    if (ret != ESP_OK) {
        send_response("ERROR Failed to get LoRa config");
        return;
    }
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "frequency", config.frequency);
    cJSON_AddNumberToObject(response, "spreading_factor", config.spreading_factor);
    cJSON_AddNumberToObject(response, "bandwidth", config.bandwidth);
    cJSON_AddNumberToObject(response, "coding_rate", config.coding_rate);
    cJSON_AddNumberToObject(response, "tx_power", config.tx_power);
    
    char *json_string = cJSON_Print(response);
    send_response(json_string);
    free(json_string);
    cJSON_Delete(response);
}

static void handle_set_lora_config(cJSON *config_json)
{
    lora_config_t config;
    esp_err_t ret = lora_get_config(&config);
    
    if (ret != ESP_OK) {
        send_response("ERROR Failed to get current LoRa config");
        return;
    }
    
    cJSON *freq = cJSON_GetObjectItem(config_json, "frequency");
    cJSON *sf = cJSON_GetObjectItem(config_json, "spreading_factor");
    cJSON *bw = cJSON_GetObjectItem(config_json, "bandwidth");
    cJSON *cr = cJSON_GetObjectItem(config_json, "coding_rate");
    cJSON *power = cJSON_GetObjectItem(config_json, "tx_power");
    
    if (cJSON_IsNumber(freq)) config.frequency = freq->valueint;
    if (cJSON_IsNumber(sf)) config.spreading_factor = sf->valueint;
    if (cJSON_IsNumber(bw)) config.bandwidth = bw->valueint;
    if (cJSON_IsNumber(cr)) config.coding_rate = cr->valueint;
    if (cJSON_IsNumber(power)) config.tx_power = power->valueint;
    
    ret = lora_set_config(&config);
    if (ret == ESP_OK) {
        send_response("OK LoRa configuration updated");
    } else {
        send_response("ERROR Failed to update LoRa configuration");
    }
}

static void handle_get_paired_devices(void)
{
    cJSON *devices_array = cJSON_CreateArray();
    
    // Get all paired devices from registry
    for (int i = 0; i < 10; i++) { // Assume max 10 devices
        paired_device_t device;
        uint16_t device_id = i + 1; // Simple iteration
        
        if (device_registry_get(device_id, &device) == ESP_OK) {
            cJSON *device_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(device_obj, "name", device.device_name);
            
            char mac_str[18];
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                    device.mac_address[0], device.mac_address[1], device.mac_address[2],
                    device.mac_address[3], device.mac_address[4], device.mac_address[5]);
            cJSON_AddStringToObject(device_obj, "mac", mac_str);
            
            cJSON_AddItemToArray(devices_array, device_obj);
        }
    }
    
    char *json_string = cJSON_Print(devices_array);
    send_response(json_string);
    free(json_string);
    cJSON_Delete(devices_array);
}

static void handle_pair_device(cJSON *pair_json)
{
    cJSON *name = cJSON_GetObjectItem(pair_json, "name");
    cJSON *mac = cJSON_GetObjectItem(pair_json, "mac");
    cJSON *key = cJSON_GetObjectItem(pair_json, "key");
    
    if (!name || !mac || !key) {
        send_response("ERROR Missing pairing parameters");
        return;
    }
    
    // Parse MAC address
    uint8_t mac_bytes[6];
    if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
               &mac_bytes[0], &mac_bytes[1], &mac_bytes[2],
               &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
        send_response("ERROR Invalid MAC address");
        return;
    }
    
    // Parse AES-256 key (64 hex chars = 32 bytes)
    uint8_t aes_key[32];
    const char *key_str = key->valuestring;
    if (strlen(key_str) != 64) {
        send_response("ERROR Invalid key length (expected 64 hex chars for AES-256)");
        return;
    }
    
    for (int i = 0; i < 32; i++) {
        if (sscanf(key_str + i*2, "%02hhx", &aes_key[i]) != 1) {
            send_response("ERROR Invalid key format");
            return;
        }
    }
    
    // Generate device ID from MAC
    uint16_t device_id = (mac_bytes[4] << 8) | mac_bytes[5];
    
    // Add to registry
    esp_err_t ret = device_registry_add(device_id, name->valuestring, mac_bytes, aes_key);
    if (ret != ESP_OK) {
        send_response("ERROR Failed to add device");
        return;
    }
    
    ESP_LOGI(TAG, "Device paired: 0x%04X (%s)", device_id, name->valuestring);
    send_response("OK Device paired successfully");
}

static void handle_fw_update_start(cJSON *update_json)
{
    cJSON *size = cJSON_GetObjectItem(update_json, "size");
    cJSON *checksum = cJSON_GetObjectItem(update_json, "checksum");
    cJSON *version = cJSON_GetObjectItem(update_json, "version");
    
    if (!cJSON_IsNumber(size) || !cJSON_IsString(checksum) || !cJSON_IsString(version)) {
        send_response("ERROR Invalid firmware update parameters");
        return;
    }
    
    // Clean up any existing OTA session
    if (ota_handle) {
        esp_ota_abort(ota_handle);
        ota_handle = 0;
    }
    
    ota_total_size = size->valueint;
    ota_received_bytes = 0;
    
    // Get next OTA partition
    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        send_response("ERROR No OTA partition available");
        return;
    }
    
    // Begin OTA update
    esp_err_t ret = esp_ota_begin(ota_partition, ota_total_size, &ota_handle);
    if (ret != ESP_OK) {
        send_response("ERROR Failed to begin OTA update");
        return;
    }
    
    ESP_LOGI(TAG, "OTA update started: %d bytes, version %s", ota_total_size, version->valuestring);
    send_response("OK Ready for firmware data");
}

static void handle_fw_update_data(const char *data_str)
{
    if (!ota_handle) {
        send_response("ERROR No active OTA session");
        return;
    }
    
    // Parse chunk number and hex data
    char *space_pos = strchr(data_str, ' ');
    if (!space_pos) {
        send_response("ERROR Invalid data format");
        return;
    }
    
    // Skip chunk number parsing for now - just use hex data
    const char *hex_data = space_pos + 1;
    
    // Convert hex string to binary
    size_t hex_len = strlen(hex_data);
    if (hex_len % 2 != 0) {
        send_response("ERROR Invalid hex data length");
        return;
    }
    
    size_t data_len = hex_len / 2;
    uint8_t *binary_data = malloc(data_len);
    if (!binary_data) {
        send_response("ERROR Memory allocation failed");
        return;
    }
    
    for (size_t i = 0; i < data_len; i++) {
        if (sscanf(hex_data + i * 2, "%02hhx", &binary_data[i]) != 1) {
            free(binary_data);
            send_response("ERROR Invalid hex data");
            return;
        }
    }
    
    esp_err_t ret = esp_ota_write(ota_handle, binary_data, data_len);
    free(binary_data);
    
    if (ret != ESP_OK) {
        send_response("ERROR Failed to write OTA data");
        return;
    }
    
    ota_received_bytes += data_len;
    send_response("OK Data written");
}

static void handle_fw_update_verify(void)
{
    if (!ota_handle) {
        send_response("ERROR No active OTA session");
        return;
    }
    
    esp_err_t ret = esp_ota_end(ota_handle);
    if (ret != ESP_OK) {
        send_response("ERROR OTA verification failed");
        ota_handle = 0;
        return;
    }
    
    send_response("OK Firmware verified");
}

static void handle_fw_update_commit(void)
{
    if (!ota_partition) {
        send_response("ERROR No OTA partition to commit");
        return;
    }
    
    esp_err_t ret = esp_ota_set_boot_partition(ota_partition);
    if (ret != ESP_OK) {
        send_response("ERROR Failed to set boot partition");
        return;
    }
    
    send_response("OK Firmware committed - rebooting");
    
    // Clean up
    ota_handle = 0;
    ota_partition = NULL;
    ota_received_bytes = 0;
    ota_total_size = 0;
}

static void process_command(const char *command_line)
{
    ESP_LOGI(TAG, "Processing command: %s", command_line);
    
    // Handle simple commands
    if (strcmp(command_line, "PING") == 0) {
        handle_ping();
        return;
    }
    
    if (strcmp(command_line, "GET_VERSION") == 0) {
        handle_get_version();
        return;
    }
    
    if (strcmp(command_line, "GET_DEVICE_NAME") == 0) {
        handle_get_device_name();
        return;
    }
    
    if (strcmp(command_line, "GET_LORA_CONFIG") == 0) {
        handle_get_lora_config();
        return;
    }
    
    if (strcmp(command_line, "GET_PAIRED_DEVICES") == 0) {
        handle_get_paired_devices();
        return;
    }
    
    // Handle commands with parameters
    if (strncmp(command_line, "SET_DEVICE_NAME ", 16) == 0) {
        handle_set_device_name(command_line + 16);
        return;
    }
    if (strncmp(command_line, "SET_BRIGHTNESS ", 15) == 0) {
        handle_set_brightness(command_line + 15);
        return;
    }
    
    // Handle JSON commands
    if (strncmp(command_line, "PAIR_DEVICE ", 12) == 0) {
        cJSON *json = cJSON_Parse(command_line + 12);
        if (json) {
            handle_pair_device(json);
            cJSON_Delete(json);
        } else {
            send_response("ERROR Invalid JSON");
        }
        return;
    }
    
    // Handle JSON commands
    if (strncmp(command_line, "SET_LORA_CONFIG ", 16) == 0) {
        cJSON *json = cJSON_Parse(command_line + 16);
        if (json) {
            handle_set_lora_config(json);
            cJSON_Delete(json);
        } else {
            send_response("ERROR Invalid JSON");
        }
        return;
    }
    
    if (strncmp(command_line, "FW_UPDATE_START ", 16) == 0) {
        cJSON *json = cJSON_Parse(command_line + 16);
        if (json) {
            handle_fw_update_start(json);
            cJSON_Delete(json);
        } else {
            send_response("ERROR Invalid JSON");
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
    
    if (strcmp(command_line, "FW_UPDATE_COMMIT") == 0) {
        handle_fw_update_commit();
        return;
    }
    
    send_response("ERROR Unknown command");
}

void usb_cdc_process_commands(void)
{
    if (!tud_cdc_connected()) {
        return;
    }
    
    while (tud_cdc_available()) {
        char c = tud_cdc_read_char();
        
        if (c == '\n' || c == '\r') {
            if (rx_len > 0) {
                rx_buffer[rx_len] = '\0';
                process_command(rx_buffer);
                rx_len = 0;
            }
        } else if (rx_len < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_len++] = c;
        } else {
            // Buffer overflow, reset
            rx_len = 0;
            send_response("ERROR Command too long");
        }
    }
}

esp_err_t usb_cdc_init(void)
{
    ESP_LOGI(TAG, "USB CDC protocol initialized");
    return ESP_OK;
}
