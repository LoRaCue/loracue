/**
 * @file web_config.c
 * @brief Minimal web configuration implementation
 * 
 * CONTEXT: Simplified implementation for Ticket 6.1 validation
 * PURPOSE: Basic web interface placeholder
 */

#include "web_config.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "WEB_CONFIG";

// Web config state
static bool web_config_initialized = false;
static web_config_state_t current_state = WEB_CONFIG_STOPPED;
static web_config_settings_t config_settings;
static device_config_t device_config;
static uint8_t connected_clients = 0;

// Default settings
static const web_config_settings_t default_settings = {
    .ap_ssid = WEB_CONFIG_DEFAULT_SSID,
    .ap_password = WEB_CONFIG_DEFAULT_PASSWORD,
    .ap_channel = 6,
    .max_connections = 4,
    .server_port = WEB_CONFIG_SERVER_PORT,
    .enable_ota = true,
};

// Default device config
static const device_config_t default_device_config = {
    .device_name = "LoRaCue-Device",
    .lora_power = 14,
    .lora_frequency = 915000000,
    .lora_spreading_factor = 7,
    .sleep_timeout_ms = 300000,
    .auto_sleep_enabled = true,
    .display_brightness = 128,
};

esp_err_t web_config_init(const web_config_settings_t *settings)
{
    ESP_LOGI(TAG, "Initializing web configuration system (placeholder)");
    
    if (settings) {
        config_settings = *settings;
    } else {
        config_settings = default_settings;
    }
    
    // Load device config from NVS or use defaults
    device_config = default_device_config;
    web_config_get_device_config(&device_config);
    
    web_config_initialized = true;
    ESP_LOGI(TAG, "Web configuration initialized");
    
    return ESP_OK;
}

esp_err_t web_config_start(void)
{
    if (!web_config_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting web configuration mode (placeholder)");
    current_state = WEB_CONFIG_RUNNING;
    
    ESP_LOGI(TAG, "Web configuration started - Connect to '%s' with password '%s'", 
             config_settings.ap_ssid, config_settings.ap_password);
    ESP_LOGI(TAG, "Open browser to 192.168.4.1 for configuration");
    
    return ESP_OK;
}

esp_err_t web_config_stop(void)
{
    ESP_LOGI(TAG, "Stopping web configuration mode");
    
    connected_clients = 0;
    current_state = WEB_CONFIG_STOPPED;
    
    ESP_LOGI(TAG, "Web configuration stopped");
    return ESP_OK;
}

web_config_state_t web_config_get_state(void)
{
    return current_state;
}

esp_err_t web_config_get_device_config(device_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Try to load from NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("device_config", NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        size_t required_size = sizeof(device_config_t);
        ret = nvs_get_blob(nvs_handle, "config", config, &required_size);
        nvs_close(nvs_handle);
        
        if (ret == ESP_OK) {
            return ESP_OK;
        }
    }
    
    // Use defaults if NVS read failed
    *config = default_device_config;
    return ESP_OK;
}

esp_err_t web_config_set_device_config(const device_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Save to NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("device_config", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = nvs_set_blob(nvs_handle, "config", config, sizeof(device_config_t));
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }
    
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        device_config = *config;
        ESP_LOGI(TAG, "Device configuration saved");
    }
    
    return ret;
}

esp_err_t web_config_get_ap_info(char *ssid, char *password, char *ip_address)
{
    if (!ssid || !password || !ip_address) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strcpy(ssid, config_settings.ap_ssid);
    strcpy(password, config_settings.ap_password);
    strcpy(ip_address, "192.168.4.1"); // Default AP IP
    
    return ESP_OK;
}

uint8_t web_config_get_client_count(void)
{
    return connected_clients;
}
