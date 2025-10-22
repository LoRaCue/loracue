#include "pc_mode_screen.h"
#include "bluetooth_config.h"
#include "bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "icons/ui_status_icons.h"
#include "oled_ui.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "ui_icons.h"
#include "ui_pairing_overlay.h"
#include "ui_status_bar.h"
#include <inttypes.h>
#include <string.h>

static const char *TAG = "pc_mode_screen";

extern u8g2_t u8g2;

// Lightbar state: 0 = lines 2&4 (even), 1 = lines 1&3 (odd)
static uint8_t lightbar_state = 0;
static uint32_t last_timestamp = 0;
static bool initialized = false;

// Keycode to display name lookup
static const char* keycode_to_name(uint8_t keycode, uint8_t modifiers)
{
    static char buf[16];
    const char *key_name = NULL;
    
    // Letters (a-z)
    if (keycode >= 0x04 && keycode <= 0x1D) {
        buf[0] = 'a' + (keycode - 0x04);
        buf[1] = '\0';
        key_name = buf;
    }
    // Numbers (1-9, 0)
    else if (keycode >= 0x1E && keycode <= 0x26) {
        buf[0] = '1' + (keycode - 0x1E);
        buf[1] = '\0';
        key_name = buf;
    }
    else if (keycode == 0x27) key_name = "0";
    // Function keys
    else if (keycode >= 0x3A && keycode <= 0x45) {
        snprintf(buf, sizeof(buf), "F%d", keycode - 0x39);
        key_name = buf;
    }
    // Special keys
    else {
        switch (keycode) {
            case 0x28: key_name = "Enter"; break;
            case 0x29: key_name = "Esc"; break;
            case 0x2A: key_name = "BkSp"; break;
            case 0x2B: key_name = "Tab"; break;
            case 0x2C: key_name = "Space"; break;
            case 0x2D: key_name = "-"; break;
            case 0x2E: key_name = "="; break;
            case 0x2F: key_name = "["; break;
            case 0x30: key_name = "]"; break;
            case 0x31: key_name = "\\"; break;
            case 0x33: key_name = ";"; break;
            case 0x34: key_name = "'"; break;
            case 0x35: key_name = "`"; break;
            case 0x36: key_name = ","; break;
            case 0x37: key_name = "."; break;
            case 0x38: key_name = "/"; break;
            case 0x4A: key_name = "Home"; break;
            case 0x4B: key_name = "PgUp"; break;
            case 0x4C: key_name = "Del"; break;
            case 0x4D: key_name = "End"; break;
            case 0x4E: key_name = "PgDn"; break;
            case 0x4F: key_name = "→"; break;
            case 0x50: key_name = "←"; break;
            case 0x51: key_name = "↓"; break;
            case 0x52: key_name = "↑"; break;
            default: key_name = "?"; break;
        }
    }
    
    // Add modifiers
    buf[0] = '\0';
    if (modifiers & 0x01) strcat(buf, "Ctrl+");  // Left Ctrl
    if (modifiers & 0x10) strcat(buf, "Ctrl+");  // Right Ctrl
    if (modifiers & 0x02) strcat(buf, "Shift+"); // Left Shift
    if (modifiers & 0x20) strcat(buf, "Shift+"); // Right Shift
    if (modifiers & 0x04) strcat(buf, "Alt+");   // Left Alt
    if (modifiers & 0x40) strcat(buf, "Alt+");   // Right Alt
    if (modifiers & 0x08) strcat(buf, "Win+");   // Left GUI
    if (modifiers & 0x80) strcat(buf, "Win+");   // Right GUI
    
    if (buf[0] != '\0') {
        strcat(buf, key_name);
        return buf;
    }
    
    return key_name;
}

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

    // Viewport: Command history (last 4 commands) - moved down 1px
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);

    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    int y           = 21; // Moved from 20 to 21

    // Toggle lightbar on every new event
    if (!initialized) {
        lightbar_state = 0; // Start at 2&4
        last_timestamp = 0;
        initialized = true;
    }
    
    // Check if newest entry (index 0) has changed
    if (status->command_history_count > 0) {
        uint32_t newest_timestamp = status->command_history[0].timestamp_ms;
        if (newest_timestamp != last_timestamp) {
            lightbar_state = 1 - lightbar_state; // Toggle
            last_timestamp = newest_timestamp;
        }
    }

    for (int i = 0; i < status->command_history_count && i < 4; i++) {
        uint32_t elapsed_sec = (now_ms - status->command_history[i].timestamp_ms) / 1000;
        
        // Determine if this line should have lightbar (only if text exists)
        bool draw_lightbar = false;
        if (lightbar_state == 0) {
            // Lines 2 & 4 (indices 1 & 3)
            draw_lightbar = (i == 1 || i == 3);
        } else {
            // Lines 1 & 3 (indices 0 & 2)
            draw_lightbar = (i == 0 || i == 2);
        }
        
        // Draw lightbar background
        if (draw_lightbar) {
            u8g2_SetDrawColor(&u8g2, 1);
            u8g2_DrawBox(&u8g2, 0, y - 7, DISPLAY_WIDTH, 9);
            u8g2_SetDrawColor(&u8g2, 0); // Inverted text
        } else {
            u8g2_SetDrawColor(&u8g2, 1); // Normal text
        }

        // Get key name from keycode
        const char *key_display = keycode_to_name(status->command_history[i].keycode, 
                                                   status->command_history[i].modifiers);

        char line[32];
        snprintf(line, sizeof(line), "%04" PRIu32 " %-8s %s", elapsed_sec, 
                 status->command_history[i].device_name, key_display);

        u8g2_DrawStr(&u8g2, 2, y, line);
        u8g2_SetDrawColor(&u8g2, 1); // Reset to normal
        y += 9;
    }

    // If no commands yet
    if (status->command_history_count == 0) {
        u8g2_SetFont(&u8g2, u8g2_font_helvB12_tr);
        u8g2_DrawCenterStr(&u8g2, DISPLAY_WIDTH, 28, "PC MODE"); // Moved down 1px

        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawCenterStr(&u8g2, DISPLAY_WIDTH, 39, "Waiting for"); // Moved down 1px
        u8g2_DrawCenterStr(&u8g2, DISPLAY_WIDTH, 49, "commands..."); // Moved down 1px
    }

    // Bottom separator
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);

    // Bottom bar
    ui_bottom_bar_draw(&ui_status);

    // Draw Bluetooth pairing overlay if active
    uint32_t passkey;
    if (bluetooth_config_get_passkey(&passkey)) {
        ui_pairing_overlay_draw(passkey);
    }

    u8g2_SendBuffer(&u8g2);
}
