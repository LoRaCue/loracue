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
#include "esp_mac.h"
#include "nvs_flash.h"
#include "version.h"
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
#include "device_config.h"

static const char *TAG = "LORACUE_MAIN";
static device_mode_t current_device_mode = DEVICE_MODE_PRESENTER;
static EventGroupHandle_t system_events;

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
    
    // Get device mode to determine behavior
    device_config_t config;
    device_config_get(&config);
    
    // Only PC mode handles commands
    if (config.device_mode != DEVICE_MODE_PC) {
        ESP_LOGD(TAG, "Ignoring command - device in PRESENTER mode");
        return ESP_OK;
    }
    
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

static EventGroupHandle_t system_events;

static void check_device_mode_change(void)
{
    device_config_t config;
    device_config_get(&config);
    
    if (config.device_mode != current_device_mode) {
        ESP_LOGI(TAG, "Device mode changed from %s to %s", 
                 device_mode_to_string(current_device_mode),
                 device_mode_to_string(config.device_mode));
        current_device_mode = config.device_mode;
    }
}

static void battery_monitor_task(void *pvParameters)
{
    oled_status_t *status = (oled_status_t *)pvParameters;
    uint8_t prev_battery = 0;
    
    while (1) {
        uint8_t current_battery = (uint8_t)(bsp_read_battery() * 100 / 4.2f);
        
        if (current_battery != prev_battery) {
            status->battery_level = current_battery;
            xEventGroupSetBits(system_events, (1 << 0)); // Battery event
            prev_battery = current_battery;
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
    }
}

static void usb_monitor_task(void *pvParameters)
{
    oled_status_t *status = (oled_status_t *)pvParameters;
    bool prev_usb = false;
    
    while (1) {
        bool current_usb = usb_hid_is_connected() || usb_pairing_is_connected();
        
        if (current_usb != prev_usb) {
            status->usb_connected = current_usb;
            xEventGroupSetBits(system_events, (1 << 1)); // USB event
            prev_usb = current_usb;
        }
        
        vTaskDelay(pdMS_TO_TICKS(500)); // Check every 500ms
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
    oled_ui_set_screen(OLED_SCREEN_MAIN);
}

void app_main(void)
{
    ESP_LOGI(TAG, "LoRaCue starting - Enterprise presentation clicker");
    ESP_LOGI(TAG, "Version: %s", LORACUE_VERSION_FULL);
    ESP_LOGI(TAG, "Build: %s (%s)", LORACUE_BUILD_COMMIT_SHORT, LORACUE_BUILD_BRANCH);
    ESP_LOGI(TAG, "Date: %s", LORACUE_BUILD_DATE);
    
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
    
    // Initialize device configuration
    ESP_LOGI(TAG, "Initializing device configuration system...");
    ret = device_config_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Device config initialization failed: %s", esp_err_to_name(ret));
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
    oled_ui_set_screen(OLED_SCREEN_BOOT);
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
    
    // Get device mode from NVS
    device_config_t config;
    device_config_get(&config);
    current_device_mode = config.device_mode;
    
    // Generate device ID from MAC address (static identity)
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    uint16_t device_id = (mac[4] << 8) | mac[5]; // Use last 2 bytes of MAC
    
    ESP_LOGI(TAG, "Device mode: %s, Static ID: 0x%04X", 
             device_mode_to_string(config.device_mode), device_id);
    
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
    
    // Create event group for system events
    system_events = xEventGroupCreate();
    if (!system_events) {
        ESP_LOGE(TAG, "Failed to create system event group");
        return;
    }
    
    // Transition to main UI state
    oled_ui_set_screen(OLED_SCREEN_MAIN);
    
    // Start LED fading after initialization complete
    ESP_LOGI(TAG, "Starting LED fade pattern");
    led_manager_fade(3000); // 3 second fade cycle
    
    // Set to receive mode initially
    lora_set_receive_mode();
    
    // Main status update loop
    oled_status_t status = {
        .battery_level = 85,
        .lora_connected = false,
        .lora_signal = 0,
        .usb_connected = false,
        .device_id = 0x1234
    };
    strcpy(status.device_name, "LoRaCue-STAGE");
    
    // Create event group for system events
    system_events = xEventGroupCreate();
    
    // Start periodic tasks
    xTaskCreate(battery_monitor_task, "battery_monitor", 2048, &status, 5, NULL);
    xTaskCreate(usb_monitor_task, "usb_monitor", 2048, &status, 5, NULL);
    
    // Main task now just handles events
    ESP_LOGI(TAG, "Main loop starting - should have low CPU usage when idle");
    
    while (1) {
        // Wait for any system event
        EventBits_t events = xEventGroupWaitBits(system_events, 
                                                0xFF, // Wait for any event
                                                pdTRUE, // Clear on exit
                                                pdFALSE, // Wait for any bit
                                                portMAX_DELAY);
        
        if (events & (1 << 0)) { // Battery changed
            oled_ui_update_status(&status);
        }
        
        if (events & (1 << 1)) { // USB status changed
            oled_ui_update_status(&status);
        }
        
        // Check for device mode changes when not in menu
        check_device_mode_change();
    }
}
