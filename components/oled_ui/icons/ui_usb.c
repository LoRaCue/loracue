#include "icons/ui_usb.h"
#include "u8g2.h"
#include "ui_config.h"

extern u8g2_t u8g2;

// USB XBM bitmap (14x7)
static const unsigned char usb_icon[] = {
    0xfe, 0xdf, 0x01, 0xe0, 0x01, 0xe0, 0xfd, 0xef, 0x01, 0xe0, 0x01, 0xe0, 0xfe, 0xdf};

void ui_usb_draw_at(int x, int y)
{
    u8g2_DrawXBM(&u8g2, x, y, 14, 7, usb_icon);
}
