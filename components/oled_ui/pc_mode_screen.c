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
        u8g2_DrawStr(&u8g2, 0, 24, "USB: Connected");
    } else {
        u8g2_DrawStr(&u8g2, 0, 24, "USB: DISCONNECTED!");
    }
    
    // LoRa status with RSSI
    if (status->lora_connected) {
        char buf[32];
        snprintf(buf, sizeof(buf), "LoRa: %d dBm", status->lora_signal);
        u8g2_DrawStr(&u8g2, 0, 36, buf);
    } else {
        u8g2_DrawStr(&u8g2, 0, 36, "LoRa: Waiting...");
    }
    
    // Last command
    char cmd_buf[32];
    if (status->last_command[0] != '\0') {
        snprintf(cmd_buf, sizeof(cmd_buf), "Last: %s", status->last_command);
    } else {
        snprintf(cmd_buf, sizeof(cmd_buf), "Last: --");
    }
    u8g2_DrawStr(&u8g2, 0, 50, cmd_buf);
    
    // Footer
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
    u8g2_DrawStr(&u8g2, 0, 62, "Buttons disabled");
    
    u8g2_SendBuffer(&u8g2);
}
