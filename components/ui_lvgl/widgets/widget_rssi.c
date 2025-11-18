#include "lvgl.h"

static lv_obj_t *rssi_label = NULL;

void widget_rssi_create(lv_obj_t *parent) {
    rssi_label = lv_label_create(parent);
    lv_label_set_text(rssi_label, "RSSI: -");
    lv_obj_align(rssi_label, LV_ALIGN_BOTTOM_MID, 0, -2);
}

void widget_rssi_update(int8_t rssi) {
    if (rssi_label) {
        if (rssi == 0) {
            lv_label_set_text(rssi_label, "RSSI: -");
        } else {
            lv_label_set_text_fmt(rssi_label, "RSSI: %ddBm", rssi);
        }
    }
}
