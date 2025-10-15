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

// RAM cache for fast lookups
static paired_device_t device_cache[MAX_PAIRED_DEVICES];
static size_t cached_device_count = 0;
static bool cache_loaded = false;

static void generate_device_key(uint16_t device_id, char *key_name, size_t key_name_size)
{
    snprintf(key_name, key_name_size, "dev_%04X", device_id);
}

static void load_cache_from_nvs(void)
{
    if (cache_loaded) return;
    
    cached_device_count = 0;
    
    if (registry_nvs_handle == 0) {
        cache_loaded = true;
        return;
    }
    
    nvs_iterator_t it = NULL;
    esp_err_t res = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_NAMESPACE, NVS_TYPE_BLOB, &it);
    
    while (res == ESP_OK && cached_device_count < MAX_PAIRED_DEVICES) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        
        if (strncmp(info.key, "dev_", 4) == 0) {
            uint16_t device_id;
            if (sscanf(info.key + 4, "%04hx", &device_id) == 1) {
                char key_name[16];
                generate_device_key(device_id, key_name, sizeof(key_name));
                size_t size = sizeof(paired_device_t);
                if (nvs_get_blob(registry_nvs_handle, key_name, &device_cache[cached_device_count], &size) == ESP_OK) {
                    cached_device_count++;
                }
            }
        }
        res = nvs_entry_next(&it);
    }
    
    if (it) nvs_release_iterator(it);
    
    cache_loaded = true;
    ESP_LOGI(TAG, "Loaded %d devices into cache", cached_device_count);
}

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

    // Update cache - replace if exists, add if new
    bool found = false;
    for (size_t i = 0; i < cached_device_count; i++) {
        if (device_cache[i].device_id == device_id) {
            memcpy(&device_cache[i], &device, sizeof(paired_device_t));
            found = true;
            break;
        }
    }
    if (!found && cached_device_count < MAX_PAIRED_DEVICES) {
        memcpy(&device_cache[cached_device_count], &device, sizeof(paired_device_t));
        cached_device_count++;
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

    // Load cache on first access
    if (!cache_loaded) {
        load_cache_from_nvs();
    }

    // Fast lookup in RAM cache
    for (size_t i = 0; i < cached_device_count; i++) {
        if (device_cache[i].device_id == device_id) {
            memcpy(device, &device_cache[i], sizeof(paired_device_t));
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t device_registry_update_last_seen(uint16_t device_id, uint16_t sequence_num)
{
    paired_device_t device;
    esp_err_t ret = device_registry_get(device_id, &device);
    if (ret != ESP_OK) {
        return ret;
    }

    device.last_sequence = sequence_num;

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

    // Remove from cache
    for (size_t i = 0; i < cached_device_count; i++) {
        if (device_cache[i].device_id == device_id) {
            // Shift remaining devices
            memmove(&device_cache[i], &device_cache[i + 1], 
                    (cached_device_count - i - 1) * sizeof(paired_device_t));
            cached_device_count--;
            break;
        }
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

    // Load cache on first access
    if (!cache_loaded) {
        load_cache_from_nvs();
    }

    // Copy from cache
    *count = (cached_device_count < max_devices) ? cached_device_count : max_devices;
    memcpy(devices, device_cache, *count * sizeof(paired_device_t));

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
