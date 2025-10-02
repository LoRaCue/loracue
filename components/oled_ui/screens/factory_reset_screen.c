#include "factory_reset_screen.h"
#include "ui_screen_controller.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "u8g2.h"

extern u8g2_t u8g2;

void factory_reset_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    
    // Header
    u8g2_DrawStr(&u8g2, 2, 10, "FACTORY RESET");
    u8g2_DrawHLine(&u8g2, 0, 12, 128);
    
    // Centered instruction text
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    const char* line1 = "Press     5 sec";
    const char* line2 = "for factory reset!";
    
    int line1_width = u8g2_GetStrWidth(&u8g2, line1);
    int line2_width = u8g2_GetStrWidth(&u8g2, line2);
    int x1 = (128 - line1_width) / 2;
    int x2 = (128 - line2_width) / 2;
    
    u8g2_DrawStr(&u8g2, x1, 32, "Press");
    u8g2_DrawXBM(&u8g2, x1 + 30, 24, both_buttons_width, both_buttons_height, both_buttons_bits);
    u8g2_DrawStr(&u8g2, x1 + 30 + both_buttons_width + 2, 32, "5 sec");
    u8g2_DrawStr(&u8g2, x2, 44, line2);
    
    // Bottom bar
    u8g2_DrawHLine(&u8g2, 0, 52, 128);
    u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, 8, 64, "Back");
    
    u8g2_SendBuffer(&u8g2);
}

void factory_reset_screen_navigate(menu_direction_t direction) {
    (void)direction;
}

void factory_reset_screen_select(void) {
    // Back to menu
    ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
}
