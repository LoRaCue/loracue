/**
 * @file main.c
 * @brief LoRaCue main application
 * 
 * CONTEXT: LoRaCue enterprise presentation clicker
 * HARDWARE: Heltec LoRa V3 (ESP32-S3 + SX1262 + SH1106)
 * PURPOSE: Main application entry point and system initialization
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "bsp.h"
#include "lora_driver.h"
#include "lora_protocol.h"
#include "usb_hid.h"
#include "device_registry.h"
#include "oled_ui.h"
#include "button_manager.h"
#include "led_manager.h"
#include "power_mgmt.h"
#include "usb_pairing.h"
#include "web_config.h"

static const char *TAG = "LORACUE_MAIN";

// Demo AES keys for different devices
static const uint8_t demo_aes_key_1[16] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static esp_err_t handle_lora_command(lora_command_t command, uint16_t sender_id)
{
    ESP_LOGI(TAG, "🎯 Executing command 0x%02X from device 0x%04X", command, sender_id);
    
    // Update power management activity on LoRa command
    power_mgmt_update_activity();
    
    switch (command) {
        case CMD_NEXT_SLIDE:
            return usb_hid_send_key(HID_KEY_PAGE_DOWN);
            
        case CMD_PREV_SLIDE:
            return usb_hid_send_key(HID_KEY_PAGE_UP);
            
        case CMD_BLACK_SCREEN:
            return usb_hid_send_key(HID_KEY_B);
            
        case CMD_START_PRESENTATION:
            return usb_hid_send_key(HID_KEY_F5);
            
        default:
            ESP_LOGW(TAG, "Unknown command: 0x%02X", command);
            return ESP_ERR_INVALID_ARG;
    }
}

static void pairing_result_callback(bool success, uint16_t device_id, const char* device_name)
{
    if (success) {
        ESP_LOGI(TAG, "🔗 Pairing successful: %s (0x%04X)", device_name, device_id);
    } else {
        ESP_LOGW(TAG, "❌ Pairing failed: %s", device_name);
    }
    
    // Return to main UI
    oled_ui_set_state(UI_STATE_MAIN);
}

void app_main(void)
{
    ESP_LOGI(TAG, "LoRaCue starting - Enterprise presentation clicker");
    
    // Check wake cause
    esp_sleep_wakeup_cause_t wake_cause = esp_sleep_get_wakeup_cause();
    if (wake_cause != ESP_SLEEP_WAKEUP_UNDEFINED) {
        ESP_LOGI(TAG, "Wake from sleep, cause: %d", wake_cause);
    }
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize power management first
    ESP_LOGI(TAG, "Initializing power management...");
    power_config_t power_config = {
#ifdef SIMULATOR_BUILD
        .light_sleep_timeout_ms = 0,         // Disabled in simulator
        .deep_sleep_timeout_ms = 0,          // Disabled in simulator  
        .enable_auto_light_sleep = false,    // No sleep in simulator
        .enable_auto_deep_sleep = false,     // No sleep in simulator
#else
        .light_sleep_timeout_ms = 0,         // TODO: Re-enable after implementing I2C restoration
        .deep_sleep_timeout_ms = 300000,    // 5 minutes
        .enable_auto_light_sleep = false,   // FIXME: Disabled due to I2C corruption after wake
        .enable_auto_deep_sleep = true,
#endif
        .cpu_freq_mhz = 80,                 // 80MHz for power efficiency
    };
    ret = power_mgmt_init(&power_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Power management initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize BSP
    ESP_LOGI(TAG, "Initializing Board Support Package...");
    ret = bsp_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BSP initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize LED manager and turn on status LED
    ESP_LOGI(TAG, "Initializing LED manager...");
    ret = led_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED manager initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    led_manager_solid(true); // Turn on LED during startup
    
    // Initialize web configuration
    ESP_LOGI(TAG, "Initializing web configuration system...");
    ret = web_config_init(NULL); // Use default settings
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Web config initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize USB pairing
    ESP_LOGI(TAG, "Initializing USB pairing system...");
    ret = usb_pairing_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "USB pairing initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize OLED UI
    ESP_LOGI(TAG, "Initializing OLED UI...");
    ret = oled_ui_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OLED UI initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Show boot logo
    oled_ui_show_boot_logo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Initialize button manager
    ESP_LOGI(TAG, "Initializing button manager...");
    ret = button_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Button manager initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Start button manager task
    ret = button_manager_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start button manager: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize device registry
    ESP_LOGI(TAG, "Initializing device registry...");
    ret = device_registry_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Device registry initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize USB HID
    ESP_LOGI(TAG, "Initializing USB HID interface...");
    ret = usb_hid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "USB HID initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize LoRa driver
    ESP_LOGI(TAG, "Initializing LoRa driver...");
    ret = lora_driver_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LoRa driver initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize LoRa protocol
    uint16_t device_id = 0xAC00 | (esp_random() & 0xFF); // PC device ID (AC = LoRaCue)
    ESP_LOGI(TAG, "Initializing LoRa protocol with PC device ID: 0x%04X", device_id);
    ret = lora_protocol_init(device_id, demo_aes_key_1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LoRa protocol initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Validate hardware
    ESP_LOGI(TAG, "Running hardware validation...");
    ret = bsp_validate_hardware();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Hardware validation failed");
    }
    
    // Transition to main UI state
    oled_ui_set_state(UI_STATE_MAIN);
    
    // Start LED fading after initialization complete
    ESP_LOGI(TAG, "Starting LED fade pattern");
    led_manager_fade(3000); // 3 second fade cycle
    
    // Set to receive mode initially
    lora_set_receive_mode();
    
    // Main status update loop
    device_status_t status = {
        .battery_voltage = 3.7f,
        .signal_strength = 85,
        .usb_connected = usb_hid_is_connected(),
        .is_charging = false,
        .paired_count = device_registry_get_count()
    };
    strcpy(status.device_name, "LoRaCue-STAGE");
    
    uint32_t stats_counter = 0;
    
    while (1) {
        // Process USB pairing
        usb_pairing_process();
        
        // Update device status
        status.battery_voltage = bsp_read_battery();
        status.usb_connected = usb_hid_is_connected() || usb_pairing_is_connected();
        status.paired_count = device_registry_get_count();
        
        // Update UI with current status
        oled_ui_update_status(&status);
        
        // Log system statistics every 30 seconds
        if (++stats_counter >= 300) { // 300 * 100ms = 30s
            power_stats_t power_stats;
            if (power_mgmt_get_stats(&power_stats) == ESP_OK) {
                ESP_LOGI(TAG, "⚡ Power Stats - Active: %dms, Light Sleep: %dms, Deep Sleep: %dms",
                         power_stats.active_time_ms, power_stats.light_sleep_time_ms, 
                         power_stats.deep_sleep_time_ms);
                ESP_LOGI(TAG, "🔋 Estimated battery life: %.1f hours", 
                         power_stats.estimated_battery_hours);
            }
            
            ESP_LOGI(TAG, "🔗 Pairing Status: %d paired devices, State: %d", 
                     status.paired_count, usb_pairing_get_state());
            
            ESP_LOGI(TAG, "🌐 Web Config: State=%d, Clients=%d", 
                     web_config_get_state(), web_config_get_client_count());
            
            stats_counter = 0;
        }
        
        // Check for incoming LoRa packets (non-blocking)
        lora_packet_data_t received_packet;
        ret = lora_protocol_receive_packet(&received_packet, 10); // 10ms timeout
        
        if (ret == ESP_OK) {
            // Get device info for logging
            paired_device_t sender_device;
            if (device_registry_get(received_packet.device_id, &sender_device) == ESP_OK) {
                ESP_LOGI(TAG, "🔓 Command from %s (0x%04X): cmd=0x%02X", 
                         sender_device.device_name, received_packet.device_id, 
                         received_packet.command);
            }
            
            // Execute received command via USB HID
            handle_lora_command(received_packet.command, received_packet.device_id);
            
            // Send ACK
            lora_protocol_send_ack(received_packet.device_id, received_packet.sequence_num);
            
            lora_set_receive_mode();
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGD(TAG, "Packet from unpaired device ignored");
        } else if (ret != ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "Receive error: %s", esp_err_to_name(ret));
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz update rate
    }
}
