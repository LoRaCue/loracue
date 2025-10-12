#include "brightness_screen.h"
#include "device_config.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "ui_icons.h"

extern u8g2_t u8g2;

static uint8_t brightness_value = 128;
static bool edit_mode           = false;

void brightness_screen_init(void)
{
    device_config_t config;
    device_config_get(&config);
    brightness_value = config.display_brightness;
    edit_mode        = false;
}

void brightness_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "BRIGHTNESS");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);

    // Brightness value
    char value_str[8];
    snprintf(value_str, sizeof(value_str), "%d", brightness_value);
    u8g2_DrawCenterStr(&u8g2, DISPLAY_WIDTH, 28, value_str);

    // Progress bar (128px width, centered)
    int bar_width  = 128;
    int bar_x      = (DISPLAY_WIDTH - bar_width) / 2;
    int bar_y      = 35;
    int bar_height = 10;
    int fill_width = (brightness_value * bar_width) / 255;

    u8g2_DrawFrame(&u8g2, bar_x, bar_y, bar_width, bar_height);
    if (fill_width > 0) {
        u8g2_DrawBox(&u8g2, bar_x + 1, bar_y + 1, fill_width - 2, bar_height - 2);
    }

    // Footer
    if (edit_mode) {
        ui_draw_footer(FOOTER_CONTEXT_EDIT, NULL);
    } else {
        const char *labels[] = {NULL, "Back", "Edit"};
        ui_draw_footer(FOOTER_CONTEXT_CUSTOM, labels);
    }

    u8g2_SendBuffer(&u8g2);
}

void brightness_screen_navigate(menu_direction_t direction)
{
    if (!edit_mode)
        return;

    if (direction == MENU_DOWN) {
        if (brightness_value <= 250)
            brightness_value += 5;
        else
            brightness_value = 255;
    } else if (direction == MENU_UP) {
        if (brightness_value >= 5)
            brightness_value -= 5;
        else
            brightness_value = 0;
    }

    u8g2_SetContrast(&u8g2, brightness_value);
}

void brightness_screen_select(void)
{
    if (edit_mode) {
        // Save
        device_config_t config;
        device_config_get(&config);
        config.display_brightness = brightness_value;
        device_config_set(&config);
        u8g2_SetContrast(&u8g2, brightness_value);
        edit_mode = false;
    } else {
        // Enter edit mode
        edit_mode = true;
    }
}

bool brightness_screen_is_edit_mode(void)
{
    return edit_mode;
}
