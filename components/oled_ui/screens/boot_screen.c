#include "boot_screen.h"
#include "ui_config.h"
#include "version.h"
#include "u8g2.h"

extern u8g2_t u8g2;

void boot_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Main title "LORACUE" (14pt Helvetica Bold, centered)
    u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
    int title_width = u8g2_GetStrWidth(&u8g2, "LORACUE");
    u8g2_DrawStr(&u8g2, (DISPLAY_WIDTH - title_width) / 2, 32, "LORACUE");
    
    // Subtitle "Enterprise Remote" (8pt Helvetica Regular, centered)
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    int subtitle_width = u8g2_GetStrWidth(&u8g2, "Enterprise Remote");
    u8g2_DrawStr(&u8g2, (DISPLAY_WIDTH - subtitle_width) / 2, 45, "Enterprise Remote");
    
    // Version and status (8pt Helvetica Regular, corners)
    u8g2_DrawStr(&u8g2, 0, 60, LORACUE_VERSION_STRING);
    int init_width = u8g2_GetStrWidth(&u8g2, "Initializing...");
    u8g2_DrawStr(&u8g2, DISPLAY_WIDTH - init_width - TEXT_MARGIN_RIGHT, 60, "Initializing...");
    
    u8g2_SendBuffer(&u8g2);
}
