#include "icons/ui_battery.h"
#include "ui_config.h"
#include "u8g2.h"

extern u8g2_t u8g2;

void ui_battery_draw(uint8_t battery_level) {
    // Calculate how many segments to fill (0-4)
    uint8_t filled_segments = (battery_level * BATTERY_SEGMENTS + 50) / 100;  // Round to nearest
    
    for (int i = 0; i < BATTERY_SEGMENTS; i++) {
        uint8_t segment_x = BATTERY_ICON_X + (i * (BATTERY_SEGMENT_WIDTH + 1));
        
        if (i < filled_segments) {
            // Draw filled segment
            u8g2_DrawBox(&u8g2, segment_x, BATTERY_ICON_Y, BATTERY_SEGMENT_WIDTH, BATTERY_ICON_HEIGHT);
        } else {
            // Draw empty segment outline
            u8g2_DrawFrame(&u8g2, segment_x, BATTERY_ICON_Y, BATTERY_SEGMENT_WIDTH, BATTERY_ICON_HEIGHT);
        }
    }
}
