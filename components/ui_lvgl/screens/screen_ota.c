#include "lvgl.h"

static lv_obj_t *progress_bar = NULL;
static lv_obj_t *progress_label = NULL;

void screen_ota_create(lv_obj_t *parent) {
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "OTA Update");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    progress_bar = lv_bar_create(parent);
    lv_obj_set_size(progress_bar, 100, 10);
    lv_obj_center(progress_bar);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    
    progress_label = lv_label_create(parent);
    lv_label_set_text(progress_label, "0%");
    lv_obj_align_to(progress_label, progress_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
}

void screen_ota_update_progress(uint8_t percent) {
    if (progress_bar) {
        lv_bar_set_value(progress_bar, percent, LV_ANIM_ON);
    }
    if (progress_label) {
        lv_label_set_text_fmt(progress_label, "%d%%", percent);
    }
}
