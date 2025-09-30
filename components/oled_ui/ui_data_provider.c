#include "ui_data_provider.h"
#include "power_mgmt.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "ui_data_provider";
static ui_status_t cached_status;
static bool status_valid = false;

static void generate_device_name(char* device_name, size_t size) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    device_name[0] = 'R';
    device_name[1] = 'C';
    device_name[2] = '-';
    device_name[3] = "0123456789ABCDEF"[mac[4] >> 4];
    device_name[4] = "0123456789ABCDEF"[mac[4] & 0xF];
    device_name[5] = "0123456789ABCDEF"[mac[5] >> 4];
    device_name[6] = "0123456789ABCDEF"[mac[5] & 0xF];
    device_name[7] = '\0';
}

esp_err_t ui_data_provider_init(void) {
    ESP_LOGI(TAG, "Initializing UI data provider");
    
    // Generate device name
    generate_device_name(cached_status.device_name, sizeof(cached_status.device_name));
    
    // Initialize with safe defaults
    cached_status.usb_connected = false;
    cached_status.lora_connected = false;
    cached_status.signal_strength = SIGNAL_NONE;
    cached_status.battery_level = 0;
    
    status_valid = true;
    ESP_LOGI(TAG, "Data provider initialized for device: %s", cached_status.device_name);
    
    return ESP_OK;
}

esp_err_t ui_data_provider_update(void) {
    if (!status_valid) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Update battery level from power management
    power_stats_t power_stats;
    esp_err_t ret = power_mgmt_get_stats(&power_stats);
    if (ret == ESP_OK) {
        // For now, simulate battery level since power_stats_t doesn't have it
        // TODO: Add battery level to power_stats_t or create separate battery API
        cached_status.battery_level = 75;  // Simulated for now
        cached_status.usb_connected = true;  // Simulated for now
        ESP_LOGD(TAG, "Battery: %d%%, USB: %s", 
                cached_status.battery_level, 
                cached_status.usb_connected ? "connected" : "disconnected");
    } else {
        ESP_LOGW(TAG, "Failed to get power stats: %s", esp_err_to_name(ret));
        // Keep previous values on error
    }
    
    // TODO: Update LoRa status when LoRa component is available
    // For now, simulate based on system state
    cached_status.lora_connected = true;  // Assume connected for demo
    cached_status.signal_strength = SIGNAL_STRONG;
    
    return ESP_OK;
}

const ui_status_t* ui_data_provider_get_status(void) {
    if (!status_valid) {
        ESP_LOGE(TAG, "Data provider not initialized");
        return NULL;
    }
    
    return &cached_status;
}

esp_err_t ui_data_provider_force_update(bool usb_connected, bool lora_connected, uint8_t battery_level) {
    if (!status_valid) {
        return ESP_ERR_INVALID_STATE;
    }
    
    cached_status.usb_connected = usb_connected;
    cached_status.lora_connected = lora_connected;
    cached_status.battery_level = battery_level;
    cached_status.signal_strength = lora_connected ? SIGNAL_STRONG : SIGNAL_NONE;
    
    ESP_LOGI(TAG, "Status force updated: USB=%d, LoRa=%d, Battery=%d%%", 
             usb_connected, lora_connected, battery_level);
    
    return ESP_OK;
}
