#include "presenter_main_screen.h"
#include "bluetooth_config.h"
#include "icons/ui_status_icons.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "ui_icons.h"
#include "ui_pairing_overlay.h"
#include "ui_status_bar.h"

extern u8g2_t u8g2;

void presenter_main_screen_draw(const ui_status_t *status)
{
    u8g2_ClearBuffer(&u8g2);

    // Draw status bars
    ui_status_bar_draw(status);

    // Main content area - "PRESENTER"
    u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
    u8g2_DrawCenterStr(&u8g2, DISPLAY_WIDTH, 30, "PRESENTER");

    // Button hints with one-button UI icons
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    ui_button_double_draw_at(BUTTON_MARGIN, 36);
    u8g2_DrawStr(&u8g2, BUTTON_MARGIN + 15, 43, "PREV");

    int next_text_width = u8g2_GetStrWidth(&u8g2, "NEXT");
    int next_x          = DISPLAY_WIDTH - BUTTON_MARGIN - 7 - next_text_width - 2;
    u8g2_DrawStr(&u8g2, next_x, 43, "NEXT");
    ui_button_short_draw_at(next_x + next_text_width + 2, 36);

    // Bottom separator
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);

    // Bottom bar
    ui_bottom_bar_draw(status);

    // Draw Bluetooth pairing overlay if active
    uint32_t passkey;
    if (bluetooth_config_get_passkey(&passkey)) {
        ui_pairing_overlay_draw(passkey);
    }

    u8g2_SendBuffer(&u8g2);
}
