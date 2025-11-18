#include "lvgl.h"

void screen_settings_create(lv_obj_t *parent) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, "Settings");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
}
