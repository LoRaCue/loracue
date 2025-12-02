#pragma once

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef SIGNAL_STRENGTH_T_DEFINED
typedef enum {
    SIGNAL_NONE   = 0,
    SIGNAL_WEAK   = 1,
    SIGNAL_FAIR   = 2,
    SIGNAL_GOOD   = 3,
    SIGNAL_STRONG = 4
} signal_strength_t;
#define SIGNAL_STRENGTH_T_DEFINED
#endif

typedef struct {
    bool usb_connected;
    bool bluetooth_enabled;
    bool bluetooth_connected;
    signal_strength_t signal_strength;
    uint8_t battery_level; // 0-100%
    const char *device_name;
} statusbar_data_t;

lv_obj_t *ui_compact_statusbar_create(lv_obj_t *parent);
void ui_compact_statusbar_update(const lv_obj_t *statusbar, const statusbar_data_t *data);
