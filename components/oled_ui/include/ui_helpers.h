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
 * @brief Footer context types for one-button UI
 */
typedef enum {
    FOOTER_CONTEXT_MENU,    ///< Next, Back, Select
    FOOTER_CONTEXT_VALUE,   ///< +, -, Save (for editing values)
    FOOTER_CONTEXT_INFO,    ///< Back only
    FOOTER_CONTEXT_CONFIRM, ///< Cancel, Confirm
    FOOTER_CONTEXT_DELETE,  ///< Back, Delete
    FOOTER_CONTEXT_PAIR,    ///< Back, Pair
    FOOTER_CONTEXT_CUSTOM   ///< Custom labels
} footer_context_t;

/**
 * @brief Draw centered text on display
 *
 * @param u8g2 U8g2 display instance
 * @param display_width Width of display in pixels
 * @param y Y coordinate for text baseline
 * @param str String to draw centered
 */
void u8g2_DrawCenterStr(u8g2_t *u8g2, int display_width, int y, const char *str);

/**
 * @brief Draw standardized header with title and separator line
 *
 * @param title Header title text
 */
void ui_draw_header(const char *title);

/**
 * @brief Draw footer with one-button UI icons
 *
 * @param context Footer context type
 * @param custom_labels Array of 3 custom labels (for FOOTER_CONTEXT_CUSTOM), can be NULL
 */
void ui_draw_footer(footer_context_t context, const char *custom_labels[3]);

#ifdef __cplusplus
}
#endif
