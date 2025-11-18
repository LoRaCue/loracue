#include "lvgl.h"

static lv_obj_t *battery_label = NULL;
static lv_obj_t *usb_label = NULL;
static lv_obj_t *ble_label = NULL;

void widget_statusbar_create(lv_obj_t *parent) {
    battery_label = lv_label_create(parent);
    lv_label_set_text(battery_label, "BAT:100%");
    lv_obj_align(battery_label, LV_ALIGN_TOP_LEFT, 2, 2);
    
    usb_label = lv_label_create(parent);
    lv_label_set_text(usb_label, "USB");
    lv_obj_align(usb_label, LV_ALIGN_TOP_MID, 0, 2);
    
    ble_label = lv_label_create(parent);
    lv_label_set_text(ble_label, "BLE");
    lv_obj_align(ble_label, LV_ALIGN_TOP_RIGHT, -2, 2);
}

void widget_statusbar_update_battery(uint8_t level, bool charging) {
    if (battery_label) {
        lv_label_set_text_fmt(battery_label, "BAT:%d%%%s", level, charging ? "+" : "");
    }
}

void widget_statusbar_update_usb(bool connected) {
    if (usb_label) {
        lv_obj_set_style_opa(usb_label, connected ? LV_OPA_COVER : LV_OPA_30, 0);
    }
}

void widget_statusbar_update_ble(bool enabled) {
    if (ble_label) {
        lv_obj_set_style_opa(ble_label, enabled ? LV_OPA_COVER : LV_OPA_30, 0);
    }
}
