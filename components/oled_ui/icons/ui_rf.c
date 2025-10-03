#include "icons/ui_rf.h"
#include "u8g2.h"
#include "ui_config.h"

extern u8g2_t u8g2;

void ui_rf_draw(signal_strength_t strength)
{
    // Draw baseline for all bars (1 pixel each) to show capacity
    for (int i = 0; i < RF_ICON_BARS; i++) {
        uint8_t bar_x      = RF_ICON_X + (i * 2);
        uint8_t baseline_y = RF_ICON_Y + RF_ICON_HEIGHT - 1;
        u8g2_DrawPixel(&u8g2, bar_x, baseline_y);
    }

    if (strength == SIGNAL_NONE)
        return; // Only baseline visible

    // Draw signal bars based on strength
    for (int i = 0; i < RF_ICON_BARS; i++) {
        if (i < strength) {
            uint8_t bar_x      = RF_ICON_X + (i * 2);
            uint8_t bar_height = i + 2; // Bars get taller: 2, 3, 4, 5 pixels
            uint8_t bar_y      = RF_ICON_Y + RF_ICON_HEIGHT - bar_height;

            // Last bar (strongest) uses full height
            if (i == RF_ICON_BARS - 1) {
                bar_height = RF_ICON_HEIGHT;
                bar_y      = RF_ICON_Y;
            }

            u8g2_DrawVLine(&u8g2, bar_x, bar_y, bar_height);
        }
    }
}
