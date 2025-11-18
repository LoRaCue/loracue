#pragma once

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Widget creation functions
void widget_statusbar_create(lv_obj_t *parent);
void widget_battery_create(lv_obj_t *parent);
void widget_rssi_create(lv_obj_t *parent);

// Widget update functions
void widget_statusbar_update_battery(uint8_t level, bool charging);
void widget_statusbar_update_usb(bool connected);
void widget_statusbar_update_ble(bool enabled);
void widget_battery_update(uint8_t level);
void widget_rssi_update(int8_t rssi);

#ifdef __cplusplus
}
#endif
