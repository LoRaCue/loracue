#include "lvgl.h"
#include "ui_rich.h"
#include "esp_log.h"

// Declare SF Pro fonts
LV_FONT_DECLARE(sf_pro_24);
LV_FONT_DECLARE(sf_pro_16);

static const char *TAG = "settings_screen";

static void back_btn_cb(lv_event_t *e)
{
    ui_rich_navigate(UI_RICH_SCREEN_HOME);
}

void settings_screen_create(void)
{
    ESP_LOGI(TAG, "Creating settings screen");
    
    lv_obj_t *scr = lv_scr_act();
    
    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &sf_pro_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Settings list
    lv_obj_t *list = lv_list_create(scr);
    lv_obj_set_size(list, 400, 300);
    lv_obj_center(list);
    
    lv_list_add_text(list, "Device Configuration");
    lv_list_add_btn(list, NULL, "Device Mode");
    lv_list_add_btn(list, NULL, "LoRa Settings");
    lv_list_add_btn(list, NULL, "Display Brightness");
    lv_list_add_btn(list, NULL, "Power Management");
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_set_size(back_btn, 100, 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);
}
