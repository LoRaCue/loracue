/**
 * @file bluetooth_config_stub.c
 * @brief Stub implementation when Bluetooth is disabled
 */

#include "bluetooth_config.h"
#include "esp_log.h"

static const char *TAG = "bluetooth_config";

esp_err_t bluetooth_config_init(void)
{
    ESP_LOGW(TAG, "Bluetooth not enabled in configuration");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bluetooth_config_deinit(void)
{
    return ESP_OK;
}

bool bluetooth_config_is_connected(void)
{
    return false;
}

bool bluetooth_config_get_passkey(uint32_t *passkey)
{
    if (passkey) {
        *passkey = 0;
    }
    return false;
}

esp_err_t bluetooth_config_set_enabled(bool enabled)
{
    (void)enabled;
    return ESP_ERR_NOT_SUPPORTED;
}
