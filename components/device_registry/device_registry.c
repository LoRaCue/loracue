/**
 * @file device_registry.c
 * @brief Simplified device registry implementation using config_manager
 */

#include "device_registry.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "DEVICE_REGISTRY";

// Runtime sequence tracking (RAM-only, not persisted)
static paired_device_t runtime_devices[MAX_PAIRED_DEVICES];
static bool runtime_loaded = false;

static void load_runtime_data(void)
{
    if (runtime_loaded) return;
    
    device_registry_config_t config;
    esp_err_t ret = config_manager_get_device_registry(&config);
    if (ret == ESP_OK) {
        size_t count = (config.device_count < MAX_PAIRED_DEVICES) ? config.device_count : MAX_PAIRED_DEVICES;
        memcpy(runtime_devices, config.devices, count * sizeof(paired_device_t));
    }
    runtime_loaded = true;
}

esp_err_t device_registry_init(void)
{
    ESP_LOGI(TAG, "Initializing simplified device registry (max %d devices)", MAX_PAIRED_DEVICES);
    load_runtime_data();
    return ESP_OK;
}

esp_err_t device_registry_add(uint16_t device_id, const char *device_name, const uint8_t *mac_address,
                              const uint8_t *aes_key)
{
    if (!device_name || !mac_address || !aes_key) {
        return ESP_ERR_INVALID_ARG;
    }

    load_runtime_data();

    // Get current config
    device_registry_config_t config;
    esp_err_t ret = config_manager_get_device_registry(&config);
    if (ret != ESP_OK) {
        memset(&config, 0, sizeof(config));
    }

    // Check if device already exists (update case)
    int existing_index = -1;
    for (size_t i = 0; i < config.device_count; i++) {
        if (config.devices[i].device_id == device_id) {
            existing_index = i;
            break;
        }
    }

    if (existing_index >= 0) {
        // Update existing device
        strncpy(config.devices[existing_index].device_name, device_name, DEVICE_NAME_MAX_LEN - 1);
        config.devices[existing_index].device_name[DEVICE_NAME_MAX_LEN - 1] = '\0';
        memcpy(config.devices[existing_index].mac_address, mac_address, DEVICE_MAC_ADDR_LEN);
        memcpy(config.devices[existing_index].aes_key, aes_key, DEVICE_AES_KEY_LEN);
    } else {
        // Add new device
        if (config.device_count >= MAX_PAIRED_DEVICES) {
            ESP_LOGE(TAG, "Registry full (max %d devices)", MAX_PAIRED_DEVICES);
            return ESP_ERR_NO_MEM;
        }

        paired_device_t *new_device = &config.devices[config.device_count];
        new_device->device_id = device_id;
        strncpy(new_device->device_name, device_name, DEVICE_NAME_MAX_LEN - 1);
        new_device->device_name[DEVICE_NAME_MAX_LEN - 1] = '\0';
        memcpy(new_device->mac_address, mac_address, DEVICE_MAC_ADDR_LEN);
        memcpy(new_device->aes_key, aes_key, DEVICE_AES_KEY_LEN);
        new_device->highest_sequence = 0;
        new_device->recent_bitmap = 0;
        config.device_count++;
    }

    // Save to NVS
    ret = config_manager_set_device_registry(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save device registry");
        return ret;
    }

    // Update runtime data
    memcpy(runtime_devices, config.devices, config.device_count * sizeof(paired_device_t));

    ESP_LOGI(TAG, "Device 0x%04X (%s) %s", device_id, device_name, 
             (existing_index >= 0) ? "updated" : "added");
    return ESP_OK;
}

esp_err_t device_registry_get(uint16_t device_id, paired_device_t *device)
{
    if (!device) {
        return ESP_ERR_INVALID_ARG;
    }

    load_runtime_data();

    // Linear search through runtime data (includes sequence tracking)
    for (size_t i = 0; i < MAX_PAIRED_DEVICES; i++) {
        if (runtime_devices[i].device_id == device_id && runtime_devices[i].device_id != 0) {
            memcpy(device, &runtime_devices[i], sizeof(paired_device_t));
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t device_registry_update_sequence(uint16_t device_id, uint16_t highest_sequence, uint64_t recent_bitmap)
{
    load_runtime_data();

    // Update runtime data only (not persisted to save flash wear)
    for (size_t i = 0; i < MAX_PAIRED_DEVICES; i++) {
        if (runtime_devices[i].device_id == device_id) {
            runtime_devices[i].highest_sequence = highest_sequence;
            runtime_devices[i].recent_bitmap = recent_bitmap;
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t device_registry_remove(uint16_t device_id)
{
    load_runtime_data();

    device_registry_config_t config;
    esp_err_t ret = config_manager_get_device_registry(&config);
    if (ret != ESP_OK) {
        return ret;
    }

    // Find and remove device
    bool found = false;
    for (size_t i = 0; i < config.device_count; i++) {
        if (config.devices[i].device_id == device_id) {
            // Shift remaining devices
            memmove(&config.devices[i], &config.devices[i + 1], 
                    (config.device_count - i - 1) * sizeof(paired_device_t));
            config.device_count--;
            found = true;
            break;
        }
    }

    if (!found) {
        return ESP_ERR_NOT_FOUND;
    }

    // Save updated config
    ret = config_manager_set_device_registry(&config);
    if (ret != ESP_OK) {
        return ret;
    }

    // Update runtime data
    memset(runtime_devices, 0, sizeof(runtime_devices));
    memcpy(runtime_devices, config.devices, config.device_count * sizeof(paired_device_t));

    ESP_LOGI(TAG, "Device 0x%04X removed", device_id);
    return ESP_OK;
}

esp_err_t device_registry_list(paired_device_t *devices, size_t max_devices, size_t *count)
{
    if (!devices || !count) {
        return ESP_ERR_INVALID_ARG;
    }

    device_registry_config_t config;
    esp_err_t ret = config_manager_get_device_registry(&config);
    if (ret != ESP_OK) {
        *count = 0;
        return ESP_OK;
    }

    *count = (config.device_count < max_devices) ? config.device_count : max_devices;
    memcpy(devices, config.devices, *count * sizeof(paired_device_t));

    return ESP_OK;
}

bool device_registry_is_paired(uint16_t device_id)
{
    paired_device_t device;
    return (device_registry_get(device_id, &device) == ESP_OK);
}

size_t device_registry_get_count(void)
{
    device_registry_config_t config;
    if (config_manager_get_device_registry(&config) != ESP_OK) {
        return 0;
    }
    return config.device_count;
}
