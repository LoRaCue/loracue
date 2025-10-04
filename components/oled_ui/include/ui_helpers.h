/**
 * @file ui_helpers.h
 * @brief UI helper functions for common display operations
 */

#pragma once

#include "u8g2.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Draw centered text on display
 * 
 * @param u8g2 U8g2 display instance
 * @param display_width Width of display in pixels
 * @param y Y coordinate for text baseline
 * @param str String to draw centered
 */
void u8g2_DrawCenterStr(u8g2_t *u8g2, int display_width, int y, const char *str);

#ifdef __cplusplus
}
#endif
