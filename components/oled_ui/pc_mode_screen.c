#include "pc_mode_screen.h"
#include "oled_ui.h"
#include "ui_status_bar.h"
#include "ui_config.h"
#include "bsp.h"
#include "u8g2.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "pc_mode_screen";

extern u8g2_t u8g2;

void pc_mode_screen_draw(const oled_status_t* status)
{
    u8g2_ClearBuffer(&u8g2);
    
    // Convert to ui_status_t for status bar
    ui_status_t ui_status = {
        .usb_connected = status->usb_connected,
        .lora_connected = status->lora_connected,
        .battery_level = status->battery_level
    };
    strncpy(ui_status.device_name, status->device_name, sizeof(ui_status.device_name) - 1);
    
    // Draw status bar (LORACUE + icons)
    ui_status_bar_draw(&ui_status);
    
    // Viewport: Command history (last 4 commands)
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
    
    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    int y = 20;
    
    for (int i = 0; i < status->command_history_count && i < 4; i++) {
        uint32_t elapsed_sec = (now_ms - status->command_history[i].timestamp_ms) / 1000;
        
        char line[32];
        snprintf(line, sizeof(line), "%04lu %-8s %s", 
                 elapsed_sec,
                 status->command_history[i].device_name,
                 status->command_history[i].command);
        
        u8g2_DrawStr(&u8g2, 2, y, line);
        y += 9;
    }
    
    // If no commands yet
    if (status->command_history_count == 0) {
        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawStr(&u8g2, 20, 30, "Waiting for");
        u8g2_DrawStr(&u8g2, 20, 42, "commands...");
    }
    
    // Bottom separator
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    
    // Bottom bar (device name + menu hint)
    ui_bottom_bar_draw(&ui_status);
    
    u8g2_SendBuffer(&u8g2);
}
