/**
 * @file device_registry.c
 * @brief Device registry implementation with NVS storage
 *
 * CONTEXT: Multi-sender management for LoRaCue PC receiver
 * STORAGE: NVS namespace "devices" for persistent storage
 */

#include "device_registry.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG           = "DEVICE_REGISTRY";
static const char *NVS_NAMESPACE = "devices";

static bool registry_initialized = false;
static nvs_handle_t registry_nvs_handle;

esp_err_t device_registry_init(void)
{
    ESP_LOGI(TAG, "Initializing device registry");

    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &registry_nvs_handle);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "NVS namespace not found, will be created on first device pairing");
        registry_initialized = true;
        return ESP_OK;
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    registry_initialized = true;
    ESP_LOGI(TAG, "Device registry initialized with existing data");

    return ESP_OK;
}

static void generate_device_key(uint16_t device_id, char *key_name, size_t key_name_size)
{
    snprintf(key_name, key_name_size, "dev_%04X", device_id);
}

esp_err_t device_registry_add(uint16_t device_id, const char *device_name, const uint8_t *mac_address,
                              const uint8_t *aes_key)
{
    if (!registry_initialized) {
        ESP_LOGE(TAG, "Registry not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Open NVS handle if not already open (lazy initialization)
    if (registry_nvs_handle == 0) {
        esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &registry_nvs_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    ESP_LOGI(TAG, "Adding device 0x%04X: %s", device_id, device_name);

    // Check if device already exists
    if (device_registry_is_paired(device_id)) {
        ESP_LOGW(TAG, "Device 0x%04X already paired, updating", device_id);
    }

    // Create device entry
    paired_device_t device = {
        .device_id     = device_id,
        .last_sequence = 0,
        .last_seen     = esp_timer_get_time() / 1000, // Convert to milliseconds
        .is_active     = true,
    };

    strncpy(device.device_name, device_name, DEVICE_NAME_MAX_LEN - 1);
    device.device_name[DEVICE_NAME_MAX_LEN - 1] = '\0';

    memcpy(device.mac_address, mac_address, DEVICE_MAC_ADDR_LEN);
    memcpy(device.aes_key, aes_key, DEVICE_AES_KEY_LEN);

    // Store in NVS
    char key_name[16];
    generate_device_key(device_id, key_name, sizeof(key_name));

    esp_err_t ret = nvs_set_blob(registry_nvs_handle, key_name, &device, sizeof(device));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store device: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_commit(registry_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Device 0x%04X added successfully", device_id);
    return ESP_OK;
}

esp_err_t device_registry_get(uint16_t device_id, paired_device_t *device)
{
    if (!registry_initialized) {
        ESP_LOGE(TAG, "Registry not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // If NVS handle not open, no devices exist yet
    if (registry_nvs_handle == 0) {
        return ESP_ERR_NOT_FOUND;
    }

    char key_name[16];
    generate_device_key(device_id, key_name, sizeof(key_name));

    size_t required_size = sizeof(paired_device_t);
    esp_err_t ret        = nvs_get_blob(registry_nvs_handle, key_name, device, &required_size);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_ERR_NOT_FOUND;
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get device: %s", esp_err_to_name(ret));
        return ret;
    }

    if (!device->is_active) {
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

esp_err_t device_registry_update_last_seen(uint16_t device_id, uint16_t sequence_num)
{
    paired_device_t device;
    esp_err_t ret = device_registry_get(device_id, &device);
    if (ret != ESP_OK) {
        return ret;
    }

    device.last_sequence = sequence_num;
    device.last_seen     = esp_timer_get_time() / 1000;

    char key_name[16];
    generate_device_key(device_id, key_name, sizeof(key_name));

    ret = nvs_set_blob(registry_nvs_handle, key_name, &device, sizeof(device));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update device: %s", esp_err_to_name(ret));
        return ret;
    }

    return nvs_commit(registry_nvs_handle);
}

esp_err_t device_registry_remove(uint16_t device_id)
{
    if (!registry_initialized) {
        ESP_LOGE(TAG, "Registry not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // If NVS handle not open, device doesn't exist
    if (registry_nvs_handle == 0) {
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Removing device 0x%04X", device_id);

    char key_name[16];
    generate_device_key(device_id, key_name, sizeof(key_name));

    esp_err_t ret = nvs_erase_key(registry_nvs_handle, key_name);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to remove device: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_commit(registry_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Device 0x%04X removed successfully", device_id);
    return ESP_OK;
}

esp_err_t device_registry_list(paired_device_t *devices, size_t max_devices, size_t *count)
{
    if (!registry_initialized) {
        ESP_LOGE(TAG, "Registry not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    *count = 0;

    // Simple approach: try to read devices with sequential IDs
    // In a real implementation, we'd maintain a device list in NVS
    for (uint16_t id = 0x1000; id < 0x2000 && *count < max_devices; id++) {
        paired_device_t device;
        if (device_registry_get(id, &device) == ESP_OK) {
            memcpy(&devices[*count], &device, sizeof(paired_device_t));
            (*count)++;
        }
    }

    ESP_LOGI(TAG, "Listed %d paired devices", *count);
    return ESP_OK;
}

bool device_registry_is_paired(uint16_t device_id)
{
    paired_device_t device;
    return (device_registry_get(device_id, &device) == ESP_OK);
}

size_t device_registry_get_count(void)
{
    paired_device_t devices[MAX_PAIRED_DEVICES];
    size_t count;

    if (device_registry_list(devices, MAX_PAIRED_DEVICES, &count) == ESP_OK) {
        return count;
    }

    return 0;
}
