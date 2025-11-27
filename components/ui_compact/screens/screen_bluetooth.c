#include "lvgl.h"
#include "ui_components.h"
#include "ui_lvgl_config.h"
#include "general_config.h"
#include "ble.h"
#include "esp_log.h"

static const char *TAG = "bluetooth";
static ui_menu_t *menu = NULL;

void screen_bluetooth_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    lv_obj_t *title = lv_label_create(parent);
    lv_obj_add_style(title, &style_title, 0);
    lv_label_set_text(title, "BLUETOOTH");
    lv_obj_set_pos(title, UI_MARGIN_LEFT, 0);
    
    // Top separator
    lv_obj_t *line = lv_line_create(parent);
    static lv_point_precise_t points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(line, points, 2);
    lv_obj_set_style_line_color(line, lv_color_white(), 0);
    lv_obj_set_style_line_width(line, 1, 0);
    
    // Bottom separator
    lv_obj_t *bottom_line = lv_line_create(parent);
    static lv_point_precise_t bottom_points[] = {{0, SEPARATOR_Y_BOTTOM}, {DISPLAY_WIDTH, SEPARATOR_Y_BOTTOM}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);
    
    general_config_t config;
    general_config_get(&config);
    
    static char bt_text[32], pair_text[32];
    snprintf(bt_text, sizeof(bt_text), "Bluetooth: %s", config.bluetooth_enabled ? "ON" : "OFF");
    snprintf(pair_text, sizeof(pair_text), "Pairing: %s", config.bluetooth_pairing_enabled ? "ON" : "OFF");
    
    const char *items[] = {bt_text, pair_text};
    menu = ui_menu_create(parent, items, 2);
    ui_menu_update(menu, items);
}

void screen_bluetooth_navigate_down(void) {
    if (!menu) return;
    menu->selected_index = (menu->selected_index + 1) % 2;
    
    general_config_t config;
    general_config_get(&config);
    
    static char bt_text[32], pair_text[32];
    snprintf(bt_text, sizeof(bt_text), "Bluetooth: %s", config.bluetooth_enabled ? "ON" : "OFF");
    snprintf(pair_text, sizeof(pair_text), "Pairing: %s", config.bluetooth_pairing_enabled ? "ON" : "OFF");
    
    const char *items[] = {bt_text, pair_text};
    ui_menu_update(menu, items);
}

void screen_bluetooth_select(void) {
    if (!menu) return;
    
    general_config_t config;
    general_config_get(&config);
    
    if (menu->selected_index == 0) {
        config.bluetooth_enabled = !config.bluetooth_enabled;
        ble_set_enabled(config.bluetooth_enabled);
    } else {
        config.bluetooth_pairing_enabled = !config.bluetooth_pairing_enabled;
    }
    
    general_config_set(&config);
    ESP_LOGI(TAG, "Bluetooth setting toggled: item %d", menu->selected_index);
}

void screen_bluetooth_reset(void) {
    if (menu) {
        free(menu);
        menu = NULL;
    }
}
