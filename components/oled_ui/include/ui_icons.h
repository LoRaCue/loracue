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

// Media player track navigation icons (4x7 pixels)
#define track_prev_width 4
#define track_prev_height 7
static const unsigned char track_prev_bits[] = {
   0xf8, 0xfc, 0xfe, 0xff, 0xfe, 0xfc, 0xf8 };

#define track_next_width 4
#define track_next_height 7
static const unsigned char track_next_bits[] = {
   0xf1, 0xf3, 0xf7, 0xff, 0xf7, 0xf3, 0xf1 };

// Combined both buttons icon (◀+▶)
#define both_buttons_width 13
#define both_buttons_height 7
static const unsigned char both_buttons_bits[] = {
   0x08, 0xe2, 0x0c, 0xe6, 0x4e, 0xee, 0xef, 0xfe, 
   0x4e, 0xee, 0x0c, 0xe6, 0x08, 0xe2 };

// Up/Down navigation icon (◀/▶)
#define updown_nav_width 13
#define updown_nav_height 7
static const unsigned char updown_nav_bits[] = {
   0x88, 0xe2, 0x8c, 0xe6, 0x4e, 0xee, 0x4f, 0xfe, 
   0x4e, 0xee, 0x2c, 0xe6, 0x28, 0xe2 };

// Checkmark icon
#define checkmark_width 9
#define checkmark_height 7
static const unsigned char checkmark_bits[] = {
   0x80, 0xff, 0xc0, 0xfe, 0x60, 0xfe, 0x31, 0xfe, 
   0x1b, 0xfe, 0x0e, 0xfe, 0x04, 0xfe };

#ifdef __cplusplus
}
#endif
