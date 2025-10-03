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
#include "esp_ota_ops.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "nvs.h"
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
#include "device_config.h"
#include "lora_comm.h"

static const char *TAG = "LORACUE_MAIN";
static device_mode_t current_device_mode = DEVICE_MODE_PRESENTER;
static EventGroupHandle_t system_events;
static const esp_partition_t *running_partition = NULL;
static TimerHandle_t ota_validation_timer = NULL;

// Demo AES-256 keys for different devices
static const uint8_t demo_aes_key_1[32] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
    0x60, 0x1e, 0xc3, 0x13, 0x77, 0x57, 0x89, 0xa5,
    0xb7, 0xa7, 0xf5, 0x04, 0xbb, 0xf3, 0xd2, 0x28
};

#define OTA_BOOT_COUNTER_KEY "ota_boot_cnt"
#define OTA_ROLLBACK_LOG_KEY "ota_rollback"
#define MAX_BOOT_ATTEMPTS 3

static uint32_t ota_get_boot_counter(void) {
    nvs_handle_t nvs;
    uint32_t counter = 0;
    if (nvs_open("storage", NVS_READONLY, &nvs) == ESP_OK) {
        nvs_get_u32(nvs, OTA_BOOT_COUNTER_KEY, &counter);
        nvs_close(nvs);
    }
    return counter;
}

static void ota_increment_boot_counter(void) {
    nvs_handle_t nvs;
    if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
        uint32_t counter = ota_get_boot_counter() + 1;
        nvs_set_u32(nvs, OTA_BOOT_COUNTER_KEY, counter);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGW(TAG, "Boot attempt %lu/%d", counter, MAX_BOOT_ATTEMPTS);
    }
}

static void ota_reset_boot_counter(void) {
    nvs_handle_t nvs;
    if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_key(nvs, OTA_BOOT_COUNTER_KEY);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}

static void ota_log_rollback(const char *reason) {
    nvs_handle_t nvs;
    if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
        char log[128];
        snprintf(log, sizeof(log), "%s|%s", running_partition->label, reason);
        nvs_set_str(nvs, OTA_ROLLBACK_LOG_KEY, log);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGE(TAG, "Rollback logged: %s", log);
    }
}

static void ota_validation_timer_cb(TimerHandle_t timer) {
    ESP_LOGI(TAG, "60s health check passed - marking firmware valid");
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running_partition, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            esp_ota_mark_app_valid_cancel_rollback();
            ota_reset_boot_counter();
        }
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
        bool current_usb = usb_hid_is_connected();
        
        if (current_usb != prev_usb) {
            status->usb_connected = current_usb;
            xEventGroupSetBits(system_events, (1 << 1)); // USB event
            prev_usb = current_usb;
        }
        
        vTaskDelay(pdMS_TO_TICKS(500)); // Check every 500ms
    }
}

// Application layer: LoRa command to USB HID mapping
static void lora_rx_handler(lora_command_t command, const uint8_t *payload, uint8_t payload_length, int16_t rssi, void *user_ctx)
{
    ESP_LOGI(TAG, "LoRa RX: cmd=0x%02X, rssi=%d dBm", command, rssi);
    
    usb_hid_keycode_t keycode;
    switch (command) {
        case CMD_NEXT_SLIDE:
            keycode = HID_KEY_PAGE_DOWN;
            break;
        case CMD_PREV_SLIDE:
            keycode = HID_KEY_PAGE_UP;
            break;
        case CMD_BLACK_SCREEN:
            keycode = HID_KEY_B;
            break;
        case CMD_START_PRESENTATION:
            keycode = HID_KEY_F5;
            break;
        default:
            ESP_LOGW(TAG, "Unknown command: 0x%02X", command);
            return;
    }
    
    usb_hid_send_key(keycode);
}

static void lora_state_handler(lora_connection_state_t state, void *user_ctx)
{
    oled_status_t *status = (oled_status_t *)user_ctx;
    status->lora_connected = (state != LORA_CONNECTION_LOST);
    status->lora_signal = (state == LORA_CONNECTION_EXCELLENT) ? 100 : 
                          (state == LORA_CONNECTION_GOOD) ? 75 :
                          (state == LORA_CONNECTION_WEAK) ? 50 : 25;
    xEventGroupSetBits(system_events, (1 << 2));
}

static void button_handler(button_event_type_t event, void *arg)
{
    lora_command_t cmd;
    switch (event) {
        case BUTTON_EVENT_PREV_SHORT:
            cmd = CMD_PREV_SLIDE;
            break;
        case BUTTON_EVENT_NEXT_SHORT:
            cmd = CMD_NEXT_SLIDE;
            break;
        case BUTTON_EVENT_PREV_LONG:
            cmd = CMD_BLACK_SCREEN;
            break;
        case BUTTON_EVENT_NEXT_LONG:
            cmd = CMD_START_PRESENTATION;
            break;
        default:
            return;
    }
    
    lora_comm_send_command_reliable(cmd, NULL, 0);
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
    
    // OTA rollback validation
    running_partition = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    
    // Check boot counter for repeated failures
    uint32_t boot_counter = ota_get_boot_counter();
    if (boot_counter >= MAX_BOOT_ATTEMPTS) {
        ESP_LOGE(TAG, "Max boot attempts reached (%lu), forcing rollback", boot_counter);
        ota_log_rollback("max_boot_attempts");
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
    
    if (esp_ota_get_state_partition(running_partition, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGW(TAG, "New firmware pending validation");
            ota_increment_boot_counter();
        }
    }
    ESP_LOGI(TAG, "Running partition: %s (offset 0x%lx)", running_partition->label, running_partition->address);
    
    // Reconfigure watchdog (90s timeout for init + 60s validation)
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 90000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&wdt_config));
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    
    // Initialize device configuration
    ESP_LOGI(TAG, "Initializing device configuration system...");
    ret = device_config_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Device config initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Get device config for power management settings
    device_config_t config;
    device_config_get(&config);
    
    // Initialize power management with settings from NVS
    ESP_LOGI(TAG, "Initializing power management...");
    power_config_t power_config = {
#ifdef SIMULATOR_BUILD
        .light_sleep_timeout_ms = 0,
        .deep_sleep_timeout_ms = 0,
        .enable_auto_light_sleep = false,
        .enable_auto_deep_sleep = false,
#else
        .light_sleep_timeout_ms = 0,         // TODO: Re-enable after I2C restoration
        .deep_sleep_timeout_ms = config.auto_sleep_enabled ? config.sleep_timeout_ms : 0,
        .enable_auto_light_sleep = false,   // FIXME: Disabled due to I2C corruption
        .enable_auto_deep_sleep = config.auto_sleep_enabled,
#endif
        .cpu_freq_mhz = 80,
    };
    ESP_LOGI(TAG, "Power config: deep_sleep=%lums, auto_sleep=%s", 
             power_config.deep_sleep_timeout_ms, 
             power_config.enable_auto_deep_sleep ? "enabled" : "disabled");
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
    
    // Initialize OLED UI
    ESP_LOGI(TAG, "Initializing OLED UI...");
    ret = oled_ui_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OLED UI initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Apply brightness from config (reuse config from power mgmt)
    device_config_get(&config);
    extern u8g2_t u8g2;
    u8g2_SetContrast(&u8g2, config.display_brightness);
    ESP_LOGI(TAG, "OLED brightness set to %d", config.display_brightness);
    
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
    
    // Initialize LoRa driver
    ESP_LOGI(TAG, "Initializing LoRa driver...");
    ret = lora_driver_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LoRa driver initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Get device mode from NVS (reuse config from brightness setting)
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
    
    // Initialize LoRa communication
    ESP_LOGI(TAG, "Initializing LoRa communication...");
    ret = lora_comm_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LoRa communication initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize USB HID
    ESP_LOGI(TAG, "Initializing USB HID interface...");
    ret = usb_hid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "USB HID initialization failed: %s", esp_err_to_name(ret));
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
    
    // Start LoRa communication task
    ESP_LOGI(TAG, "Starting LoRa communication...");
    ret = lora_comm_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start LoRa communication: %s", esp_err_to_name(ret));
        return;
    }
    
    // Reset watchdog after successful init
    esp_task_wdt_reset();
    
    // Start 60s validation timer for new firmware
    if (esp_ota_get_state_partition(running_partition, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Starting 60s health check for new firmware");
            ota_validation_timer = xTimerCreate("ota_valid", pdMS_TO_TICKS(60000), 
                                                pdFALSE, NULL, ota_validation_timer_cb);
            if (ota_validation_timer) {
                xTimerStart(ota_validation_timer, 0);
            }
        }
    }
    
    // Main status update loop
    oled_status_t status = {
        .battery_level = 85,
        .lora_connected = false,
        .lora_signal = 0,
        .usb_connected = false,
        .device_id = 0x1234
    };
    strcpy(status.device_name, "LoRaCue-STAGE");
    
    // Register application callbacks
    lora_comm_register_rx_callback(lora_rx_handler, &status);
    lora_comm_register_state_callback(lora_state_handler, &status);
    button_manager_register_callback(button_handler, NULL);
    
    // Create event group for system events
    system_events = xEventGroupCreate();
    
    // Start periodic tasks
    xTaskCreate(battery_monitor_task, "battery_monitor", 2048, &status, 5, NULL);
    xTaskCreate(usb_monitor_task, "usb_monitor", 2048, &status, 5, NULL);
    
    // Main task now just handles events
    ESP_LOGI(TAG, "Main loop starting - should have low CPU usage when idle");
    
    while (1) {
        // Reset watchdog
        esp_task_wdt_reset();
        
        // Wait for any system event
        EventBits_t events = xEventGroupWaitBits(system_events, 
                                                0xFF, // Wait for any event
                                                pdTRUE, // Clear on exit
                                                pdFALSE, // Wait for any bit
                                                pdMS_TO_TICKS(10000)); // 10s timeout
        
        if (events & (1 << 0)) { // Battery changed
            oled_ui_update_status(&status);
        }
        
        if (events & (1 << 1)) { // USB status changed
            oled_ui_update_status(&status);
        }
        
        if (events & (1 << 2)) { // LoRa state changed
            oled_ui_update_status(&status);
        }
        
        // Check for device mode changes when not in menu
        check_device_mode_change();
    }
}
