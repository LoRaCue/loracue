#include "icons/ui_bluetooth.h"
#include "u8g2.h"
#include "ui_config.h"

extern u8g2_t u8g2;

void ui_bluetooth_draw(bool enabled, bool connected)
{
    if (!enabled)
        return; // Don't draw if disabled

    // Bluetooth symbol (simplified)
    // Vertical line
    u8g2_DrawVLine(&u8g2, BT_ICON_X + 3, BT_ICON_Y, BT_ICON_HEIGHT);
    
    // Top triangle
    u8g2_DrawLine(&u8g2, BT_ICON_X + 3, BT_ICON_Y, BT_ICON_X + 6, BT_ICON_Y + 2);
    u8g2_DrawLine(&u8g2, BT_ICON_X + 3, BT_ICON_Y + 3, BT_ICON_X + 6, BT_ICON_Y + 2);
    
    // Bottom triangle
    u8g2_DrawLine(&u8g2, BT_ICON_X + 3, BT_ICON_Y + 3, BT_ICON_X + 6, BT_ICON_Y + 5);
    u8g2_DrawLine(&u8g2, BT_ICON_X + 3, BT_ICON_Y + 6, BT_ICON_X + 6, BT_ICON_Y + 5);
    
    // Left triangles
    u8g2_DrawLine(&u8g2, BT_ICON_X, BT_ICON_Y + 2, BT_ICON_X + 3, BT_ICON_Y);
    u8g2_DrawLine(&u8g2, BT_ICON_X, BT_ICON_Y + 2, BT_ICON_X + 3, BT_ICON_Y + 3);
    u8g2_DrawLine(&u8g2, BT_ICON_X, BT_ICON_Y + 5, BT_ICON_X + 3, BT_ICON_Y + 3);
    u8g2_DrawLine(&u8g2, BT_ICON_X, BT_ICON_Y + 5, BT_ICON_X + 3, BT_ICON_Y + 6);
}
