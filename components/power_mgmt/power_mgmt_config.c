#include "power_mgmt_config.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "PWR_CFG";

static const power_mgmt_config_t default_config = {
    .display_sleep_enabled    = true,
    .display_sleep_timeout_ms = 30000,
    .light_sleep_enabled      = true,
    .light_sleep_timeout_ms   = 180000,
    .deep_sleep_enabled       = true,
    .deep_sleep_timeout_ms    = 360000,
};

static power_mgmt_config_t cached_config;
static bool cache_valid = false;

esp_err_t power_mgmt_config_init(void)
{
    ESP_LOGI(TAG, "Initializing power management configuration");
    cache_valid = false;
    return ESP_OK;
}

esp_err_t power_mgmt_config_get(power_mgmt_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    if (cache_valid) {
        memcpy(config, &cached_config, sizeof(power_mgmt_config_t));
        return ESP_OK;
    }

    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("powermanagement", NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        size_t required_size = sizeof(power_mgmt_config_t);
        ret                  = nvs_get_blob(nvs_handle, "config", config, &required_size);
        nvs_close(nvs_handle);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Power config loaded from NVS");
            memcpy(&cached_config, config, sizeof(power_mgmt_config_t));
            cache_valid = true;
            return ESP_OK;
        }
    }

    *config = default_config;
    ESP_LOGI(TAG, "Using default power config");
    return ESP_OK;
}

esp_err_t power_mgmt_config_set(const power_mgmt_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("powermanagement", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_blob(nvs_handle, "config", config, sizeof(power_mgmt_config_t));
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Power configuration saved to NVS");
        memcpy(&cached_config, config, sizeof(power_mgmt_config_t));
        cache_valid = true;
    }

    return ret;
}
