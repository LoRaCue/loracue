#include "config_mode_screen.h"
#include "config_wifi_server.h"
#include "device_config.h"
#include "esp_crc.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "oled_ui.h"
#include "ui_config.h"
#include "ui_icons.h"
#include <stdio.h>
#include <string.h>

extern u8g2_t u8g2;

static const char *TAG = "CONFIG_MODE";

// Screen state
static char device_ssid[32]     = {0};
static char device_password[16] = {0};

static void generate_credentials(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // Generate SSID: LoRaCue-XXXX (last 4 hex digits of MAC)
    snprintf(device_ssid, sizeof(device_ssid), "LoRaCue-%02X%02X", mac[4], mac[5]);

    // Generate password from MAC CRC32
    uint32_t crc        = esp_crc32_le(0, mac, 6);
    const char *charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < 8; i++) {
        device_password[i] = charset[crc % 62];
        crc /= 62;
    }
    device_password[8] = '\0';

    ESP_LOGI(TAG, "Generated credentials: %s / %s", device_ssid, device_password);
}

void config_mode_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    // Generate credentials if not done yet
    if (device_ssid[0] == '\0') {
        generate_credentials();
    }

    // Start WiFi server if not running
    if (!config_wifi_server_is_running()) {
        config_wifi_server_start();
    }

    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "CONFIGURATION MODE");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);

    // WiFi credentials
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 23, "SSID:");
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    u8g2_DrawStr(&u8g2, 35, 23, device_ssid);

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 35, "Password:");
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    u8g2_DrawStr(&u8g2, 55, 35, device_password);

    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    u8g2_DrawStr(&u8g2, 2, 47, "http://192.168.4.1");

    // Bottom bar
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, 8, DISPLAY_HEIGHT - 1, "Back");

    u8g2_SendBuffer(&u8g2);
}

void config_mode_screen_reset(void)
{
    // Reset credentials to force regeneration
    device_ssid[0]     = '\0';
    device_password[0] = '\0';

    // Stop WiFi server when leaving config mode
    config_wifi_server_stop();
}

void config_mode_screen_toggle_display(void)
{
    // No QR code mode anymore, this is a no-op
}
