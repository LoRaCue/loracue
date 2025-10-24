#include "lora_bw_screen.h"
#include "lora_driver.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "ui_icons.h"

extern u8g2_t u8g2;

static int selected_item = 0;

static const uint16_t bw_values[] = {125, 250, 500};
static const int bw_count         = 3;

void lora_bw_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    // Header
    ui_draw_header("BANDWIDTH");

    // Get current config
    lora_config_t config;
    lora_get_config(&config);

    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height     = viewport_height / bw_count;

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    for (int i = 0; i < bw_count; i++) {
        int item_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + (item_height / 2) + 3;

        if (i == selected_item) {
            int bar_y      = SEPARATOR_Y_TOP + 2 + (i * item_height) + 1;
            int bar_height = item_height - 2;
            if (i == bw_count - 1)
                bar_height -= 1;

            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }

        char bw_str[16];
        snprintf(bw_str, sizeof(bw_str), "%d kHz", bw_values[i]);

        if (bw_values[i] == config.bandwidth) {
            int icon_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + (item_height / 2) - (checkmark_height / 2);
            u8g2_DrawXBM(&u8g2, 4, icon_y, checkmark_width, checkmark_height, checkmark_bits);
            u8g2_DrawStr(&u8g2, 16, item_y, bw_str);
        } else {
            u8g2_DrawStr(&u8g2, 16, item_y, bw_str);
        }

        if (i == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }

    // Footer with one-button UI icons
    ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);

    u8g2_SendBuffer(&u8g2);
}

void lora_bw_screen_navigate(menu_direction_t direction)
{
    if (direction == MENU_DOWN) {
        selected_item = (selected_item + 1) % bw_count;
    } else if (direction == MENU_UP) {
        selected_item = (selected_item - 1 + bw_count) % bw_count;
    }
}

void lora_bw_screen_select(void)
{
    lora_config_t config;
    lora_get_config(&config);
    config.bandwidth = bw_values[selected_item];
    lora_set_config(&config);
}
