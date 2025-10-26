#include "main_screen.h"
#include "general_config.h"
#include "pc_mode_screen.h"
#include "presenter_main_screen.h"

// Main screen adapts to current device mode
void main_screen_draw(const ui_status_t *status)
{
    // Use global mode variable directly (not cached config)
    extern device_mode_t current_device_mode;

    if (current_device_mode == DEVICE_MODE_PC) {
        // Draw PC mode layout
        extern ui_mini_status_t g_oled_status;
        if (g_oled_status.device_name[0] != '\0') {
            pc_mode_screen_draw(&g_oled_status);
        } else {
            // Not initialized yet, draw empty PC mode
            ui_mini_status_t temp  = {0};
            temp.battery_level  = status->battery_level;
            temp.usb_connected  = status->usb_connected;
            temp.lora_connected = status->lora_connected;
            pc_mode_screen_draw(&temp);
        }
    } else {
        // Draw presenter mode layout
        presenter_main_screen_draw(status);
    }
}
