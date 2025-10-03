#include "icons/ui_usb.h"
#include "u8g2.h"
#include "ui_config.h"

extern u8g2_t u8g2;

void ui_usb_draw(bool connected)
{
    if (!connected)
        return; // Don't draw if not connected

    // USB-C connector with rounded corners (1px gaps for rounding)
    u8g2_DrawHLine(&u8g2, USB_ICON_X + 1, USB_ICON_Y, USB_ICON_WIDTH - 2);     // Top edge
    u8g2_DrawHLine(&u8g2, USB_ICON_X + 1, USB_ICON_Y + 4, USB_ICON_WIDTH - 2); // Bottom edge
    u8g2_DrawVLine(&u8g2, USB_ICON_X, USB_ICON_Y + 1, 3);                      // Left edge
    u8g2_DrawVLine(&u8g2, USB_ICON_X + USB_ICON_WIDTH - 1, USB_ICON_Y + 1, 3); // Right edge

    // Contact strip
    u8g2_DrawHLine(&u8g2, USB_ICON_X + 2, USB_ICON_Y + 2, USB_ICON_WIDTH - 4);
}
