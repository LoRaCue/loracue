#include "pc_main_screen.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_status_bar.h"

extern u8g2_t u8g2;

void pc_main_screen_draw(const ui_status_t *status)
{
    u8g2_ClearBuffer(&u8g2);

    // Draw status bars
    ui_status_bar_draw(status);

    // Event log area (TODO: Replace with real event log data)
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 21, "14:23:15 RC-A4B2 NEXT");
    u8g2_DrawStr(&u8g2, 2, 29, "14:23:12 RC-A4B2 PREV");
    u8g2_DrawStr(&u8g2, 2, 37, "14:23:08 RC-A4B2 NEXT");
    u8g2_DrawStr(&u8g2, 2, 45, "14:22:45 RC-A4B2 PREV");

    // Bottom separator
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);

    // Bottom bar
    ui_bottom_bar_draw(status);

    u8g2_SendBuffer(&u8g2);
}
