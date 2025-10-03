#include "pc_mode_screen.h"
#include "ui_config.h"
#include "bsp.h"
#include "u8g2.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "pc_mode_screen";

extern u8g2_t u8g2;

void pc_mode_screen_draw(const ui_status_t* status)
{
    u8g2_ClearBuffer(&u8g2);
    
    // Title
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    u8g2_DrawStr(&u8g2, 0, 10, "PC MODE - RECEIVER");
    u8g2_DrawHLine(&u8g2, 0, 12, 128);
    
    // USB status
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    if (status->usb_connected) {
        u8g2_DrawStr(&u8g2, 0, 24, "USB: OK");
    } else {
        u8g2_DrawStr(&u8g2, 0, 24, "USB: DISC!");
    }
    
    // Active presenters count
    char buf[32];
    snprintf(buf, sizeof(buf), "Presenters: %d", status->active_presenter_count);
    u8g2_DrawStr(&u8g2, 0, 36, buf);
    
    // Last command with RSSI
    if (status->last_command[0] != '\0') {
        snprintf(buf, sizeof(buf), "%s (%ddBm)", status->last_command, status->lora_signal);
    } else {
        snprintf(buf, sizeof(buf), "Waiting...");
    }
    u8g2_DrawStr(&u8g2, 0, 48, buf);
    
    // Footer
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
    u8g2_DrawStr(&u8g2, 0, 62, "Buttons disabled");
    
    u8g2_SendBuffer(&u8g2);
}
