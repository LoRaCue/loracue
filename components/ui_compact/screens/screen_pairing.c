#include "lvgl.h"
#include "ui_compact_fonts.h"
#include "ui_lvgl_config.h"
#include "usb_pairing.h"
#include "esp_log.h"

static const char *TAG = "pairing";

LV_IMG_DECLARE(button_double_press);

static lv_obj_t *status_label = NULL;
static lv_obj_t *msg1 = NULL;
static lv_obj_t *msg2 = NULL;

static void pairing_callback(bool success, uint16_t device_id, const char *device_name) {
    if (status_label) {
        // Hide hint messages and show status centered
        if (msg1) lv_obj_add_flag(msg1, LV_OBJ_FLAG_HIDDEN);
        if (msg2) lv_obj_add_flag(msg2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);
        
        if (success) {
            ESP_LOGI(TAG, "Pairing successful: %s (0x%04X)", device_name, device_id);
            lv_label_set_text_fmt(status_label, "Paired: %s", device_name);
        } else {
            ESP_LOGI(TAG, "Pairing failed");
            lv_label_set_text(status_label, "Pairing failed");
        }
    }
}

void screen_pairing_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "USB PAIRING");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(title, 2, 0);
    
    lv_obj_t *line = lv_line_create(parent);
    static lv_point_precise_t points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(line, points, 2);
    lv_obj_set_style_line_color(line, lv_color_white(), 0);
    lv_obj_set_style_line_width(line, 1, 0);
    
    msg1 = lv_label_create(parent);
    lv_label_set_text(msg1, "Connect other LoRaCue");
    lv_obj_set_style_text_color(msg1, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg1, &lv_font_pixolletta_10, 0);
    lv_obj_align(msg1, LV_ALIGN_CENTER, 0, -5);
    
    msg2 = lv_label_create(parent);
    lv_label_set_text(msg2, "device by USB-C.");
    lv_obj_set_style_text_color(msg2, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg2, &lv_font_pixolletta_10, 0);
    lv_obj_align(msg2, LV_ALIGN_CENTER, 0, 5);
    
    // Status label for pairing feedback (initially hidden)
    status_label = lv_label_create(parent);
    lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_pixolletta_10, 0);
    lv_obj_add_flag(status_label, LV_OBJ_FLAG_HIDDEN);
    
    // Start USB pairing
    esp_err_t ret = usb_pairing_start(pairing_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start USB pairing: %s", esp_err_to_name(ret));
        
        // Hide hints and show error centered
        lv_obj_add_flag(msg1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(msg2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(status_label, LV_ALIGN_CENTER, 0, -5);
        
        // Show detailed error
        lv_label_set_text(status_label, "Error");
        
        lv_obj_t *error_detail = lv_label_create(parent);
        lv_obj_set_style_text_color(error_detail, lv_color_white(), 0);
        lv_obj_set_style_text_font(error_detail, &lv_font_pixolletta_10, 0);
        lv_obj_align(error_detail, LV_ALIGN_CENTER, 0, 5);
        
        if (ret == ESP_ERR_INVALID_STATE) {
            lv_label_set_text(error_detail, "Already active");
        } else if (ret == ESP_ERR_NO_MEM) {
            lv_label_set_text(error_detail, "Out of memory");
        } else {
            lv_label_set_text_fmt(error_detail, "Code: 0x%x", ret);
        }
    } else {
        ESP_LOGI(TAG, "USB pairing started");
    }
    
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

void screen_pairing_reset(void) {
    ESP_LOGW(TAG, "=== Pairing screen reset - stopping USB pairing ===");
    usb_pairing_stop();
    ESP_LOGW(TAG, "=== usb_pairing_stop returned ===");
    status_label = NULL;
    msg1 = NULL;
    msg2 = NULL;
}
