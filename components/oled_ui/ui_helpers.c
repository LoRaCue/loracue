/**
 * @file ui_helpers.c
 * @brief UI helper functions implementation
 */

#include "ui_helpers.h"
#include "ui_config.h"
#include "icons/ui_status_icons.h"

extern u8g2_t u8g2;

void u8g2_DrawCenterStr(u8g2_t *u8g2, int display_width, int y, const char *str)
{
    int str_width = u8g2_GetStrWidth(u8g2, str);
    int x = (display_width - str_width) / 2;
    u8g2_DrawStr(u8g2, x, y, str);
}

void ui_draw_footer(footer_context_t context, const char *custom_labels[3])
{
    u8g2_DrawHLine(&u8g2, 0, 54, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    switch (context) {
        case FOOTER_CONTEXT_MENU:
            ui_button_short_draw_at(2, 56);
            u8g2_DrawStr(&u8g2, 11, 64, "Next");
            ui_button_double_draw_at(40, 56);
            u8g2_DrawStr(&u8g2, 55, 64, "Back");
            ui_button_long_draw_at(86, 56);  // -1px (was 87)
            u8g2_DrawStr(&u8g2, 101, 64, "Select");  // -1px (was 102)
            break;
            
        case FOOTER_CONTEXT_EDIT:
            ui_button_short_draw_at(2, 56);
            u8g2_DrawStr(&u8g2, 11, 64, "+");
            ui_button_double_draw_at(30, 56);
            u8g2_DrawStr(&u8g2, 45, 64, "-");
            ui_button_long_draw_at(82, 56);  // +3px right (was 79)
            u8g2_DrawStr(&u8g2, 104, 64, "Save");  // -3px left (was 107)
            break;
            
        case FOOTER_CONTEXT_INFO:
            ui_button_double_draw_at(2, 56);
            u8g2_DrawStr(&u8g2, 17, 64, "Back");
            break;
            
        case FOOTER_CONTEXT_CONFIRM:
            ui_button_double_draw_at(2, 56);
            u8g2_DrawStr(&u8g2, 17, 64, "Cancel");
            ui_button_long_draw_at(70, 56);
            u8g2_DrawStr(&u8g2, 85, 64, "Confirm");
            break;
            
        case FOOTER_CONTEXT_CUSTOM:
            if (custom_labels && custom_labels[0]) {
                ui_button_short_draw_at(2, 56);
                u8g2_DrawStr(&u8g2, 11, 64, custom_labels[0]);
            }
            if (custom_labels && custom_labels[1]) {
                ui_button_double_draw_at(35, 56);  // -5px left (was 40)
                u8g2_DrawStr(&u8g2, 50, 64, custom_labels[1]);  // -5px left (was 55)
            }
            if (custom_labels && custom_labels[2]) {
                ui_button_long_draw_at(94, 56);  // +5px (was 89)
                u8g2_DrawStr(&u8g2, 109, 64, custom_labels[2]);  // +5px (was 104)
            }
            break;
    }
}
