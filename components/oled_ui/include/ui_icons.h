/**
 * @file ui_icons.h
 * @brief Central icon definitions for OLED UI
 * 
 * All bitmap icons used throughout the UI system.
 * Format: XBM (X11 Bitmap) for u8g2 compatibility.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Scroll arrows for menu navigation
#define scroll_up_width 8
#define scroll_up_height 8
static const unsigned char scroll_up_bits[] = {
   0x18, 0x3c, 0x7e, 0xff, 0x18, 0x18, 0x18, 0x00 };

#define scroll_down_width 8
#define scroll_down_height 8
static const unsigned char scroll_down_bits[] = {
   0x00, 0x18, 0x18, 0x18, 0xff, 0x7e, 0x3c, 0x18 };

#ifdef __cplusplus
}
#endif
