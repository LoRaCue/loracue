#include "lvgl.h"

static lv_obj_t *menu_list = NULL;

void screen_menu_create(lv_obj_t *parent) {
    menu_list = lv_list_create(parent);
    lv_obj_set_size(menu_list, lv_pct(100), lv_pct(100));
    
    lv_list_add_text(menu_list, "Settings");
    lv_list_add_button(menu_list, NULL, "Device Mode");
    lv_list_add_button(menu_list, NULL, "LoRa Settings");
    lv_list_add_button(menu_list, NULL, "Battery Info");
    lv_list_add_button(menu_list, NULL, "System Info");
}
