#include "lora_txpower_screen.h"
#include "lora_driver.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_helpers.h"

extern u8g2_t u8g2;

static int selected_power = 13; // Default 13 dBm
static bool edit_mode     = false;

// TX power range: 5 to 20 dBm
#define MIN_TX_POWER 5
#define MAX_TX_POWER 20

void lora_txpower_screen_init(void)
{
    lora_config_t config;
    lora_get_config(&config);
    selected_power = config.tx_power;
    edit_mode      = false;
}

void lora_txpower_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    // Header
    ui_draw_header("TX POWER");

    // Display TX power value centered
    u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
    char power_str[16];
    snprintf(power_str, sizeof(power_str), "%d dBm", selected_power);
    int text_width = u8g2_GetStrWidth(&u8g2, power_str);
    int text_x     = (DISPLAY_WIDTH - text_width) / 2;
    int text_y     = (SEPARATOR_Y_TOP + SEPARATOR_Y_BOTTOM) / 2 + 5;
    u8g2_DrawStr(&u8g2, text_x, text_y, power_str);

    // Footer
    if (edit_mode) {
        ui_draw_footer(FOOTER_CONTEXT_VALUE, NULL);
    } else {
        const char *labels[] = {NULL, "Back", "Edit"};
        ui_draw_footer(FOOTER_CONTEXT_CUSTOM, labels);
    }

    u8g2_SendBuffer(&u8g2);
}

void lora_txpower_screen_navigate(menu_direction_t direction)
{
    if (!edit_mode)
        return;

    if (direction == MENU_DOWN) {
        if (selected_power < MAX_TX_POWER)
            selected_power++;
    } else if (direction == MENU_UP) {
        if (selected_power > MIN_TX_POWER)
            selected_power--;
    }
}

void lora_txpower_screen_select(void)
{
    if (edit_mode) {
        // Save
        lora_config_t config;
        lora_get_config(&config);
        config.tx_power = selected_power;
        lora_set_config(&config);
        edit_mode = false;
    } else {
        // Enter edit mode
        edit_mode = true;
    }
}

bool lora_txpower_screen_is_edit_mode(void)
{
    return edit_mode;
}
