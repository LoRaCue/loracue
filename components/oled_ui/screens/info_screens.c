#include "info_screens.h"
#include "ui_config.h"
#include "version.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "u8g2.h"

extern u8g2_t u8g2;

static void draw_info_header(const char* title) {
    u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
    u8g2_DrawStr(&u8g2, 2, 12, title);
    u8g2_DrawHLine(&u8g2, 0, 15, DISPLAY_WIDTH);
}

void system_info_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    draw_info_header("SYSTEM INFO");
    
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    // Firmware version
    u8g2_DrawStr(&u8g2, 2, 26, "Firmware: ");
    u8g2_DrawStr(&u8g2, 55, 26, LORACUE_VERSION_STRING);
    
    // Hardware
    u8g2_DrawStr(&u8g2, 2, 36, "Hardware: Heltec LoRa V3");
    
    // ESP-IDF version
    u8g2_DrawStr(&u8g2, 2, 46, "ESP-IDF: ");
    u8g2_DrawStr(&u8g2, 50, 46, IDF_VER);
    
    // Free heap memory - simple conversion
    uint32_t heap_kb = esp_get_free_heap_size() / 1024;
    char heap_str[20] = "Free RAM: ";
    char *p = heap_str + 10;
    
    if (heap_kb >= 100) {
        *p++ = '0' + (heap_kb / 100);
        heap_kb %= 100;
    }
    if (heap_kb >= 10 || p > heap_str + 10) {
        *p++ = '0' + (heap_kb / 10);
        heap_kb %= 10;
    }
    *p++ = '0' + heap_kb;
    *p++ = 'K';
    *p++ = 'B';
    *p = '\0';
    
    u8g2_DrawStr(&u8g2, 2, 56, heap_str);
    
    // Navigation hint
    u8g2_DrawStr(&u8g2, 2, 62, "[<] Back");
    
    u8g2_SendBuffer(&u8g2);
}

void device_info_screen_draw(const ui_status_t* status) {
    u8g2_ClearBuffer(&u8g2);
    
    draw_info_header("DEVICE INFO");
    
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    // Device name
    u8g2_DrawStr(&u8g2, 2, 26, "Device: ");
    u8g2_DrawStr(&u8g2, 45, 26, status->device_name);
    
    // Mode
    u8g2_DrawStr(&u8g2, 2, 36, "Mode: STAGE Remote");
    
    // LoRa channel
    u8g2_DrawStr(&u8g2, 2, 46, "LoRa: 868.1 MHz");
    
    // Device ID (from MAC)
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char device_id[16] = "ID: ";
    device_id[4] = "0123456789ABCDEF"[mac[4] >> 4];
    device_id[5] = "0123456789ABCDEF"[mac[4] & 0xF];
    device_id[6] = "0123456789ABCDEF"[mac[5] >> 4];
    device_id[7] = "0123456789ABCDEF"[mac[5] & 0xF];
    device_id[8] = '\0';
    u8g2_DrawStr(&u8g2, 2, 56, device_id);
    
    // Navigation hint
    u8g2_DrawStr(&u8g2, 2, 62, "[<] Back");
    
    u8g2_SendBuffer(&u8g2);
}
