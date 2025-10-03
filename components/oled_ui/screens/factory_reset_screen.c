#include "factory_reset_screen.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "ui_screen_controller.h"

static const char *TAG = "factory_reset_screen";
extern u8g2_t u8g2;

void factory_reset_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);

    // Header
    u8g2_DrawStr(&u8g2, 2, 10, "FACTORY RESET");
    u8g2_DrawHLine(&u8g2, 0, 12, 128);

    // Centered instruction text with 3px padding around icon
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    const char *text_before = "Press";
    const char *text_after  = "5 sec";
    const char *line2       = "for factory reset!";

    int before_width = u8g2_GetStrWidth(&u8g2, text_before);
    int after_width  = u8g2_GetStrWidth(&u8g2, text_after);
    int line2_width  = u8g2_GetStrWidth(&u8g2, line2);

    // Calculate total width: text + 3px + icon + 3px + text
    int total_width = before_width + 3 + both_buttons_width + 3 + after_width;
    int x_start     = (128 - total_width) / 2;

    u8g2_DrawStr(&u8g2, x_start, 32, text_before);
    u8g2_DrawXBM(&u8g2, x_start + before_width + 3, 24, both_buttons_width, both_buttons_height, both_buttons_bits);
    u8g2_DrawStr(&u8g2, x_start + before_width + 3 + both_buttons_width + 3, 32, text_after);

    int x2 = (128 - line2_width) / 2;
    u8g2_DrawStr(&u8g2, x2, 44, line2);

    // Bottom bar
    u8g2_DrawHLine(&u8g2, 0, 52, 128);
    u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, 8, 64, "Back");

    u8g2_SendBuffer(&u8g2);
}

void factory_reset_screen_navigate(menu_direction_t direction)
{
    (void)direction;
}

void factory_reset_screen_select(void)
{
    // Back to menu
    ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
}

void factory_reset_screen_execute(void)
{
    ESP_LOGW(TAG, "Factory reset: executing!");

    // Erase NVS
    esp_err_t ret = nvs_flash_erase();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS erased successfully");
    } else {
        ESP_LOGE(TAG, "Failed to erase NVS: %s", esp_err_to_name(ret));
    }

    // Restart
    esp_restart();
}
