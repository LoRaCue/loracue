#include "lvgl.h"
#include "ui_compact_statusbar.h"
#include "ui_compact_fonts.h"
#include "ui_components.h"
#include "ui_lvgl_config.h"
#include "esp_log.h"

static const char *TAG = "screen_main";
static lv_obj_t *statusbar = NULL;
static lv_obj_t *mode_label = NULL;
static lv_obj_t *device_label = NULL;
static lv_group_t *group = NULL;

// External declarations for generated button icons
LV_IMG_DECLARE(button_short_press);
LV_IMG_DECLARE(button_double_press);
LV_IMG_DECLARE(button_long_press);

void screen_main_create(lv_obj_t *parent, const statusbar_data_t *initial_status) {
    ESP_LOGI(TAG, "Creating presenter main screen");
    
    // Set black background
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    // Create status bar at top
    statusbar = ui_compact_statusbar_create(parent);
    
    // Use provided status or defaults
    statusbar_data_t status;
    if (initial_status) {
        status = *initial_status;
    } else {
        // Default values for testing
        status = (statusbar_data_t){
            .usb_connected = false,
            .bluetooth_enabled = false,
            .bluetooth_connected = false,
            .signal_strength = SIGNAL_NONE,
            .battery_level = 0,
            .device_name = "LC-????"
        };
    }
    ui_compact_statusbar_update(statusbar, &status);
    
    // Main content: "PRESENTER" centered
    mode_label = lv_label_create(parent);
    lv_label_set_text(mode_label, "PRESENTER");
    lv_obj_set_style_text_color(mode_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(mode_label, &lv_font_pixolletta_20, 0);
    lv_obj_align(mode_label, LV_ALIGN_CENTER, 0, PRESENTER_TEXT_Y - (DISPLAY_HEIGHT / 2));
    
    // Button hints: Double-press icon + "PREV" on left
    lv_obj_t *double_press_img = lv_img_create(parent);
    lv_img_set_src(double_press_img, &button_double_press);
    lv_obj_set_pos(double_press_img, BUTTON_MARGIN, BUTTON_HINTS_Y);
    
    lv_obj_t *prev_label = lv_label_create(parent);
    lv_label_set_text(prev_label, "PREV");
    lv_obj_set_style_text_color(prev_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(prev_label, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(prev_label, BUTTON_MARGIN + button_double_press.header.w + 2, BUTTON_TEXT_Y);
    
    // Button hints: "NEXT" + short-press icon on right
    lv_obj_t *next_label = lv_label_create(parent);
    lv_label_set_text(next_label, "NEXT");
    lv_obj_set_style_text_color(next_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(next_label, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(next_label, DISPLAY_WIDTH - BUTTON_MARGIN - button_short_press.header.w - 24 - 2, BUTTON_TEXT_Y);
    
    lv_obj_t *short_press_img = lv_img_create(parent);
    lv_img_set_src(short_press_img, &button_short_press);
    lv_obj_set_pos(short_press_img, DISPLAY_WIDTH - BUTTON_MARGIN - button_short_press.header.w, BUTTON_HINTS_Y);
    
    // Bottom separator line
    lv_obj_t *bottom_line = lv_line_create(parent);
    static lv_point_precise_t bottom_points[] = {{0, SEPARATOR_Y_BOTTOM}, {DISPLAY_WIDTH, SEPARATOR_Y_BOTTOM}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);
    
    // Bottom bar: Device name on left
    device_label = lv_label_create(parent);
    const char *device_name = (initial_status && initial_status->device_name) ? initial_status->device_name : "LC-????";
    lv_label_set_text(device_label, device_name);
    lv_obj_set_style_text_color(device_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(device_label, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(device_label, TEXT_MARGIN_LEFT, BOTTOM_TEXT_Y);
    ESP_LOGI(TAG, "Device name: %s", device_name);
    
    // Bottom bar: Long-press icon + "Menu" on right
    ui_draw_icon_text(parent, &button_long_press, "Menu", DISPLAY_WIDTH - TEXT_MARGIN_RIGHT, BOTTOM_BAR_Y + 2, UI_ALIGN_RIGHT);
    
    // Create button group for input
    group = lv_group_create();
    lv_group_add_obj(group, mode_label);
    
    ESP_LOGI(TAG, "Presenter main screen created");
}

void screen_main_update_mode(const char *mode) {
    if (mode_label) {
        lv_label_set_text(mode_label, mode);
    }
}

void screen_main_update_device_name(const char *device_name) {
    if (device_label && device_name) {
        lv_label_set_text(device_label, device_name);
    }
}

void screen_main_update_status(const char *status) {
    // Reserved for future status updates
}

lv_group_t *screen_main_get_group(void) {
    return group;
}
