/**
 * @file ui_compact_fonts.h
 * @brief Font declarations for ui_compact (small displays)
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pixolletta bitmap fonts for OLED displays
LV_FONT_DECLARE(lv_font_pixolletta_10); // UI text
LV_FONT_DECLARE(lv_font_pixolletta_20); // Large value displays

// Micro font for version text
LV_FONT_DECLARE(lv_font_micro_10);

// Inter Light fonts for E-Paper displays
LV_FONT_DECLARE(lv_font_1bpp_inter_light_8);
LV_FONT_DECLARE(lv_font_1bpp_inter_light_10);
LV_FONT_DECLARE(lv_font_1bpp_inter_light_12);
LV_FONT_DECLARE(lv_font_1bpp_inter_light_14);
LV_FONT_DECLARE(lv_font_1bpp_inter_light_16);
LV_FONT_DECLARE(lv_font_1bpp_inter_light_18);
LV_FONT_DECLARE(lv_font_1bpp_inter_light_20);

// Conditional font selection based on display type
#if CONFIG_LORACUE_MODEL_BETA || CONFIG_LORACUE_MODEL_GAMMA
    #define UI_FONT_SMALL  &lv_font_1bpp_inter_light_12
    #define UI_FONT_BODY   &lv_font_1bpp_inter_light_14
    #define UI_FONT_LARGE  &lv_font_1bpp_inter_light_18
#else
    #define UI_FONT_SMALL  &lv_font_pixolletta_10
    #define UI_FONT_BODY   &lv_font_pixolletta_10
    #define UI_FONT_LARGE  &lv_font_pixolletta_20
#endif

#ifdef __cplusplus
}
#endif
