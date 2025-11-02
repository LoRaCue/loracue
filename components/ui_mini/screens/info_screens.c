#include "info_screens.h"
#include "device_mode_screen.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "lora_driver.h"
#include "lora_protocol.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_data_provider.h"
#include "ui_helpers.h"
#include "ui_icons.h"
#include "version.h"

extern u8g2_t u8g2;

static void draw_info_header(const char *title)
{
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);                // Same as main screen
    u8g2_DrawStr(&u8g2, 2, 8, title);                         // Move down to y=8 to avoid clipping
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH); // Use config constant
}

static void draw_info_footer(void)
{
    ui_draw_footer(FOOTER_CONTEXT_INFO, NULL);
}

void system_info_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    draw_info_header("SYSTEM INFO");

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    // Firmware version
    u8g2_DrawStr(&u8g2, 2, 20, "Firmware: ");
    u8g2_DrawStr(&u8g2, 55, 20, LORACUE_VERSION_FULL);

    // Hardware
    // FIXME: Make dynamic for other hardware versions
    u8g2_DrawStr(&u8g2, 2, 30, "Hardware: Heltec LoRa V3");

    // ESP-IDF version
    u8g2_DrawStr(&u8g2, 2, 40, "ESP-IDF: ");
    u8g2_DrawStr(&u8g2, 50, 40, IDF_VER);

    // Free heap memory - simple conversion
    uint32_t heap_kb  = esp_get_free_heap_size() / 1024;
    char heap_str[20] = "Free RAM: ";
    char *p           = heap_str + 10;

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
    *p   = '\0';

    u8g2_DrawStr(&u8g2, 2, 50, heap_str);

    draw_info_footer();

    u8g2_SendBuffer(&u8g2);
}

void device_info_screen_draw(const ui_status_t *status)
{
    u8g2_ClearBuffer(&u8g2);

    draw_info_header("DEVICE INFO");

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    // Device name
    u8g2_DrawStr(&u8g2, 2, 20, "Device: ");
    u8g2_DrawStr(&u8g2, 45, 20, status->device_name);

    // Mode (dynamic based on current device mode)
    device_mode_t current_mode = device_mode_get_current();
    if (current_mode == DEVICE_MODE_PRESENTER) {
        u8g2_DrawStr(&u8g2, 2, 30, "Mode: PRESENTER");
    } else {
        u8g2_DrawStr(&u8g2, 2, 30, "Mode: PC");
    }

    uint32_t freq_hz      = lora_get_frequency();
    uint32_t freq_mhz     = freq_hz / 1000000;
    uint32_t freq_decimal = (freq_hz % 1000000) / 100000;

    char freq_str[20] = "LoRa: ";
    char *p           = freq_str + 6;

    // Convert MHz part
    if (freq_mhz >= 1000) {
        *p++ = '0' + (freq_mhz / 1000);
        freq_mhz %= 1000;
    }
    if (freq_mhz >= 100) {
        *p++ = '0' + (freq_mhz / 100);
        freq_mhz %= 100;
    }
    *p++ = '0' + (freq_mhz / 10);
    *p++ = '0' + (freq_mhz % 10);

    // Add decimal if needed
    if (freq_decimal > 0) {
        *p++ = '.';
        *p++ = '0' + freq_decimal;
    }

    *p++ = ' ';
    *p++ = 'M';
    *p++ = 'H';
    *p++ = 'z';
    *p   = '\0';

    u8g2_DrawStr(&u8g2, 2, 40, freq_str);

    // Device ID (from MAC)
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char device_id[16] = "ID: ";
    device_id[4]       = "0123456789ABCDEF"[mac[4] >> 4];
    device_id[5]       = "0123456789ABCDEF"[mac[4] & 0xF];
    device_id[6]       = "0123456789ABCDEF"[mac[5] >> 4];
    device_id[7]       = "0123456789ABCDEF"[mac[5] & 0xF];
    device_id[8]       = '\0';
    u8g2_DrawStr(&u8g2, 2, 50, device_id);

    draw_info_footer();

    u8g2_SendBuffer(&u8g2);
}

void battery_status_screen_draw(const ui_status_t *status)
{
    u8g2_ClearBuffer(&u8g2);

    draw_info_header("BATTERY STATUS");

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    // Get detailed battery info
    battery_info_t battery_info;
    esp_err_t ret = ui_data_provider_get_battery_info(&battery_info);

    if (ret == ESP_OK) {
        // Battery level with percentage
        char level_str[20] = "Level: ";
        char *p            = level_str + 7;
        uint8_t level      = battery_info.percentage;
        if (level >= 100) {
            *p++ = '1';
            *p++ = '0';
            *p++ = '0';
        } else if (level >= 10) {
            *p++ = '0' + (level / 10);
            *p++ = '0' + (level % 10);
        } else {
            *p++ = '0' + level;
        }
        *p++ = '%';
        *p   = '\0';
        u8g2_DrawStr(&u8g2, 2, 20, level_str);

        // Real battery voltage
        char voltage_str[20] = "Voltage: ";
        p                    = voltage_str + 9;
        uint16_t voltage_mv  = (uint16_t)(battery_info.voltage * 1000);
        *p++                 = '0' + (voltage_mv / 1000);
        *p++                 = '.';
        *p++                 = '0' + ((voltage_mv % 1000) / 100);
        *p++                 = 'V';
        *p                   = '\0';
        u8g2_DrawStr(&u8g2, 2, 30, voltage_str);

        // Charging/USB status
        if (battery_info.usb_connected) {
            u8g2_DrawStr(&u8g2, 2, 40, battery_info.charging ? "Status: Charging" : "Status: USB Power");
        } else {
            u8g2_DrawStr(&u8g2, 2, 40, "Status: Battery");
        }

        // Health based on voltage
        const char *health = "Good";
        if (battery_info.voltage < 3.2f) {
            health = "Critical";
        } else if (battery_info.voltage < 3.5f) {
            health = "Low";
        }
        char health_str[20] = "Health: ";
        char *h             = health_str + 8;
        while (*health)
            *h++ = *health++;
        *h = '\0';
        u8g2_DrawStr(&u8g2, 2, 50, health_str);
    } else {
        // Fallback to basic status
        u8g2_DrawStr(&u8g2, 2, 20, "Level: --");
        u8g2_DrawStr(&u8g2, 2, 30, "Voltage: --");
        u8g2_DrawStr(&u8g2, 2, 40, "Status: Unknown");
        u8g2_DrawStr(&u8g2, 2, 50, "Health: --");
    }

    draw_info_footer();

    u8g2_SendBuffer(&u8g2);
}

void lora_stats_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    draw_info_header("LORA STATS");

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    // Get connection statistics
    lora_connection_stats_t stats;
    lora_protocol_get_stats(&stats);

    // Packets sent/received
    char line1[32];
    snprintf(line1, sizeof(line1), "TX: %lu  RX: %lu", stats.packets_sent, stats.packets_received);
    u8g2_DrawStr(&u8g2, 2, 20, line1);

    // ACKs and retransmissions
    char line2[32];
    snprintf(line2, sizeof(line2), "ACK: %lu  Retry: %lu", stats.acks_received, stats.retransmissions);
    u8g2_DrawStr(&u8g2, 2, 30, line2);

    // Packet loss rate
    char line3[32];
    snprintf(line3, sizeof(line3), "Loss: %.1f%%", stats.packet_loss_rate);
    u8g2_DrawStr(&u8g2, 2, 40, line3);

    // Current RSSI
    int16_t rssi = lora_protocol_get_last_rssi();
    char line4[32];
    snprintf(line4, sizeof(line4), "RSSI: %d dBm", rssi);
    u8g2_DrawStr(&u8g2, 2, 50, line4);

    draw_info_footer();

    u8g2_SendBuffer(&u8g2);
}
