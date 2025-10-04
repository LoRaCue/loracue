#include "presenter_main_screen.h"
#include "bluetooth_config.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "ui_status_bar.h"
#include "ui_pairing_overlay.h"

extern u8g2_t u8g2;

void presenter_main_screen_draw(const ui_status_t *status)
{
    u8g2_ClearBuffer(&u8g2);

    // Draw status bars
    ui_status_bar_draw(status);

    // Main content area - "PRESENTER"
    u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
    int title_width = u8g2_GetStrWidth(&u8g2, "PRESENTER");
    u8g2_DrawStr(&u8g2, (DISPLAY_WIDTH - title_width) / 2, 30, "PRESENTER");

    // Button hints with compact arrows
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawXBM(&u8g2, BUTTON_MARGIN, 35, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, BUTTON_MARGIN + arrow_prev_width + 2, 43, "PREV");

    int next_x = DISPLAY_WIDTH - BUTTON_MARGIN - track_next_width - u8g2_GetStrWidth(&u8g2, "NEXT") - 2;
    u8g2_DrawStr(&u8g2, next_x, 43, "NEXT");
    u8g2_DrawXBM(&u8g2, next_x + u8g2_GetStrWidth(&u8g2, "NEXT") + 2, 35, track_next_width, track_next_height,
                 track_next_bits);

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
