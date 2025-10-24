#include "lvgl.h"
#include "ui_rich.h"
#include "esp_log.h"

// Declare SF Pro fonts
LV_FONT_DECLARE(sf_pro_24);
LV_FONT_DECLARE(sf_pro_16);

static const char *TAG = "home_screen";

static void settings_btn_cb(lv_event_t *e)
{
    ui_rich_navigate(UI_RICH_SCREEN_SETTINGS);
}

static void presenter_btn_cb(lv_event_t *e)
{
    ui_rich_navigate(UI_RICH_SCREEN_PRESENTER);
}

static void pc_mode_btn_cb(lv_event_t *e)
{
    ui_rich_navigate(UI_RICH_SCREEN_PC_MODE);
}

static lv_obj_t *create_app_icon(lv_obj_t *parent, const char *label, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 120, 120);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_center(lbl);
    
    return btn;
}

void home_screen_create(void)
{
    ESP_LOGI(TAG, "Creating home screen");
    
    lv_obj_t *scr = lv_scr_act();
    
    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "LoRaCue");
    lv_obj_set_style_text_font(title, &sf_pro_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Icon grid container
    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_set_size(grid, 400, 300);
    lv_obj_center(grid);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(grid, 20, 0);
    lv_obj_set_style_pad_gap(grid, 20, 0);
    
    // Create app icons
    create_app_icon(grid, "Settings", settings_btn_cb);
    create_app_icon(grid, "Presenter", presenter_btn_cb);
    create_app_icon(grid, "PC Mode", pc_mode_btn_cb);
}
