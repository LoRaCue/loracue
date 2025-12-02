#include "ui_compact.h"
#include "ble.h"
#include "bsp.h"
#include "esp_log.h"
#include "general_config.h"
#include "input_manager.h"
#include "lora_protocol.h"
#include "screens.h"
#include "system_events.h"
#include "tusb.h"
#include "ui_components.h"
#include "ui_lvgl.h"
#include "ui_navigator.h"

static const char *TAG = "ui_compact";

#define BATTERY_UPDATE_INTERVAL_MS 5000

static lv_obj_t *current_screen             = NULL;
static ui_screen_type_t current_screen_type = UI_SCREEN_BOOT;
static general_config_t cached_config;

void ui_compact_get_status(statusbar_data_t *status)
{
    general_config_get(&cached_config);

    // Get USB host connection status from TinyUSB
    status->usb_connected = tud_mounted();

    // Get Bluetooth status
    status->bluetooth_enabled   = ble_is_enabled();
    status->bluetooth_connected = ble_is_connected();

    // Get LoRa signal strength
    lora_connection_state_t conn_state = lora_protocol_get_connection_state();
    status->signal_strength            = lora_connection_to_signal_strength(conn_state);

    // Get battery level with caching (update every 5 seconds)
    static uint32_t last_battery_update = 0;
    static float cached_voltage         = 0.0f;

    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (now - last_battery_update > BATTERY_UPDATE_INTERVAL_MS || last_battery_update == 0) {
        cached_voltage      = bsp_read_battery();
        last_battery_update = now;
    }

    status->battery_level = bsp_battery_voltage_to_percentage(cached_voltage);

    status->device_name = cached_config.device_name;
}

static void input_event_handler(input_event_t event)
{
    ESP_LOGD(TAG, "Input event received: %d on screen %d", event, current_screen_type);

    ui_lvgl_lock();
    ui_navigator_handle_input_event(event);
    ui_lvgl_unlock();
}

// cppcheck-suppress constParameterCallback
static void mode_change_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const system_event_mode_t *evt = (const system_event_mode_t *)data;
    ESP_LOGI(TAG, "Mode changed to: %s", evt->mode == DEVICE_MODE_PRESENTER ? "PRESENTER" : "PC");

    // Reload main screen with new mode
    ui_compact_show_main_screen();
}

static void on_navigator_screen_change(ui_screen_type_t type)
{
    current_screen_type = type;
    current_screen      = NULL;
}

esp_err_t ui_compact_init(void)
{
    ESP_LOGI(TAG, "Initializing compact UI for small displays");

    // Initialize LVGL core
    esp_err_t ret = ui_lvgl_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL core");
        return ret;
    }

    // Initialize UI component styles
    ui_components_init();

    // Register for mode change events
    esp_event_loop_handle_t loop = system_events_get_loop();
    esp_event_handler_register_with(loop, SYSTEM_EVENTS, SYSTEM_EVENT_MODE_CHANGED, mode_change_event_handler, NULL);

    // Initialize Navigator
    ui_navigator_init();
    ui_navigator_set_screen_change_callback(on_navigator_screen_change);

    // Register Screens
    static const struct {
        ui_screen_type_t type;
        ui_screen_t *(*getter)(void);
    } screen_registry[] = {
        {UI_SCREEN_BOOT, screen_boot_get_interface},
        {UI_SCREEN_MAIN, screen_main_get_interface},
        {UI_SCREEN_PC_MODE, screen_pc_mode_get_interface},
        {UI_SCREEN_MENU, screen_menu_get_interface},
        {UI_SCREEN_PAIRING, screen_pairing_get_interface},
        {UI_SCREEN_DEVICE_REGISTRY, screen_device_registry_get_interface},
        {UI_SCREEN_CONFIG_MODE, screen_config_mode_get_interface},
        {UI_SCREEN_DEVICE_INFO, screen_device_info_get_interface},
        {UI_SCREEN_SYSTEM_INFO, screen_system_info_get_interface},
        {UI_SCREEN_BATTERY, screen_battery_status_get_interface},
        {UI_SCREEN_BLUETOOTH, screen_bluetooth_get_interface},
        {UI_SCREEN_CONTRAST, screen_contrast_get_interface},
        {UI_SCREEN_SLOT, screen_slot_get_interface},
        {UI_SCREEN_DEVICE_MODE, screen_device_mode_get_interface},
        {UI_SCREEN_FACTORY_RESET, screen_factory_reset_get_interface},
        {UI_SCREEN_LORA_FREQUENCY, screen_lora_frequency_get_interface},
        {UI_SCREEN_LORA_BW, screen_lora_bw_get_interface},
        {UI_SCREEN_LORA_CR, screen_lora_cr_get_interface},
        {UI_SCREEN_LORA_TXPOWER, screen_lora_txpower_get_interface},
        {UI_SCREEN_LORA_BAND, screen_lora_band_get_interface},
        {UI_SCREEN_LORA_PRESETS, screen_lora_presets_get_interface},
        {UI_SCREEN_LORA_SF, screen_lora_sf_get_interface},
        {UI_SCREEN_LORA_SUBMENU, screen_lora_submenu_get_interface},
    };

    for (size_t i = 0; i < sizeof(screen_registry) / sizeof(screen_registry[0]); i++) {
        ui_navigator_register_screen(screen_registry[i].type, screen_registry[i].getter());
    }

    ESP_LOGI(TAG, "Compact UI initialized");
    return ESP_OK;
}

esp_err_t ui_compact_show_boot_screen(void)
{
    ui_lvgl_lock();
    ui_navigator_switch_to(UI_SCREEN_BOOT);
    ui_lvgl_unlock();
    return ESP_OK;
}

esp_err_t ui_compact_show_main_screen(void)
{
    ui_lvgl_lock();

    // Check device mode and show appropriate screen
    general_config_t config;
    general_config_get(&config);

    if (config.device_mode == DEVICE_MODE_PC) {
        ui_navigator_switch_to(UI_SCREEN_PC_MODE);
    } else {
        ui_navigator_switch_to(UI_SCREEN_MAIN);
    }

    ui_lvgl_unlock();
    return ESP_OK;
}

esp_err_t ui_compact_register_button_callback(void)
{
    input_manager_register_callback(input_event_handler);
    ESP_LOGI(TAG, "Input callback registered");
    return ESP_OK;
}

// Stub for commands component
esp_err_t ui_data_provider_reload_config(void)
{
    ESP_LOGI(TAG, "Config reload requested (stub)");
    return ESP_OK;
}
