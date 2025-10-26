#include "lora_cr_screen.h"
#include "lora_driver.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "ui_icons.h"

extern u8g2_t u8g2;

static int selected_item = 0;

static const uint8_t cr_values[] = {5, 6, 7, 8};
static const char *cr_labels[]   = {"4/5", "4/6", "4/7", "4/8"};
static const int cr_count        = 4;

void lora_cr_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    // Header
    ui_draw_header("CODING RATE");

    // Get current config
    lora_config_t config;
    lora_get_config(&config);

    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height     = viewport_height / cr_count;

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    for (int i = 0; i < cr_count; i++) {
        int item_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + (item_height / 2) + 3;

        if (i == selected_item) {
            int bar_y      = SEPARATOR_Y_TOP + 2 + (i * item_height) + 1;
            int bar_height = item_height - 2;
            if (i == cr_count - 1)
                bar_height -= 1;

            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }

        if (cr_values[i] == config.coding_rate) {
            int icon_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + (item_height / 2) - (checkmark_height / 2);
            u8g2_DrawXBM(&u8g2, 4, icon_y, checkmark_width, checkmark_height, checkmark_bits);
            u8g2_DrawStr(&u8g2, 16, item_y, cr_labels[i]);
        } else {
            u8g2_DrawStr(&u8g2, 16, item_y, cr_labels[i]);
        }

        if (i == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }

    // Footer with one-button UI icons
    ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);

    u8g2_SendBuffer(&u8g2);
}

void lora_cr_screen_navigate(menu_direction_t direction)
{
    if (direction == MENU_DOWN) {
        selected_item = (selected_item + 1) % cr_count;
    } else if (direction == MENU_UP) {
        selected_item = (selected_item - 1 + cr_count) % cr_count;
    }
}

void lora_cr_screen_select(void)
{
    lora_config_t config;
    lora_get_config(&config);
    config.coding_rate = cr_values[selected_item];
    lora_set_config(&config);
}
