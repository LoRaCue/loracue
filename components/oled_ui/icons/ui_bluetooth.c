#include "icons/ui_bluetooth.h"
#include "u8g2.h"
#include "ui_config.h"

extern u8g2_t u8g2;

void ui_bluetooth_draw_at(int x, int y, bool connected)
{
    // Bluetooth symbol (simplified)
    // Vertical line
    u8g2_DrawVLine(&u8g2, x + 3, y, BT_ICON_HEIGHT);
    
    // Top triangle
    u8g2_DrawLine(&u8g2, x + 3, y, x + 6, y + 2);
    u8g2_DrawLine(&u8g2, x + 3, y + 3, x + 6, y + 2);
    
    // Bottom triangle
    u8g2_DrawLine(&u8g2, x + 3, y + 3, x + 6, y + 5);
    u8g2_DrawLine(&u8g2, x + 3, y + 6, x + 6, y + 5);
    
    // Left triangles
    u8g2_DrawLine(&u8g2, x, y + 2, x + 3, y);
    u8g2_DrawLine(&u8g2, x, y + 2, x + 3, y + 3);
    u8g2_DrawLine(&u8g2, x, y + 5, x + 3, y + 3);
    u8g2_DrawLine(&u8g2, x, y + 5, x + 3, y + 6);
}
