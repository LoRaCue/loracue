#include "lvgl.h"

void screen_boot_create(lv_obj_t *parent) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, "LoRaCue\nBooting...");
    lv_obj_center(label);
}
