#include "lvgl.h"
#include "ui_components.h"
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
    
    general_config_t config;
    general_config_get(&config);
    
    static char bt_text[32], pair_text[32];
    snprintf(bt_text, sizeof(bt_text), "Bluetooth: %s", config.bluetooth_enabled ? "ON" : "OFF");
    snprintf(pair_text, sizeof(pair_text), "Pairing: %s", config.bluetooth_pairing_enabled ? "ON" : "OFF");
    
    const char *items[] = {bt_text, pair_text};
    menu = ui_menu_create(parent, items, 2);
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
