#include "main_screen.h"
#include "ui_config.h"
#include "ui_status_bar.h"
#include "u8g2.h"

extern u8g2_t u8g2;

void main_screen_draw(const ui_status_t* status) {
    u8g2_ClearBuffer(&u8g2);
    
    // Draw status bars
    ui_status_bar_draw(status);
    
    // Main content area - "PRESENTER"
    u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
    int title_width = u8g2_GetStrWidth(&u8g2, "PRESENTER");
    u8g2_DrawStr(&u8g2, (DISPLAY_WIDTH - title_width) / 2, 30, "PRESENTER");
    
    // Button hints - symmetrically spaced
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, BUTTON_MARGIN, 43, "[<PREV]");
    int next_width = u8g2_GetStrWidth(&u8g2, "[NEXT>]");
    u8g2_DrawStr(&u8g2, DISPLAY_WIDTH - next_width - BUTTON_MARGIN, 43, "[NEXT>]");
    
    // Bottom separator
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    
    // Bottom bar
    ui_bottom_bar_draw(status);
    
    u8g2_SendBuffer(&u8g2);
}
