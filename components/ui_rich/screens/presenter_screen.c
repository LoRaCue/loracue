#include "lvgl.h"
#include "ui_rich.h"
#include "esp_log.h"

// Declare SF Pro fonts
LV_FONT_DECLARE(sf_pro_bold_24);
LV_FONT_DECLARE(sf_pro_24);
LV_FONT_DECLARE(sf_pro_16);

static const char *TAG = "presenter_screen";

static void back_btn_cb(lv_event_t *e)
{
    ui_rich_navigate(UI_RICH_SCREEN_HOME);
}

static void prev_btn_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Previous slide");
}

static void next_btn_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Next slide");
}

void presenter_screen_create(void)
{
    ESP_LOGI(TAG, "Creating presenter screen");
    
    lv_obj_t *scr = lv_scr_act();
    
    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Presenter Mode");
    lv_obj_set_style_text_font(title, &sf_pro_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Status info
    lv_obj_t *status = lv_label_create(scr);
    lv_label_set_text(status, "Connected | Battery: 85% | LoRa: Active");
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 60);
    
    // Control buttons container
    lv_obj_t *btn_container = lv_obj_create(scr);
    lv_obj_set_size(btn_container, 500, 200);
    lv_obj_center(btn_container);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Previous button
    lv_obj_t *prev_btn = lv_btn_create(btn_container);
    lv_obj_set_size(prev_btn, 200, 150);
    lv_obj_add_event_cb(prev_btn, prev_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *prev_lbl = lv_label_create(prev_btn);
    lv_label_set_text(prev_lbl, "< PREV");
    lv_obj_set_style_text_font(prev_lbl, &sf_pro_bold_24, 0);
    lv_obj_center(prev_lbl);
    
    // Next button
    lv_obj_t *next_btn = lv_btn_create(btn_container);
    lv_obj_set_size(next_btn, 200, 150);
    lv_obj_add_event_cb(next_btn, next_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, "NEXT >");
    lv_obj_set_style_text_font(next_lbl, &sf_pro_bold_24, 0);
    lv_obj_center(next_lbl);
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_set_size(back_btn, 100, 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);
}
