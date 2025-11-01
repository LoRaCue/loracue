/**
 * @file ota_update_screen.c
 * @brief OTA firmware update screen
 */

#include "ui_mini.h"
#include "ui_screen_controller.h"
#include "u8g2.h"
#include <stdio.h>

extern u8g2_t u8g2;
extern uint8_t ui_mini_get_ota_progress(void);

void ui_screen_ota_update(void)
{
    uint8_t progress = ui_mini_get_ota_progress();
    char progress_str[16];
    
    u8g2_ClearBuffer(&u8g2);
    
    // Title
    u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
    u8g2_DrawStr(&u8g2, 20, 15, "Updating...");
    
    // Progress bar background
    u8g2_DrawFrame(&u8g2, 10, 25, 108, 12);
    
    // Progress bar fill
    uint8_t fill_width = (progress * 104) / 100;
    if (fill_width > 0) {
        u8g2_DrawBox(&u8g2, 12, 27, fill_width, 8);
    }
    
    // Progress percentage
    snprintf(progress_str, sizeof(progress_str), "%d%%", progress);
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    uint8_t str_width = u8g2_GetStrWidth(&u8g2, progress_str);
    u8g2_DrawStr(&u8g2, (128 - str_width) / 2, 50, progress_str);
    
    // Warning message
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    u8g2_DrawStr(&u8g2, 10, 62, "Do not power off!");
    
    u8g2_SendBuffer(&u8g2);
}
