#include "pc_mode_screen.h"
#include "bluetooth_config.h"
#include "bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled_ui.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "ui_icons.h"
#include "ui_status_bar.h"
#include "ui_pairing_overlay.h"
#include "icons/ui_status_icons.h"
#include <inttypes.h>
#include <string.h>

static const char *TAG = "pc_mode_screen";

extern u8g2_t u8g2;

void pc_mode_screen_draw(const oled_status_t *status)
{
    if (!status) {
        ESP_LOGE(TAG, "NULL status pointer");
        return;
    }

    u8g2_ClearBuffer(&u8g2);

    // Convert to ui_status_t for status bar
    ui_status_t ui_status = {.usb_connected  = status->usb_connected,
                             .lora_connected = status->lora_connected,
                             .battery_level  = status->battery_level};
    strncpy(ui_status.device_name, status->device_name, sizeof(ui_status.device_name) - 1);

    // Draw status bar (LORACUE + icons)
    ui_status_bar_draw(&ui_status);

    // Viewport: Command history (last 4 commands)
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);

    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    int y           = 20;

    for (int i = 0; i < status->command_history_count && i < 4; i++) {
        uint32_t elapsed_sec = (now_ms - status->command_history[i].timestamp_ms) / 1000;

        char line[32];
        snprintf(line, sizeof(line), "%04" PRIu32 " %-8s %s", elapsed_sec, status->command_history[i].device_name,
                 status->command_history[i].command);

        u8g2_DrawStr(&u8g2, 2, y, line);
        y += 9;
    }

    // If no commands yet
    if (status->command_history_count == 0) {
        u8g2_SetFont(&u8g2, u8g2_font_helvB12_tr);
        u8g2_DrawCenterStr(&u8g2, DISPLAY_WIDTH, 27, "PC MODE");

        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawCenterStr(&u8g2, DISPLAY_WIDTH, 38, "Waiting for");
        u8g2_DrawCenterStr(&u8g2, DISPLAY_WIDTH, 48, "commands...");
    }

    // Bottom separator
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);

    // Bottom bar
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    // Device name on left
    u8g2_DrawStr(&u8g2, TEXT_MARGIN_LEFT - 1, DISPLAY_HEIGHT - 1, ui_status.device_name);

    // Menu hint on right - using long press icon
    const char *menu_text   = "3s ";
    const char *menu_suffix = " Menu";
    int text_width          = u8g2_GetStrWidth(&u8g2, menu_text);
    int suffix_width        = u8g2_GetStrWidth(&u8g2, menu_suffix);

    int start_x = DISPLAY_WIDTH - TEXT_MARGIN_RIGHT - suffix_width - 7;
    u8g2_DrawStr(&u8g2, start_x - text_width, DISPLAY_HEIGHT - 1, menu_text);
    ui_button_long_draw_at(start_x, DISPLAY_HEIGHT - 8);
    u8g2_DrawStr(&u8g2, start_x + 7, DISPLAY_HEIGHT - 1, menu_suffix);

    // Draw Bluetooth pairing overlay if active
    uint32_t passkey;
    if (bluetooth_config_get_passkey(&passkey)) {
        ui_pairing_overlay_draw(passkey);
    }

    u8g2_SendBuffer(&u8g2);
}
