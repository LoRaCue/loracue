/**
 * @file device_config.c
 * @brief Device configuration management with NVS persistence
 *
 * CONTEXT: Clean device configuration management
 * PURPOSE: Store and retrieve device settings from NVS
 */

#include "device_config.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "DEVICE_CONFIG";

// Default device configuration
static const device_config_t default_config = {
    .device_name        = "LoRaCue-Device",
    .device_mode        = DEVICE_MODE_PRESENTER,
    .display_brightness = 128,
    .bluetooth_enabled  = true,
    .slot_id            = 1,
};

// Cached configuration
static device_config_t cached_config;
static bool cache_valid = false;

esp_err_t device_config_init(void)
{
    ESP_LOGI(TAG, "Initializing device configuration system");
    cache_valid = false;
    return ESP_OK;
}

esp_err_t device_config_get(device_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Return cached config if valid
    if (cache_valid) {
        memcpy(config, &cached_config, sizeof(device_config_t));
        return ESP_OK;
    }

    // Try to load from NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("general", NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        size_t required_size = sizeof(device_config_t);
        ret                  = nvs_get_blob(nvs_handle, "config", config, &required_size);
        nvs_close(nvs_handle);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Device config loaded from NVS - mode: %s", device_mode_to_string(config->device_mode));
            // Cache the loaded config
            memcpy(&cached_config, config, sizeof(device_config_t));
            cache_valid = true;
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "Failed to read config from NVS: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGW(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
    }

    // Use defaults if NVS read failed
    *config = default_config;

    // Generate device name from MAC address (LC-XXXX)
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(config->device_name, sizeof(config->device_name), "LC-%02X%02X", mac[4], mac[5]);

    ESP_LOGI(TAG, "Using default device configuration - name: %s, mode: %s", config->device_name,
             device_mode_to_string(config->device_mode));

    // Cache the default config
    memcpy(&cached_config, config, sizeof(device_config_t));
    cache_valid = true;

    return ESP_OK;
}

const char *device_mode_to_string(device_mode_t mode)
{
    switch (mode) {
        case DEVICE_MODE_PRESENTER:
            return "PRESENTER";
        case DEVICE_MODE_PC:
            return "PC";
        default:
            return "UNKNOWN";
    }
}

esp_err_t device_config_set(const device_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Save to NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("general", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_blob(nvs_handle, "config", config, sizeof(device_config_t));
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Device configuration saved to NVS");
        // Update cache with new config
        memcpy(&cached_config, config, sizeof(device_config_t));
        cache_valid = true;
    } else {
        ESP_LOGE(TAG, "Failed to save device config: %s", esp_err_to_name(ret));
        // Invalidate cache on error
        cache_valid = false;
    }

    return ret;
}

esp_err_t device_config_factory_reset(void)
{
    ESP_LOGW(TAG, "Factory reset initiated - erasing all NVS data");

    // Erase entire NVS partition
    esp_err_t ret = nvs_flash_erase();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "NVS erased successfully, rebooting...");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Give time for log output

    // Reboot device
    esp_restart();

    return ESP_OK; // Will never reach here
}
