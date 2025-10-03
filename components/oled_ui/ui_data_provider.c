#include "ui_data_provider.h"
#include "bsp.h"
#include "device_config.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora_driver.h"
#include "lora_protocol.h"
#include "power_mgmt.h"

#ifndef SIMULATOR_BUILD
#include "bluetooth_config.h"
#endif

static const char *TAG = "ui_data_provider";
static ui_status_t cached_status;
static battery_info_t cached_battery;
static bool status_valid = false;

esp_err_t ui_data_provider_init(void)
{
    ESP_LOGI(TAG, "Initializing UI data provider");

    // Load device name from config
    device_config_t config;
    device_config_get(&config);
    strncpy(cached_status.device_name, config.device_name, sizeof(cached_status.device_name) - 1);

    // Initialize with safe defaults
    cached_status.usb_connected       = false;
    cached_status.bluetooth_enabled   = config.bluetooth_enabled;
    cached_status.bluetooth_connected = false;
    cached_status.lora_connected      = false;
    cached_status.signal_strength     = SIGNAL_NONE;
    cached_status.battery_level       = 0;

    status_valid = true;
    ESP_LOGI(TAG, "Data provider initialized for device: %s", cached_status.device_name);

    return ESP_OK;
}

esp_err_t ui_data_provider_update(void)
{
    if (!status_valid) {
        return ESP_ERR_INVALID_STATE;
    }

    // Update real battery data from BSP
    float battery_voltage = bsp_read_battery();
    if (battery_voltage > 0) {
        // Update cached battery info
        cached_battery.voltage       = battery_voltage;
        cached_battery.usb_connected = (battery_voltage > 4.3f);
        cached_battery.charging      = cached_battery.usb_connected && (battery_voltage < 4.2f);

        // Convert voltage to percentage (3.0V = 0%, 4.2V = 100%)
        if (battery_voltage >= 4.2f) {
            cached_battery.percentage = 100;
        } else if (battery_voltage <= 3.0f) {
            cached_battery.percentage = 0;
        } else {
            cached_battery.percentage = (uint8_t)((battery_voltage - 3.0f) / 1.2f * 100);
        }

        // Update main status
        cached_status.battery_level = cached_battery.percentage;
        cached_status.usb_connected = cached_battery.usb_connected;

        ESP_LOGD(TAG, "Battery: %.2fV (%d%%), USB: %s, Charging: %s", battery_voltage, cached_battery.percentage,
                 cached_battery.usb_connected ? "yes" : "no", cached_battery.charging ? "yes" : "no");
    } else {
        ESP_LOGW(TAG, "Failed to read battery voltage");
        // Keep previous values on error
    }

    // Update Bluetooth status
    device_config_t config;
    device_config_get(&config);
    cached_status.bluetooth_enabled = config.bluetooth_enabled;
#ifndef SIMULATOR_BUILD
    cached_status.bluetooth_connected = bluetooth_config_is_connected();
#else
    cached_status.bluetooth_connected = false;
#endif

    // Update LoRa status from protocol layer
    lora_connection_state_t conn_state = lora_protocol_get_connection_state();
    int16_t rssi                       = lora_protocol_get_last_rssi();

    // Map connection state to signal strength
    switch (conn_state) {
        case LORA_CONNECTION_EXCELLENT:
            cached_status.signal_strength = SIGNAL_STRONG;
            cached_status.lora_connected  = true;
            break;
        case LORA_CONNECTION_GOOD:
            cached_status.signal_strength = SIGNAL_GOOD;
            cached_status.lora_connected  = true;
            break;
        case LORA_CONNECTION_WEAK:
            cached_status.signal_strength = SIGNAL_FAIR;
            cached_status.lora_connected  = true;
            break;
        case LORA_CONNECTION_POOR:
            cached_status.signal_strength = SIGNAL_WEAK;
            cached_status.lora_connected  = true;
            break;
        case LORA_CONNECTION_LOST:
        default:
            cached_status.signal_strength = SIGNAL_NONE;
            cached_status.lora_connected  = false;
            break;
    }

    ESP_LOGD(TAG, "LoRa: %s, RSSI: %d dBm", cached_status.lora_connected ? "connected" : "disconnected", rssi);

    return ESP_OK;
}

const ui_status_t *ui_data_provider_get_status(void)
{
    if (!status_valid) {
        ESP_LOGE(TAG, "Data provider not initialized");
        return NULL;
    }

    return &cached_status;
}

esp_err_t ui_data_provider_get_battery_info(battery_info_t *battery_info)
{
    if (!status_valid || !battery_info) {
        return ESP_ERR_INVALID_ARG;
    }

    *battery_info = cached_battery;
    return ESP_OK;
}

esp_err_t ui_data_provider_force_update(bool usb_connected, bool lora_connected, uint8_t battery_level)
{
    if (!status_valid) {
        return ESP_ERR_INVALID_STATE;
    }

    cached_status.usb_connected   = usb_connected;
    cached_status.lora_connected  = lora_connected;
    cached_status.battery_level   = battery_level;
    cached_status.signal_strength = lora_connected ? SIGNAL_STRONG : SIGNAL_NONE;

    ESP_LOGI(TAG, "Status force updated: USB=%d, LoRa=%d, Battery=%d%%", usb_connected, lora_connected, battery_level);

    return ESP_OK;
}
