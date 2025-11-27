#include "lvgl.h"
#include "ui_components.h"
#include "screens.h"
#include "esp_log.h"

static const char *TAG = "lora_submenu";
static ui_menu_t *menu = NULL;

const char *LORA_MENU_ITEMS[LORA_MENU_COUNT] = {
    [LORA_MENU_PRESETS] = "Presets",
    [LORA_MENU_FREQUENCY] = "Frequency",
    [LORA_MENU_SF] = "Spreading Factor",
    [LORA_MENU_BW] = "Bandwidth",
    [LORA_MENU_CR] = "Coding Rate",
    [LORA_MENU_TXPOWER] = "TX-Power",
    [LORA_MENU_BAND] = "Band"
};

void screen_lora_submenu_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(parent);
    lv_obj_add_style(title, &style_title, 0);
    lv_label_set_text(title, "LORA SETTINGS");
    lv_obj_set_pos(title, UI_MARGIN_LEFT, 0);
    
    // Preserve selected index if menu already exists
    int selected = menu ? menu->selected_index : 0;
    
    // Create menu
    menu = ui_menu_create(parent, LORA_MENU_ITEMS, LORA_MENU_COUNT);
    menu->selected_index = selected;
    ui_menu_update(menu, LORA_MENU_ITEMS);
}

void screen_lora_submenu_navigate_down(void) {
    if (!menu) return;
    menu->selected_index = (menu->selected_index + 1) % LORA_MENU_COUNT;
    ui_menu_update(menu, LORA_MENU_ITEMS);
}

void screen_lora_submenu_select(void) {
    if (!menu) return;
    ESP_LOGI(TAG, "LoRa submenu item selected: %d - %s", 
             menu->selected_index, LORA_MENU_ITEMS[menu->selected_index]);
}

int screen_lora_submenu_get_selected(void) {
    return menu ? menu->selected_index : 0;
}

void screen_lora_submenu_reset(void) {
    if (menu) {
        free(menu);
        menu = NULL;
    }
}
