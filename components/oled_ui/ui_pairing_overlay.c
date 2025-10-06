#include "ui_pairing_overlay.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include <inttypes.h>
#include <stdio.h>

extern u8g2_t u8g2;

void ui_pairing_overlay_draw(uint32_t passkey)
{
    // Overlay dimensions - centered in viewport
    const int box_width = 100;
    const int box_height = 36;
    const int box_x = (DISPLAY_WIDTH - box_width) / 2;
    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int box_y = SEPARATOR_Y_TOP + (viewport_height - box_height) / 2;

    // Draw white background box
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawBox(&u8g2, box_x, box_y, box_width, box_height);

    // Draw double border (outer and inner frames)
    u8g2_DrawFrame(&u8g2, box_x, box_y, box_width, box_height);
    u8g2_DrawFrame(&u8g2, box_x + 1, box_y + 1, box_width - 2, box_height - 2);

    // Draw large Bluetooth icon on left (vertically centered)
    const int icon_x = box_x + 6;
    const int icon_y = box_y + (box_height - bluetooth_pairing_height) / 2;
    u8g2_DrawXBM(&u8g2, icon_x, icon_y, bluetooth_pairing_width, bluetooth_pairing_height, bluetooth_pairing_bits);

    // Draw text on right side (inverted - black on white)
    u8g2_SetDrawColor(&u8g2, 0);
    
    // "Bluetooth Connection" - bold font
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    const char *title = "Bluetooth";
    int text_x = box_x + bluetooth_pairing_width + 12;
    u8g2_DrawStr(&u8g2, text_x, box_y + 10, title);
    
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    const char *subtitle = "Connection";
    u8g2_DrawStr(&u8g2, text_x, box_y + 19, subtitle);

    // "PIN: 123456" - larger font
    u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
    char pin_str[16];
    snprintf(pin_str, sizeof(pin_str), "PIN:%06" PRIu32, passkey);
    u8g2_DrawStr(&u8g2, text_x, box_y + 32, pin_str);

    // Reset draw color
    u8g2_SetDrawColor(&u8g2, 1);
}
