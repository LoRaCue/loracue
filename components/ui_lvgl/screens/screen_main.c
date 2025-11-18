#include "lvgl.h"

static lv_obj_t *status_label = NULL;
static lv_obj_t *mode_label = NULL;

void screen_main_create(lv_obj_t *parent) {
    mode_label = lv_label_create(parent);
    lv_label_set_text(mode_label, "PRESENTER");
    lv_obj_align(mode_label, LV_ALIGN_TOP_MID, 0, 10);
    
    status_label = lv_label_create(parent);
    lv_label_set_text(status_label, "Ready");
    lv_obj_center(status_label);
}

void screen_main_update_mode(const char *mode) {
    if (mode_label) {
        lv_label_set_text(mode_label, mode);
    }
}

void screen_main_update_status(const char *status) {
    if (status_label) {
        lv_label_set_text(status_label, status);
    }
}
