#include "lvgl.h"

static lv_obj_t *battery_bar = NULL;
static lv_obj_t *battery_label = NULL;

void widget_battery_create(lv_obj_t *parent) {
    battery_bar = lv_bar_create(parent);
    lv_obj_set_size(battery_bar, 80, 20);
    lv_obj_center(battery_bar);
    lv_bar_set_range(battery_bar, 0, 100);
    lv_bar_set_value(battery_bar, 100, LV_ANIM_OFF);
    
    battery_label = lv_label_create(parent);
    lv_label_set_text(battery_label, "100%");
    lv_obj_align_to(battery_label, battery_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
}

void widget_battery_update(uint8_t level) {
    if (battery_bar) {
        lv_bar_set_value(battery_bar, level, LV_ANIM_ON);
    }
    if (battery_label) {
        lv_label_set_text_fmt(battery_label, "%d%%", level);
    }
}
