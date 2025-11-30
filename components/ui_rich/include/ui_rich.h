/**
 * @file ui_rich.h
 * @brief Rich UI for e-paper displays (LVGL-based)
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// cppcheck-suppress unusedStructMember
typedef struct {
    uint8_t battery_percent;
    bool charging;
    bool lora_connected;
    bool wifi_connected;
} ui_rich_status_t;

esp_err_t ui_rich_init(void);
void ui_rich_update_status(const ui_rich_status_t *status);
void ui_rich_show_ota_update(void);

#ifdef __cplusplus
}
#endif
