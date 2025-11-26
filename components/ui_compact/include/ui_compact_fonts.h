/**
 * @file ui_compact_fonts.h
 * @brief Font declarations for ui_compact (small displays)
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pixolletta bitmap fonts for small displays
LV_FONT_DECLARE(lv_font_pixolletta_10);  // UI text
LV_FONT_DECLARE(lv_font_pixolletta_20);  // Large value displays

// Micro font for version text
LV_FONT_DECLARE(lv_font_micro_5);

#ifdef __cplusplus
}
#endif
