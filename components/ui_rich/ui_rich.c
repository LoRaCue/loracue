/**
 * @file ui_rich.c
 * @brief Rich UI implementation for LilyGO T5 e-paper
 */

#include "ui_rich.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "bootscreen.h"
#include <time.h>

static const char *TAG = "ui_rich";

static lv_obj_t *screen_boot = NULL;
static lv_obj_t *screen_launcher = NULL;
static lv_obj_t *status_bar = NULL;
static lv_obj_t *label_time = NULL;
static lv_obj_t *label_battery = NULL;
static lv_obj_t *icon_lora = NULL;
static lv_obj_t *icon_wifi = NULL;
static lv_obj_t *icon_charging = NULL;

static ui_rich_status_t current_status = {
    .battery_percent = 100,
    .charging = false,
    .lora_connected = false,
    .wifi_connected = false,
};

static void create_boot_screen(void)
{
    screen_boot = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_boot, lv_color_white(), 0);
    
    lv_obj_t *img = lv_img_create(screen_boot);
    lv_img_set_src(img, &bootscreen);
    lv_obj_center(img);
}

static void create_status_bar(lv_obj_t *parent)
{
    status_bar = lv_obj_create(parent);
    lv_obj_set_size(status_bar, LV_PCT(100), 40);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(status_bar, lv_color_white(), 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_pad_all(status_bar, 5, 0);
    
    // Time (left)
    label_time = lv_label_create(status_bar);
    lv_label_set_text(label_time, "12:00");
    lv_obj_align(label_time, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_20, 0);
    
    // Battery percentage (right)
    label_battery = lv_label_create(status_bar);
    lv_label_set_text(label_battery, "100%");
    lv_obj_align(label_battery, LV_ALIGN_RIGHT_MID, -5, 0);
    lv_obj_set_style_text_font(label_battery, &lv_font_montserrat_16, 0);
    
    // Charging icon (bolt)
    icon_charging = lv_label_create(status_bar);
    lv_label_set_text(icon_charging, LV_SYMBOL_CHARGE);
    lv_obj_align_to(icon_charging, label_battery, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_add_flag(icon_charging, LV_OBJ_FLAG_HIDDEN);
    
    // WiFi icon
    icon_wifi = lv_label_create(status_bar);
    lv_label_set_text(icon_wifi, LV_SYMBOL_WIFI);
    lv_obj_align_to(icon_wifi, icon_charging, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_add_flag(icon_wifi, LV_OBJ_FLAG_HIDDEN);
    
    // LoRa icon (using antenna symbol)
    icon_lora = lv_label_create(status_bar);
    lv_label_set_text(icon_lora, LV_SYMBOL_BLUETOOTH);
    lv_obj_align_to(icon_lora, icon_wifi, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_add_flag(icon_lora, LV_OBJ_FLAG_HIDDEN);
}

static void create_launcher_screen(void)
{
    screen_launcher = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_launcher, lv_color_white(), 0);
    
    create_status_bar(screen_launcher);
    
    // App grid container
    lv_obj_t *grid = lv_obj_create(screen_launcher);
    lv_obj_set_size(grid, LV_PCT(100), LV_PCT(100) - 40);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(grid, lv_color_white(), 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(grid, 20, 0);
    lv_obj_set_style_pad_gap(grid, 20, 0);
    
    // Create 12 app icons (3x4 grid)
    const char *app_names[] = {
        "Presenter", "PC Mode", "Settings", "LoRa",
        "WiFi", "Bluetooth", "Power", "Info",
        "Pairing", "Registry", "Brightness", "Reset"
    };
    
    for (int i = 0; i < 12; i++) {
        lv_obj_t *app = lv_obj_create(grid);
        lv_obj_set_size(app, 150, 180);
        lv_obj_set_style_bg_color(app, lv_color_hex(0xF0F0F0), 0);
        lv_obj_set_style_radius(app, 15, 0);
        
        // Icon placeholder
        lv_obj_t *icon = lv_label_create(app);
        lv_label_set_text(icon, LV_SYMBOL_SETTINGS);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -20);
        
        // App name
        lv_obj_t *label = lv_label_create(app);
        lv_label_set_text(label, app_names[i]);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    }
}

static void update_time(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
    lv_label_set_text(label_time, buf);
}

static void timer_cb(lv_timer_t *timer)
{
    (void)timer;
    update_time();
}

static void boot_timer_cb(lv_timer_t *timer)
{
    lv_scr_load(screen_launcher);
    lv_timer_del(timer);
}

esp_err_t ui_rich_init(void)
{
    ESP_LOGI(TAG, "Initializing Rich UI");
    
    // Create screens
    create_boot_screen();
    create_launcher_screen();
    
    // Show boot screen
    lv_scr_load(screen_boot);
    
    // Switch to launcher after 2 seconds
    lv_timer_t *boot_timer = lv_timer_create(boot_timer_cb, 2000, NULL);
    lv_timer_set_repeat_count(boot_timer, 1);
    
    // Update time every minute
    lv_timer_create(timer_cb, 60000, NULL);
    update_time();
    
    ESP_LOGI(TAG, "Rich UI initialized");
    return ESP_OK;
}

void ui_rich_update_status(const ui_rich_status_t *status)
{
    if (!status) return;
    
    current_status = *status;
    
    // Update battery
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", status->battery_percent);
    lv_label_set_text(label_battery, buf);
    
    // Update charging icon
    if (status->charging) {
        lv_obj_clear_flag(icon_charging, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(icon_charging, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Update WiFi icon
    if (status->wifi_connected) {
        lv_obj_clear_flag(icon_wifi, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(icon_wifi, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Update LoRa icon
    if (status->lora_connected) {
        lv_obj_clear_flag(icon_lora, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(icon_lora, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_rich_show_ota_update(void)
{
    // E-paper is too slow for progress updates
    // Just show static "Updating..." message
    // Implementation depends on LVGL setup
    // TODO: Add LVGL label with "Firmware Update in Progress"
}
