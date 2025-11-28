#include "esp_crc.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "general_config.h"
#include "lvgl.h"
#include "screens.h"
#include "ui_compact_fonts.h"
#include "ui_components.h"
#include "ui_lvgl_config.h"
#include "ui_navigator.h"

static const char *TAG          = "config_mode";
static char device_ssid[32]     = {0};
static char device_password[16] = {0};

LV_IMG_DECLARE(button_double_press);

static void generate_credentials(void)
{
    if (device_ssid[0] != '\0')
        return;

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    uint16_t device_id = general_config_get_device_id();
    snprintf(device_ssid, sizeof(device_ssid), "LoRaCue-%04X", device_id);

    uint32_t crc        = esp_crc32_le(0, mac, 6);
    const char *charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < 8; i++) {
        device_password[i] = charset[crc % 62];
        crc /= 62;
    }
    device_password[8] = '\0';

    ESP_LOGI(TAG, "Generated credentials: %s / %s", device_ssid, device_password);
}

void screen_config_mode_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    generate_credentials();

    ui_create_header(parent, "CONFIG MODE");

    lv_obj_t *ssid_label = lv_label_create(parent);
    lv_label_set_text_fmt(ssid_label, "SSID: %s", device_ssid);
    lv_obj_set_style_text_color(ssid_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(ssid_label, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(ssid_label, 2, 18);

    lv_obj_t *pass_label = lv_label_create(parent);
    lv_label_set_text_fmt(pass_label, "Pass: %s", device_password);
    lv_obj_set_style_text_color(pass_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(pass_label, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(pass_label, 2, 28);

    lv_obj_t *url_label = lv_label_create(parent);
    lv_label_set_text(url_label, "http://192.168.4.1");
    lv_obj_set_style_text_color(url_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(url_label, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(url_label, 0, 38);

    ui_create_footer(parent);
    ui_draw_icon_text(parent, &button_double_press, "Back", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
}

void screen_config_mode_reset(void)
{
    device_ssid[0]     = '\0';
    device_password[0] = '\0';
    ESP_LOGI(TAG, "Config mode screen reset");
}

static void handle_input(button_event_type_t event)
{
    if (event == BUTTON_EVENT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
}

static ui_screen_t config_mode_screen = {
    .type         = UI_SCREEN_CONFIG_MODE,
    .create       = screen_config_mode_create,
    .destroy      = screen_config_mode_reset,
    .handle_input = handle_input,
};

ui_screen_t *screen_config_mode_get_interface(void)
{
    return &config_mode_screen;
}
