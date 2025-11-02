/**
 * @file main.c
 * @brief LoRaCue main application
 *
 * CONTEXT: LoRaCue enterprise presentation clicker
 * HARDWARE: Heltec LoRa V3 (ESP32-S3 + SX1262 + SH1106)
 * PURPOSE: Main application entry point and system initialization
 */

#include "bluetooth_config.h"
#include "bsp.h"
#include "button_manager.h"
#include "common_types.h"
#include "device_registry.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_random.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "general_config.h"
#include "led_manager.h"
#include "lora_driver.h"
#include "lora_protocol.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ui_interface.h"
#include "system_events.h"
#include "ota_engine.h"
#include "pc_mode_manager.h"
#include "power_mgmt.h"
#include "power_mgmt_config.h"
#include "presenter_mode_manager.h"
#include "uart_commands.h"
#include "usb_cdc.h"
#include "usb_console.h"
#include "usb_hid.h"
#include "version.h"
#include <stdio.h>
#include <string.h>

static const char *TAG            = "LORACUE_MAIN";
static const esp_partition_t *running_partition = NULL;
static TimerHandle_t ota_validation_timer       = NULL;

#define OTA_BOOT_COUNTER_KEY "ota_boot_cnt"
#define OTA_ROLLBACK_LOG_KEY "ota_rollback"
#define MAX_BOOT_ATTEMPTS 3

static uint32_t ota_get_boot_counter(void)
{
    nvs_handle_t nvs;
    uint32_t counter = 0;
    if (nvs_open("storage", NVS_READONLY, &nvs) == ESP_OK) {
        nvs_get_u32(nvs, OTA_BOOT_COUNTER_KEY, &counter);
        nvs_close(nvs);
    }
    return counter;
}

static void ota_increment_boot_counter(void)
{
    nvs_handle_t nvs;
    if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
        uint32_t counter = ota_get_boot_counter() + 1;
        nvs_set_u32(nvs, OTA_BOOT_COUNTER_KEY, counter);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGW(TAG, "Boot attempt %lu/%d", counter, MAX_BOOT_ATTEMPTS);
    }
}

static void ota_reset_boot_counter(void)
{
    nvs_handle_t nvs;
    if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_key(nvs, OTA_BOOT_COUNTER_KEY);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}

static void ota_log_rollback(const char *reason)
{
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

static void ota_validation_timer_cb(TimerHandle_t timer)
{
    ESP_LOGI(TAG, "60s health check passed - marking firmware valid");
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running_partition, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            esp_ota_mark_app_valid_cancel_rollback();
            ota_reset_boot_counter();
        }
    }
}

static void battery_monitor_task(void *pvParameters)
{
    uint8_t prev_battery = 0;

    while (1) {
        uint8_t current_battery = (uint8_t)(bsp_read_battery() * 100 / 4.2f);

        if (current_battery != prev_battery) {
            system_events_post_battery(current_battery, false);  // TODO: detect charging
            prev_battery = current_battery;
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
    }
}

static void usb_monitor_task(void *pvParameters)
{
    bool prev_usb = false;

    while (1) {
        bool current_usb = usb_hid_is_connected();

        if (current_usb != prev_usb) {
            system_events_post_usb(current_usb);
            prev_usb = current_usb;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Application layer: LoRa command to USB HID mapping
static void lora_rx_handler(uint16_t device_id, uint16_t sequence_num, lora_command_t command, const uint8_t *payload,
                            uint8_t payload_length, int16_t rssi, void *user_ctx)
{
    general_config_t config;
    general_config_get(&config);

    // In presenter mode, only accept ACKs
    if (config.device_mode == DEVICE_MODE_PRESENTER) {
        if (command == CMD_ACK) {
            ESP_LOGI(TAG, "Presenter mode: ACK received from 0x%04X (seq=%u)", device_id, sequence_num);
            // ACK handling is done in lora_protocol layer
            return;
        }
        ESP_LOGD(TAG, "Presenter mode: ignoring non-ACK command");
        return;
    }

    // PC mode - delegate to pc_mode_manager
    pc_mode_manager_process_command(device_id, sequence_num, command, payload, payload_length, rssi);
}

static void lora_state_handler(lora_connection_state_t state, const void *user_ctx)
{
    bool connected = (state != LORA_CONNECTION_LOST);
    int8_t signal = (state == LORA_CONNECTION_EXCELLENT) ? 100
                   : (state == LORA_CONNECTION_GOOD)    ? 75
                   : (state == LORA_CONNECTION_WEAK)    ? 50
                                                        : 25;
    system_events_post_lora_state(connected, signal);
}

void app_main(void)
{
    ESP_LOGI(TAG, "LoRaCue starting - Enterprise presentation clicker");
    ESP_LOGI(TAG, "Version: %s", LORACUE_VERSION_FULL);
    ESP_LOGI(TAG, "Build: %s (%s)", LORACUE_BUILD_COMMIT_SHORT, LORACUE_BUILD_BRANCH);
    ESP_LOGI(TAG, "Date: %s", LORACUE_BUILD_DATE);

    // Reduce u8g2 HAL logging noise
    esp_log_level_set("u8g2_hal", ESP_LOG_WARN);

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

    // OTA boot diagnostics
    running_partition = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running from partition: %s (0x%lx, %lu bytes)", running_partition->label, running_partition->address,
             running_partition->size);

    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
    if (boot_partition) {
        ESP_LOGI(TAG, "Boot partition: %s (0x%lx)", boot_partition->label, boot_partition->address);
        if (boot_partition != running_partition) {
            ESP_LOGW(TAG, "WARNING: Boot partition != running partition (rollback occurred?)");
        }
    }

    esp_ota_img_states_t ota_state;
    uint32_t boot_counter = ota_get_boot_counter();

    if (esp_ota_get_state_partition(running_partition, &ota_state) == ESP_OK) {
        const char *state_str = (ota_state == ESP_OTA_IMG_NEW)              ? "NEW"
                                : (ota_state == ESP_OTA_IMG_PENDING_VERIFY) ? "PENDING_VERIFY"
                                : (ota_state == ESP_OTA_IMG_VALID)          ? "VALID"
                                : (ota_state == ESP_OTA_IMG_INVALID)        ? "INVALID"
                                : (ota_state == ESP_OTA_IMG_ABORTED)        ? "ABORTED"
                                                                            : "UNKNOWN";
        ESP_LOGI(TAG, "OTA state: %s, boot counter: %lu", state_str, boot_counter);

        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGW(TAG, "New firmware pending validation (will auto-validate in 60s)");
            ota_increment_boot_counter();

            if (boot_counter >= MAX_BOOT_ATTEMPTS) {
                ESP_LOGE(TAG, "Max boot attempts reached (%lu), forcing rollback NOW", boot_counter);
                ota_log_rollback("max_boot_attempts");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    } else {
        ESP_LOGW(TAG, "Could not read OTA state for running partition");
    }

    // Initialize OTA engine
    ESP_LOGI(TAG, "Initializing OTA engine...");
    ret = ota_engine_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA engine initialization failed: %s", esp_err_to_name(ret));
    }

    // Reconfigure watchdog (90s timeout for init + 60s validation)
    esp_task_wdt_config_t wdt_config = {.timeout_ms = 90000, .idle_core_mask = 0, .trigger_panic = true};
    ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&wdt_config));
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    // Initialize device configuration
    ESP_LOGI(TAG, "Initializing device configuration system...");
    ret = general_config_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Device config initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Get device config for power management settings
    general_config_t config;
    general_config_get(&config);

    // Initialize power management with settings from NVS
    ESP_LOGI(TAG, "Initializing power management...");
    power_mgmt_config_t pwr_cfg;
    power_mgmt_config_get(&pwr_cfg);

    power_config_t power_config = {
        .display_sleep_timeout_ms  = pwr_cfg.display_sleep_timeout_ms,
        .light_sleep_timeout_ms    = pwr_cfg.light_sleep_timeout_ms,
        .deep_sleep_timeout_ms     = pwr_cfg.deep_sleep_timeout_ms,
        .enable_auto_display_sleep = pwr_cfg.display_sleep_enabled,
        .enable_auto_light_sleep   = pwr_cfg.light_sleep_enabled,
        .enable_auto_deep_sleep    = pwr_cfg.deep_sleep_enabled,
        .cpu_freq_mhz = 80,
    };

    ESP_LOGI(TAG, "Power config: display_sleep=%s, light_sleep=%s, deep_sleep=%s",
             power_config.enable_auto_display_sleep ? "enabled" : "disabled",
             power_config.enable_auto_light_sleep ? "enabled" : "disabled",
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

    // Detect board type early
    const char *board_id = bsp_get_board_id();
    ESP_LOGI(TAG, "Board: %s", board_id);

    // Initialize LED manager and turn on status LED
    ESP_LOGI(TAG, "Initializing LED manager...");
    ret = led_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED manager initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    led_manager_solid(true); // Turn on LED during startup

    // Initialize system events
    ESP_LOGI(TAG, "Initializing system events...");
    ret = system_events_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "System events initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize UI (self-contained, creates own task)
    ESP_LOGI(TAG, "Initializing UI...");
    ret = ui_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UI initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize button manager
    ESP_LOGI(TAG, "Initializing button manager...");
    ret = button_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Button manager initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize PC mode manager
    ESP_LOGI(TAG, "Initializing PC mode manager...");
    ret = pc_mode_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PC mode manager initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize presenter mode manager
    ESP_LOGI(TAG, "Initializing presenter mode manager...");
    ret = presenter_mode_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Presenter mode manager initialization failed: %s", esp_err_to_name(ret));
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
    general_config_get(&config);

    // Get device ID from MAC address (static identity)
    uint16_t device_id = general_config_get_device_id();

    ESP_LOGI(TAG, "Device mode: %s, Static ID: 0x%04X", device_mode_to_string(config.device_mode), device_id);

    // Notify UI of initial device mode
    system_events_post_mode_changed(config.device_mode);

    // Get AES key from LoRa config
    lora_config_t lora_cfg;
    lora_get_config(&lora_cfg);

    ret = lora_protocol_init(device_id, lora_cfg.aes_key);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LoRa protocol initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize LoRa communication
    ESP_LOGI(TAG, "Initializing LoRa communication...");
    // lora_protocol_init is called by lora_driver_init
    // No separate init needed here

    // Initialize USB HID
    ESP_LOGI(TAG, "Initializing USB HID interface...");
    ret = usb_hid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "USB HID initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize USB CDC command interface
    ESP_LOGI(TAG, "Initializing USB CDC command interface...");
    ret = usb_cdc_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "USB CDC initialization failed: %s", esp_err_to_name(ret));
        return;
    }

#if 0  // USB CDC is only for command parser, not console
    // Wait for USB enumeration before redirecting console
    ESP_LOGI(TAG, "Waiting for USB enumeration...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Redirect console to USB CDC (same port as commands)
    usb_console_init();
    ESP_LOGI(TAG, "Console redirected to USB CDC");
#endif

    // Initialize Bluetooth configuration
    ESP_LOGI(TAG, "Initializing Bluetooth configuration...");
    ret = bluetooth_config_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Bluetooth initialization failed: %s (continuing without BLE)", esp_err_to_name(ret));
        // Non-fatal - continue without Bluetooth
    }

#ifdef CONFIG_UART_COMMANDS_ENABLED
    // Initialize UART command interface
    ESP_LOGI(TAG, "Initializing UART command interface...");
    ret = uart_commands_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "UART commands initialization failed: %s", esp_err_to_name(ret));
    } else {
        ret = uart_commands_start();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "UART commands start failed: %s", esp_err_to_name(ret));
        }
    }
#else
    ESP_LOGI(TAG, "UART commands disabled - UART0 used for debug logging");
#endif

    // Validate hardware
    ESP_LOGI(TAG, "Running hardware validation...");
    ret = bsp_validate_hardware();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Hardware validation failed");
    }

    // Start LED fading after initialization complete
    ESP_LOGI(TAG, "Starting LED fade pattern");
    led_manager_fade(3000); // 3 second fade cycle

    // Set to receive mode initially
    lora_set_receive_mode();

    // Start LoRa communication task
    ESP_LOGI(TAG, "Starting LoRa communication...");
    ret = lora_protocol_start();
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
            ota_validation_timer =
                xTimerCreate("ota_valid", pdMS_TO_TICKS(60000), pdFALSE, NULL, ota_validation_timer_cb);
            if (ota_validation_timer) {
                xTimerStart(ota_validation_timer, 0);
            }
        }
    }

    // Register application callbacks
    lora_protocol_register_rx_callback(lora_rx_handler, NULL);
    lora_protocol_register_state_callback((lora_protocol_state_callback_t)lora_state_handler, NULL);

    // Start monitoring tasks
    xTaskCreate(battery_monitor_task, "battery_monitor", 4096, NULL, 5, NULL);
    xTaskCreate(usb_monitor_task, "usb_monitor", 2048, NULL, 5, NULL);

    // Main task now just handles events
    ESP_LOGI(TAG, "Main loop starting - watchdog keepalive only");

    while (1) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
