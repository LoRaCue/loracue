/**
 * @file device_config.c
 * @brief Device configuration management with NVS persistence
 * 
 * CONTEXT: Clean device configuration management
 * PURPOSE: Store and retrieve device settings from NVS
 */

#include "device_config.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "DEVICE_CONFIG";

// Default device configuration
static const device_config_t default_config = {
    .device_name = "LoRaCue-Device",
    .device_mode = DEVICE_MODE_PRESENTER,
    .sleep_timeout_ms = 300000,     // 5 minutes
    .auto_sleep_enabled = true,
    .display_brightness = 128,      // 50% brightness
};

esp_err_t device_config_init(void)
{
    ESP_LOGI(TAG, "Initializing device configuration system");
    return ESP_OK;
}

esp_err_t device_config_get(device_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Try to load from NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("general", NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        size_t required_size = sizeof(device_config_t);
        ret = nvs_get_blob(nvs_handle, "config", config, &required_size);
        nvs_close(nvs_handle);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Device config loaded from NVS - mode: %s", 
                     device_mode_to_string(config->device_mode));
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "Failed to read config from NVS: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGW(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
    }
    
    // Use defaults if NVS read failed
    *config = default_config;
    ESP_LOGI(TAG, "Using default device configuration - mode: %s", 
             device_mode_to_string(config->device_mode));
    return ESP_OK;
}

const char* device_mode_to_string(device_mode_t mode)
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
        ESP_LOGI(TAG, "Device configuration saved to NVS - mode: %s", 
                 device_mode_to_string(config->device_mode));
    } else {
        ESP_LOGE(TAG, "Failed to save device config: %s", esp_err_to_name(ret));
    }
    
    return ret;
}
