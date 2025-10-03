#include "icons/ui_bluetooth.h"
#include "u8g2.h"
#include "ui_config.h"

extern u8g2_t u8g2;

#define bluetooth_width  5
#define bluetooth_height 8
static unsigned char bluetooth_bits[] = {
    0xe4, 0xec, 0xf5, 0xee, 0xee, 0xf5, 0xec, 0xe4};

void ui_bluetooth_draw_at(int x, int y, bool connected)
{
    u8g2_DrawXBM(&u8g2, x, y, bluetooth_width, bluetooth_height, bluetooth_bits);
}
