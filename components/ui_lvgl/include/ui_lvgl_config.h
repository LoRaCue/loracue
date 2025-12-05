#pragma once

// Display dimensions and layout constants
#if CONFIG_LORACUE_MODEL_BETA || CONFIG_LORACUE_MODEL_GAMMA
    // E-Paper display (250x122)
    #define DISPLAY_WIDTH 250
    #define DISPLAY_HEIGHT 122
    
    #define HEADER_HEIGHT 18
    #define FOOTER_HEIGHT 20
    #define SEPARATOR_Y_TOP 18
    #define SEPARATOR_Y_BOTTOM (DISPLAY_HEIGHT - FOOTER_HEIGHT - 2)
    
    #define RF_ICON_X 196
    #define RF_ICON_Y 2
    #define BATTERY_ICON_X 224
    #define BATTERY_ICON_Y 2
    #define ICON_SPACING 6
    
    #define TEXT_MARGIN_LEFT 0
    #define TEXT_MARGIN_RIGHT 2
    #define BUTTON_MARGIN 10
    
    #define PRESENTER_TEXT_Y 52
    #define BUTTON_HINTS_Y 76
    #define BUTTON_TEXT_Y 74
    
    #define BOTTOM_BAR_Y (DISPLAY_HEIGHT - FOOTER_HEIGHT - 4)
    #define BOTTOM_TEXT_Y (BOTTOM_BAR_Y + 2)
#else
    // OLED display (128x64) - Alpha/Alpha+
    #define DISPLAY_WIDTH 128
    #define DISPLAY_HEIGHT 64
    
    #define HEADER_HEIGHT 9
    #define FOOTER_HEIGHT 10
    #define SEPARATOR_Y_TOP 10
    #define SEPARATOR_Y_BOTTOM (DISPLAY_HEIGHT - FOOTER_HEIGHT - 1)
    
    #define RF_ICON_X 98
    #define RF_ICON_Y 1
    #define BATTERY_ICON_X 112
    #define BATTERY_ICON_Y 1
    #define ICON_SPACING 3
    
    #define TEXT_MARGIN_LEFT 0
    #define TEXT_MARGIN_RIGHT 1
    #define BUTTON_MARGIN 5
    
    #define PRESENTER_TEXT_Y 27
    #define BUTTON_HINTS_Y 39
    #define BUTTON_TEXT_Y 38
    
    #define BOTTOM_BAR_Y (DISPLAY_HEIGHT - FOOTER_HEIGHT - 4)
    #define BOTTOM_TEXT_Y (BOTTOM_BAR_Y + 1)
#endif

// Content area (calculated from separators)
#define CONTENT_Y_START (SEPARATOR_Y_TOP + 1)
#define CONTENT_Y_END (SEPARATOR_Y_BOTTOM - 1)
#define CONTENT_HEIGHT (CONTENT_Y_END - CONTENT_Y_START)
