#include "lvgl.h"
#include "ui_rich.h"
#include "esp_log.h"

// Declare SF Pro fonts
LV_FONT_DECLARE(sf_pro_24);
LV_FONT_DECLARE(sf_pro_16);

static const char *TAG = "pc_mode_screen";

static void back_btn_cb(lv_event_t *e)
{
    ui_rich_navigate(UI_RICH_SCREEN_HOME);
}

void pc_mode_screen_create(void)
{
    ESP_LOGI(TAG, "Creating PC mode screen");
    
    lv_obj_t *scr = lv_scr_act();
    
    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "PC Mode");
    lv_obj_set_style_text_font(title, &sf_pro_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Status
    lv_obj_t *status = lv_label_create(scr);
    lv_label_set_text(status, "USB HID: Connected");
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 60);
    
    // Info panel
    lv_obj_t *info = lv_obj_create(scr);
    lv_obj_set_size(info, 500, 250);
    lv_obj_center(info);
    
    lv_obj_t *info_text = lv_label_create(info);
    lv_label_set_text(info_text, 
        "PC Mode Active\n\n"
        "Device is acting as USB keyboard\n"
        "Receiving commands from paired devices\n\n"
        "Paired Devices: 2\n"
        "Last Activity: 5s ago");
    lv_obj_center(info_text);
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_set_size(back_btn, 100, 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);
}
