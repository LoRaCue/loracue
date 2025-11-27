#include "lvgl.h"
#include "ui_compact_fonts.h"
#include "ui_lvgl_config.h"
#include "esp_log.h"

static const char *TAG = "device_registry";

LV_IMG_DECLARE(button_double_press);

void screen_device_registry_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "DEVICE REGISTRY");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(title, 2, 0);
    
    lv_obj_t *line = lv_line_create(parent);
    static lv_point_precise_t points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(line, points, 2);
    lv_obj_set_style_line_color(line, lv_color_white(), 0);
    lv_obj_set_style_line_width(line, 1, 0);
    
    lv_obj_t *msg1 = lv_label_create(parent);
    lv_label_set_text(msg1, "No devices paired");
    lv_obj_set_style_text_color(msg1, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg1, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(msg1, 12, 22);
    
    lv_obj_t *msg2 = lv_label_create(parent);
    lv_label_set_text(msg2, "Use config mode to");
    lv_obj_set_style_text_color(msg2, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg2, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(msg2, 8, 32);
    
    lv_obj_t *msg3 = lv_label_create(parent);
    lv_label_set_text(msg3, "pair new devices");
    lv_obj_set_style_text_color(msg3, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg3, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(msg3, 12, 42);
    
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

void screen_device_registry_reset(void) {
    ESP_LOGI(TAG, "Device registry screen reset");
}
