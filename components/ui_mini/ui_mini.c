#include "ui_mini.h"
#include "ui_lvgl.h"
#include "screens.h"
#include "widgets.h"
#include "esp_log.h"

static const char *TAG = "ui_mini";
static bool ui_initialized = false;

ui_state_t ui_state = {
    .battery_level = 100,
    .battery_charging = false,
    .usb_connected = false,
    .ble_enabled = false,
    .lora_rssi = 0,
    .current_mode = DEVICE_MODE_PRESENTER
};

esp_err_t ui_mini_init(void) {
    ESP_LOGI(TAG, "Initializing UI Mini (LVGL)");
    
    esp_err_t ret = ui_lvgl_init();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Create main screen with status bar
    lv_obj_t *screen = lv_obj_create(NULL);
    widget_statusbar_create(screen);
    screen_main_create(screen);
    lv_screen_load(screen);
    
    ui_initialized = true;
    ESP_LOGI(TAG, "UI Mini initialized");
    return ESP_OK;
}

esp_err_t ui_mini_set_screen(ui_mini_screen_t screen) {
    (void)screen;
    return ESP_OK;
}

ui_mini_screen_t ui_mini_get_screen(void) {
    return OLED_SCREEN_MAIN;
}

esp_err_t ui_mini_show_message(const char *title, const char *message, uint32_t timeout_ms) {
    (void)title; (void)message; (void)timeout_ms;
    return ESP_OK;
}

esp_err_t ui_mini_clear(void) {
    return ESP_OK;
}

u8g2_t *ui_mini_get_u8g2(void) {
    return NULL;
}

bool ui_mini_try_lock_draw(void) {
    return true;
}

void ui_mini_unlock_draw(void) {
}

esp_err_t ui_mini_display_off(void) {
    return ESP_OK;
}

esp_err_t ui_mini_display_on(void) {
    return ESP_OK;
}

esp_err_t ui_mini_show_ota_update(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    screen_ota_create(screen);
    lv_screen_load(screen);
    return ESP_OK;
}

esp_err_t ui_mini_update_ota_progress(uint8_t progress) {
    screen_ota_update_progress(progress);
    return ESP_OK;
}

// Stub functions for backward compatibility
void ui_data_provider_reload_config(void) {}
void ui_mini_enable_background_tasks(bool enable) { (void)enable; }
bool ui_mini_background_tasks_enabled(void) { return true; }
uint8_t ui_mini_get_ota_progress(void) { return 0; }

