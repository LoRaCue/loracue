#pragma once

#include <stdbool.h>
#include <stdint.h>

// Display dimensions
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

// Layout constants
#define HEADER_HEIGHT 9
#define FOOTER_HEIGHT 10
#define SEPARATOR_Y_TOP 9
#define SEPARATOR_Y_BOTTOM 53

// Status icons configuration
// USB and Bluetooth are dynamic (only show when active)
// RF and Battery are fixed positions on the right
#define USB_ICON_WIDTH 10
#define USB_ICON_HEIGHT 6

#define BT_ICON_WIDTH 5
#define BT_ICON_HEIGHT 8

#define RF_ICON_X 113
#define RF_ICON_Y 2
#define RF_ICON_BARS 4
#define RF_ICON_HEIGHT 6

#define BATTERY_ICON_X 123
#define BATTERY_ICON_Y 2
#define BATTERY_SEGMENTS 4
#define BATTERY_SEGMENT_WIDTH 2
#define BATTERY_ICON_HEIGHT 6

#define ICON_Y 2
#define ICON_SPACING 3

// Text positioning
#define TEXT_MARGIN_LEFT 0
#define TEXT_MARGIN_RIGHT 1
#define BUTTON_MARGIN 5

// UI data structures
typedef enum {
    SIGNAL_NONE   = 0,
    SIGNAL_WEAK   = 1,
    SIGNAL_FAIR   = 2,
    SIGNAL_GOOD   = 3,
    SIGNAL_STRONG = 4
} signal_strength_t;

typedef struct {
    bool usb_connected;
    bool bluetooth_enabled;
    bool bluetooth_connected;
    bool lora_connected;
    signal_strength_t signal_strength;
    uint8_t battery_level; // 0-100%
    char device_name[16];
    int16_t lora_signal;   // RSSI in dBm
    char last_command[16]; // Last received command
    uint8_t active_presenter_count;
    uint8_t command_history_count;
} ui_status_t;

typedef struct {
    uint8_t x, y, width, height;
} ui_rect_t;
