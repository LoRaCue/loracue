#include "ota_error_screen.h"
#include "u8g2.h"
#include "ui_config.h"

extern u8g2_t u8g2;

void ota_error_screen_draw(const char *current_board, const char *new_board)
{
    u8g2_ClearBuffer(&u8g2);

    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "FIRMWARE ERROR");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);

    // Error message
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 22, "Incompatible firmware");
    
    // Current board
    u8g2_DrawStr(&u8g2, 2, 34, "Current:");
    u8g2_DrawStr(&u8g2, 50, 34, current_board);
    
    // New board
    u8g2_DrawStr(&u8g2, 2, 44, "New:");
    u8g2_DrawStr(&u8g2, 50, 44, new_board);

    // Footer
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 62, "Update aborted");

    u8g2_SendBuffer(&u8g2);
}
