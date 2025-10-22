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
#include "oled_ui.h"
#include "ota_engine.h"
#include "power_mgmt.h"
#include "power_mgmt_config.h"
#include "uart_commands.h"
#include "usb_hid.h"
#include "version.h"
#include <stdio.h>
#include <string.h>

static const char *TAG            = "LORACUE_MAIN";
device_mode_t current_device_mode = DEVICE_MODE_PRESENTER;
static EventGroupHandle_t system_events;
static const esp_partition_t *running_partition = NULL;
static TimerHandle_t ota_validation_timer       = NULL;

// External UI functions
extern void oled_ui_enable_background_tasks(bool enable);

// Active presenter tracking (PC mode)
typedef struct {
    uint16_t device_id;
    int16_t last_rssi;
    uint32_t last_seen_ms;
    uint32_t command_count;
} active_presenter_t;

#define MAX_ACTIVE_PRESENTERS 4
#define MAX_COMMAND_HISTORY 4
static active_presenter_t active_presenters[MAX_ACTIVE_PRESENTERS]  = {0};
static command_history_entry_t command_history[MAX_COMMAND_HISTORY] = {0};
static uint8_t command_history_count                                = 0;
static uint32_t total_commands_received                             = 0;

// Global status for PC mode screen access
oled_status_t g_oled_status = {0};

static void add_command_to_history(uint16_t device_id, const char *cmd_name, uint8_t keycode, uint8_t modifiers)
{
    // Shift history down
    for (int i = MAX_COMMAND_HISTORY - 1; i > 0; i--) {
        command_history[i] = command_history[i - 1];
    }

    // Add new entry at top
    command_history[0].timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    command_history[0].device_id    = device_id;
    command_history[0].keycode      = keycode;
    command_history[0].modifiers    = modifiers;

    // Get device name from registry
    paired_device_t device;
    if (device_registry_get(device_id, &device) == ESP_OK) {
        strncpy(command_history[0].device_name, device.device_name, sizeof(command_history[0].device_name) - 1);
    } else {
        snprintf(command_history[0].device_name, sizeof(command_history[0].device_name), "LC-%04X", device_id);
    }

    strncpy(command_history[0].command, cmd_name, sizeof(command_history[0].command) - 1);

    if (command_history_count < MAX_COMMAND_HISTORY) {
        command_history_count++;
    }
}

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

static EventGroupHandle_t system_events;

static void update_active_presenter(uint16_t device_id, int16_t rssi)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    int slot     = -1;

    // Find existing or empty slot
    for (int i = 0; i < MAX_ACTIVE_PRESENTERS; i++) {
        if (active_presenters[i].device_id == device_id) {
            slot = i;
            break;
        }
        if (slot == -1 && active_presenters[i].device_id == 0) {
            slot = i;
        }
    }

    // Expire old entries (>30s)
    for (int i = 0; i < MAX_ACTIVE_PRESENTERS; i++) {
        if (active_presenters[i].device_id != 0 && (now - active_presenters[i].last_seen_ms) > 30000) {
            ESP_LOGI(TAG, "Presenter 0x%04X expired", active_presenters[i].device_id);
            memset(&active_presenters[i], 0, sizeof(active_presenter_t));
        }
    }

    if (slot != -1) {
        active_presenters[slot].device_id    = device_id;
        active_presenters[slot].last_rssi    = rssi;
        active_presenters[slot].last_seen_ms = now;
        active_presenters[slot].command_count++;
    }
}

static void battery_monitor_task(void *pvParameters)
{
    oled_status_t *status = (oled_status_t *)pvParameters;
    uint8_t prev_battery  = 0;

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
    bool prev_usb         = false;

    while (1) {
        bool current_usb = usb_hid_is_connected();

        if (current_usb != prev_usb) {
            status->usb_connected = current_usb;
            xEventGroupSetBits(system_events, (1 << 1));

            if (current_device_mode == DEVICE_MODE_PC && !current_usb) {
                ESP_LOGW(TAG, "PC mode: USB disconnected - cannot send HID events");
                oled_ui_show_message("PC Mode", "Connect USB Cable", 3000);
            }

            prev_usb = current_usb;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void pc_mode_update_task(void *pvParameters)
{
    oled_status_t *status = (oled_status_t *)pvParameters;

    while (1) {
        // Only update in PC mode (check global mode instead of NVS)
        if (current_device_mode == DEVICE_MODE_PC && oled_ui_get_screen() == OLED_SCREEN_PC_MODE) {
            // Update command history timestamps
            status->command_history_count = command_history_count;
            for (int i = 0; i < command_history_count && i < 4; i++) {
                memcpy(&status->command_history[i], &command_history[i], sizeof(command_history_entry_t));
            }
            xEventGroupSetBits(system_events, (1 << 4));
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Rate limiter for PC mode
typedef struct {
    uint32_t last_command_ms;
    uint32_t command_count_1s;
} rate_limiter_t;

static rate_limiter_t rate_limiter = {0};

static bool rate_limiter_check(void)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (now - rate_limiter.last_command_ms > 1000) {
        rate_limiter.command_count_1s = 0;
    }

    if (rate_limiter.command_count_1s >= 10) {
        return false;
    }

    rate_limiter.last_command_ms = now;
    rate_limiter.command_count_1s++;
    return true;
}

// Application layer: LoRa command to USB HID mapping
static void lora_rx_handler(uint16_t device_id, uint16_t sequence_num, lora_command_t command, const uint8_t *payload,
                            uint8_t payload_length, int16_t rssi, void *user_ctx)
{
    // In presenter mode, only accept ACKs
    if (current_device_mode == DEVICE_MODE_PRESENTER) {
        if (command == CMD_ACK) {
            ESP_LOGI(TAG, "Presenter mode: ACK received from 0x%04X (seq=%u)", device_id, sequence_num);
            // ACK handling is done in lora_protocol layer
            return;
        }
        ESP_LOGD(TAG, "Presenter mode: ignoring non-ACK command");
        return;
    }

    ESP_LOGI(TAG, "PC mode RX: device=0x%04X, seq=%u, cmd=0x%02X, rssi=%d dBm", device_id, sequence_num, command,
             rssi);

    // Device pairing validation
    if (!device_registry_is_paired(device_id)) {
        ESP_LOGW(TAG, "Ignoring command from unpaired device 0x%04X", device_id);
        return;
    }

    // Sequence tracking is now handled in lora_protocol_receive_packet()

    // Track active presenter
    update_active_presenter(device_id, rssi);
    total_commands_received++;

    if (!rate_limiter_check()) {
        ESP_LOGW(TAG, "Rate limit exceeded (>10 cmd/s)");
        return;
    }

    usb_hid_keycode_t keycode = 0;
    uint8_t modifiers         = 0;
    uint8_t slot_id           = 0;
    const char *cmd_name      = "UNKNOWN";

    // Parse HID report from payload
    if (command == CMD_HID_REPORT && payload_length >= sizeof(lora_payload_t)) {
        const lora_payload_t *pkt = (const lora_payload_t *)payload;
        slot_id                   = LORA_SLOT(pkt->version_slot);
        uint8_t hid_type          = LORA_HID_TYPE(pkt->type_flags);

        if (hid_type == HID_TYPE_KEYBOARD) {
            keycode   = pkt->hid_report.keyboard.keycode[0];
            modifiers = pkt->hid_report.keyboard.modifiers;

            // Map keycode to command name for logging
            switch (keycode) {
                case HID_KEY_PAGE_DOWN:
                    cmd_name = "NEXT";
                    break;
                case HID_KEY_PAGE_UP:
                    cmd_name = "PREV";
                    break;
                case HID_KEY_B:
                    cmd_name = "BLACK";
                    break;
                case HID_KEY_F5:
                    cmd_name = "START";
                    break;
                default:
                    cmd_name = "KEY";
                    break;
            }
        }
    } else {
        ESP_LOGW(TAG, "Invalid command or payload: cmd=0x%02X len=%d", command, payload_length);
        return;
    }

    if (keycode == 0) {
        ESP_LOGW(TAG, "No valid keycode extracted");
        return;
    }

    // Forward to USB HID if connected
    if (usb_hid_is_connected()) {
        ESP_LOGI(TAG, "PC mode: forwarding to USB slot %d", slot_id);
        usb_hid_send_key(keycode);
    } else {
        ESP_LOGW(TAG, "USB not connected, skipping HID forwarding (ACK sent by protocol layer)");
    }

    // Note: ACK is automatically sent by lora_protocol layer for successfully decrypted packets

    // Add to command history
    add_command_to_history(device_id, cmd_name, keycode, modifiers);
    
    // Trigger instant screen update in PC mode
    extern void ui_pc_history_notify_update(void);
    ui_pc_history_notify_update();

    // Update status for PC mode screen
    oled_status_t *status = (oled_status_t *)user_ctx;
    status->lora_signal   = rssi;
    strncpy(status->last_command, cmd_name, sizeof(status->last_command) - 1);

    // Update command history in status
    status->command_history_count = command_history_count;
    for (int i = 0; i < command_history_count && i < 4; i++) {
        memcpy(&status->command_history[i], &command_history[i], sizeof(command_history_entry_t));
    }

    // Update active presenters list
    status->active_presenter_count = 0;
    for (int i = 0; i < MAX_ACTIVE_PRESENTERS; i++) {
        if (active_presenters[i].device_id != 0) {
            status->active_presenters[status->active_presenter_count].device_id = active_presenters[i].device_id;
            status->active_presenters[status->active_presenter_count].rssi      = active_presenters[i].last_rssi;
            status->active_presenters[status->active_presenter_count].command_count =
                active_presenters[i].command_count;
            status->active_presenter_count++;
        }
    }

    // Immediate redraw for PC mode screen
    if (oled_ui_get_screen() == OLED_SCREEN_PC_MODE) {
        extern void pc_mode_screen_draw(const oled_status_t *);
        pc_mode_screen_draw(status);
    }

    xEventGroupSetBits(system_events, (1 << 3));
}

static void lora_state_handler(lora_connection_state_t state, void *user_ctx)
{
    oled_status_t *status  = (oled_status_t *)user_ctx;
    status->lora_connected = (state != LORA_CONNECTION_LOST);
    status->lora_signal    = (state == LORA_CONNECTION_EXCELLENT) ? 100
                             : (state == LORA_CONNECTION_GOOD)    ? 75
                             : (state == LORA_CONNECTION_WEAK)    ? 50
                                                                  : 25;
    xEventGroupSetBits(system_events, (1 << 2));
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
#ifdef SIMULATOR_BUILD
        .light_sleep_timeout_ms  = 0,
        .deep_sleep_timeout_ms   = 0,
        .enable_auto_light_sleep = false,
        .enable_auto_deep_sleep  = false,
#else
        .light_sleep_timeout_ms  = pwr_cfg.light_sleep_timeout_ms,
        .deep_sleep_timeout_ms   = pwr_cfg.deep_sleep_timeout_ms,
        .enable_auto_light_sleep = pwr_cfg.light_sleep_enabled,
        .enable_auto_deep_sleep  = pwr_cfg.deep_sleep_enabled,
#endif
        .cpu_freq_mhz = 80,
    };
    ESP_LOGI(TAG, "Power config: light_sleep=%s, deep_sleep=%s",
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
    general_config_get(&config);
    extern u8g2_t u8g2;
    u8g2_SetContrast(&u8g2, config.display_brightness);
    ESP_LOGI(TAG, "OLED brightness set to %d", config.display_brightness);

    // Show boot logo
    oled_ui_set_screen(OLED_SCREEN_BOOT);

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
    general_config_get(&config);
    current_device_mode = config.device_mode;

    // Generate device ID from MAC address (static identity)
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    uint16_t device_id = (mac[4] << 8) | mac[5]; // Use last 2 bytes of MAC

    ESP_LOGI(TAG, "Device mode: %s, Static ID: 0x%04X", device_mode_to_string(config.device_mode), device_id);

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

    // Create event group for system events
    system_events = xEventGroupCreate();
    if (!system_events) {
        ESP_LOGE(TAG, "Failed to create system event group");
        return;
    }

    // Initialize status with device name BEFORE showing screen
    g_oled_status.battery_level  = 85;
    g_oled_status.lora_connected = false;
    g_oled_status.lora_signal    = 0;
    g_oled_status.usb_connected  = false;
    g_oled_status.device_id      = 0x1234;

    // Load device name from config
    general_config_t dev_config;
    general_config_get(&dev_config);
    strncpy(g_oled_status.device_name, dev_config.device_name, sizeof(g_oled_status.device_name) - 1);
    strcpy(g_oled_status.last_command, "");

    // Transition to main UI state (adapts to mode automatically)
    oled_ui_set_screen(OLED_SCREEN_MAIN);

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
    lora_protocol_register_rx_callback(lora_rx_handler, &g_oled_status);
    lora_protocol_register_state_callback(lora_state_handler, &g_oled_status);
    // Button manager handles events directly via ui_screen_controller

    // Create event group for system events
    system_events = xEventGroupCreate();

    // Start periodic tasks
    xTaskCreate(battery_monitor_task, "battery_monitor", 4096, &g_oled_status, 5, NULL);
    xTaskCreate(usb_monitor_task, "usb_monitor", 2048, &g_oled_status, 5, NULL);
    xTaskCreate(pc_mode_update_task, "pc_mode_update", 4096, &g_oled_status, 5, NULL);

    // Main task now just handles events
    ESP_LOGI(TAG, "Main loop starting - should have low CPU usage when idle");

    while (1) {
        // Reset watchdog
        esp_task_wdt_reset();

        // Wait for any system event
        EventBits_t events = xEventGroupWaitBits(system_events,
                                                 0xFF,                  // Wait for any event
                                                 pdTRUE,                // Clear on exit
                                                 pdFALSE,               // Wait for any bit
                                                 pdMS_TO_TICKS(10000)); // 10s timeout

        if (events & (1 << 0)) { // Battery changed
            oled_ui_update_status(&g_oled_status);
        }

        if (events & (1 << 1)) { // USB status changed
            oled_ui_update_status(&g_oled_status);
        }

        if (events & (1 << 2)) { // LoRa state changed
            oled_ui_update_status(&g_oled_status);
        }

        if (events & (1 << 3)) { // Command received
            oled_ui_update_status(&g_oled_status);
        }

        if (events & (1 << 4)) { // PC mode periodic update
            // Just redraw PC mode screen, don't update full status
            if (oled_ui_get_screen() == OLED_SCREEN_PC_MODE) {
                extern void pc_mode_screen_draw(const oled_status_t *);
                pc_mode_screen_draw(&g_oled_status);
            }
        }

        if (events & (1 << 5)) { // Device mode changed
            ESP_LOGI(TAG, "Device mode changed to: %s", device_mode_to_string(current_device_mode));

            // Save to NVS
            general_config_t cfg;
            general_config_get(&cfg);
            cfg.device_mode = current_device_mode;
            general_config_set(&cfg);

            // Force screen redraw with new mode
            oled_ui_update_status(&g_oled_status);
        }

        // Check if device mode changed and persist to NVS (fallback for missed events)
        general_config_get(&config);
        if (config.device_mode != current_device_mode) {
            ESP_LOGI(TAG, "Device mode changed to: %s (fallback)", device_mode_to_string(current_device_mode));

            // Save to NVS
            config.device_mode = current_device_mode;
            general_config_set(&config);
        }
    }
}
