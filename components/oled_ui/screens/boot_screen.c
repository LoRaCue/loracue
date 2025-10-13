#include "boot_screen.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "version.h"

extern u8g2_t u8g2;

void boot_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    // Draw boot logo centered at Y=0
    int logo_x = (DISPLAY_WIDTH - boot_logo_width) / 2;
    u8g2_DrawXBM(&u8g2, logo_x, 0, boot_logo_width, boot_logo_height, boot_logo_bits);

    // Version centered
    u8g2_SetFont(&u8g2, u8g2_font_profont10_tr);
    u8g2_DrawStr(&u8g2, (128 - u8g2_GetStrWidth(&u8g2, LORACUE_VERSION_FULL)) / 2, 63, LORACUE_VERSION_FULL);
    u8g2_SendBuffer(&u8g2);
}
