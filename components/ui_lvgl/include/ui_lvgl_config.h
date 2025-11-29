#pragma once

// Display dimensions
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

// Layout constants (for 128x64 OLED display)
#define HEADER_HEIGHT 9
#define FOOTER_HEIGHT 10
#define SEPARATOR_Y_TOP 10
#define SEPARATOR_Y_BOTTOM 53

// Content area
#define CONTENT_Y_START (SEPARATOR_Y_TOP + 1)
#define CONTENT_Y_END (SEPARATOR_Y_BOTTOM - 1)
#define CONTENT_HEIGHT (CONTENT_Y_END - CONTENT_Y_START)

// Status icons positioning
#define RF_ICON_X 98
#define RF_ICON_Y 1

#define BATTERY_ICON_X 112
#define BATTERY_ICON_Y 1

#define ICON_SPACING 3

// Text positioning
#define TEXT_MARGIN_LEFT 0
#define TEXT_MARGIN_RIGHT 1
#define BUTTON_MARGIN 5

// Main screen layout
#define PRESENTER_TEXT_Y 27
#define BUTTON_HINTS_Y 39
#define BUTTON_TEXT_Y 38  // LVGL uses top position, not baseline

// Bottom bar
#define BOTTOM_BAR_Y 54
#define BOTTOM_TEXT_Y 55
