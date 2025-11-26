#include "lvgl.h"
#include "ui_compact_fonts.h"
#include "ui_lvgl_config.h"
#include "general_config.h"
#include "esp_mac.h"
#include "esp_crc.h"
#include "esp_log.h"

static const char *TAG = "config_mode";
static char device_ssid[32] = {0};
static char device_password[16] = {0};

LV_IMG_DECLARE(button_double_press);

static void generate_credentials(void) {
    if (device_ssid[0] != '\0') return;
    
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    uint16_t device_id = general_config_get_device_id();
    snprintf(device_ssid, sizeof(device_ssid), "LoRaCue-%04X", device_id);
    
    uint32_t crc = esp_crc32_le(0, mac, 6);
    const char *charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < 8; i++) {
        device_password[i] = charset[crc % 62];
        crc /= 62;
    }
    device_password[8] = '\0';
    
    ESP_LOGI(TAG, "Generated credentials: %s / %s", device_ssid, device_password);
}

void screen_config_mode_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    generate_credentials();
    
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "CONFIG MODE");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(title, 2, 1);
    
    lv_obj_t *line = lv_line_create(parent);
    static lv_point_precise_t points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(line, points, 2);
    lv_obj_set_style_line_color(line, lv_color_white(), 0);
    lv_obj_set_style_line_width(line, 1, 0);
    
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
    lv_label_set_text(url_label, "192.168.4.1");
    lv_obj_set_style_text_color(url_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(url_label, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(url_label, 20, 42);
    
    lv_obj_t *bottom_line = lv_line_create(parent);
    static lv_point_precise_t bottom_points[] = {{0, SEPARATOR_Y_BOTTOM}, {DISPLAY_WIDTH, SEPARATOR_Y_BOTTOM}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);
    
    lv_obj_t *back_img = lv_img_create(parent);
    lv_img_set_src(back_img, &button_double_press);
    lv_obj_set_pos(back_img, DISPLAY_WIDTH - 20 - button_double_press.header.w - 4, BOTTOM_TEXT_Y + 1);
    
    lv_obj_t *back_label = lv_label_create(parent);
    lv_label_set_text(back_label, "Back");
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(back_label, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(back_label, DISPLAY_WIDTH - 20 - 2, BOTTOM_TEXT_Y);
}

void screen_config_mode_reset(void) {
    device_ssid[0] = '\0';
    device_password[0] = '\0';
    ESP_LOGI(TAG, "Config mode screen reset");
}
