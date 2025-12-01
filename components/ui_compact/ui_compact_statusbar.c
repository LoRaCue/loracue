#include "ui_compact_statusbar.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_compact_fonts.h"
#include "ui_lvgl_config.h"

// External declarations for generated icon images
LV_IMG_DECLARE(usb_icon);
LV_IMG_DECLARE(bluetooth_icon);
LV_IMG_DECLARE(battery_0_bars);
LV_IMG_DECLARE(battery_1_bars);
LV_IMG_DECLARE(battery_2_bars);
LV_IMG_DECLARE(battery_3_bars);
LV_IMG_DECLARE(battery_4_bars);
LV_IMG_DECLARE(rf_0_bars);
LV_IMG_DECLARE(rf_1_bars);
LV_IMG_DECLARE(rf_2_bars);
LV_IMG_DECLARE(rf_3_bars);
LV_IMG_DECLARE(rf_4_bars);

// Icon objects
static lv_obj_t *usb_img     = NULL;
static lv_obj_t *bt_img      = NULL;
static lv_obj_t *rf_img      = NULL;
static lv_obj_t *battery_img = NULL;

// Battery blink state for critical level
static bool battery_blink_visible   = true;
static uint32_t battery_last_toggle = 0;

lv_obj_t *ui_compact_statusbar_create(lv_obj_t *parent)
{
    // Create container for status bar
    lv_obj_t *statusbar = lv_obj_create(parent);
    lv_obj_set_size(statusbar, DISPLAY_WIDTH, SEPARATOR_Y_TOP);
    lv_obj_set_pos(statusbar, 0, 0);
    lv_obj_set_style_bg_opa(statusbar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(statusbar, 0, 0);
    lv_obj_set_style_pad_all(statusbar, 0, 0);

    // Brand label "LORACUE" at top left
    lv_obj_t *brand = lv_label_create(statusbar);
    lv_label_set_text(brand, "LORACUE");
    lv_obj_set_style_text_color(brand, lv_color_white(), 0);
    lv_obj_set_style_text_font(brand, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(brand, TEXT_MARGIN_LEFT, 0);

    // Create USB icon (hidden by default)
    usb_img = lv_img_create(statusbar);
    lv_img_set_src(usb_img, &usb_icon);
    lv_obj_add_flag(usb_img, LV_OBJ_FLAG_HIDDEN);

    // Create Bluetooth icon (hidden by default)
    bt_img = lv_img_create(statusbar);
    lv_img_set_src(bt_img, &bluetooth_icon);
    lv_obj_add_flag(bt_img, LV_OBJ_FLAG_HIDDEN);

    // Create RF signal icon (fixed position)
    rf_img = lv_img_create(statusbar);
    lv_img_set_src(rf_img, &rf_0_bars);
    lv_obj_set_pos(rf_img, RF_ICON_X, RF_ICON_Y);

    // Create battery icon (fixed position)
    battery_img = lv_img_create(statusbar);
    lv_img_set_src(battery_img, &battery_4_bars);
    lv_obj_set_pos(battery_img, BATTERY_ICON_X, BATTERY_ICON_Y);

    // Top separator line
    lv_obj_t *line                          = lv_line_create(parent);
    static lv_point_precise_t line_points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_color(line, lv_color_white(), 0);
    lv_obj_set_style_line_width(line, 1, 0);

    return statusbar;
}

void ui_compact_statusbar_update(const lv_obj_t *statusbar, const statusbar_data_t *data)
{
    if (!statusbar || !data)
        return;

    // Calculate dynamic icon positions (right to left from RF icon)
    int next_x = RF_ICON_X - ICON_SPACING;

    // Update Bluetooth icon
    if (data->bluetooth_enabled) {
        next_x -= bluetooth_icon.header.w;
        lv_obj_set_pos(bt_img, next_x, 0);
        lv_obj_clear_flag(bt_img, LV_OBJ_FLAG_HIDDEN);
        next_x -= ICON_SPACING;
    } else {
        lv_obj_add_flag(bt_img, LV_OBJ_FLAG_HIDDEN);
    }

    // Update USB icon
    if (data->usb_connected) {
        next_x -= usb_icon.header.w;
        lv_obj_set_pos(usb_img, next_x, 1);
        lv_obj_clear_flag(usb_img, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(usb_img, LV_OBJ_FLAG_HIDDEN);
    }

    // Update RF signal icon
    const void *rf_icon_src;
    switch (data->signal_strength) {
        case SIGNAL_STRONG:
            rf_icon_src = &rf_4_bars;
            break;
        case SIGNAL_GOOD:
            rf_icon_src = &rf_3_bars;
            break;
        case SIGNAL_FAIR:
            rf_icon_src = &rf_2_bars;
            break;
        case SIGNAL_WEAK:
            rf_icon_src = &rf_1_bars;
            break;
        default:
            rf_icon_src = &rf_0_bars;
            break;
    }
    lv_img_set_src(rf_img, rf_icon_src);

    // Update battery icon with blink for critical level
    const void *bat_icon_src;
    bool should_blink = false;

    if (data->battery_level > 75) {
        bat_icon_src = &battery_4_bars;
    } else if (data->battery_level > 50) {
        bat_icon_src = &battery_3_bars;
    } else if (data->battery_level > 25) {
        bat_icon_src = &battery_2_bars;
    } else if (data->battery_level > 5) {
        bat_icon_src = &battery_1_bars;
    } else {
        bat_icon_src = &battery_0_bars;
        should_blink = true;
    }

    lv_img_set_src(battery_img, bat_icon_src);

    // Handle battery blink for critical level (≤5%)
    if (should_blink) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - battery_last_toggle > 500) {
            battery_blink_visible = !battery_blink_visible;
            battery_last_toggle   = now;
        }

        if (battery_blink_visible) {
            lv_obj_clear_flag(battery_img, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(battery_img, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_clear_flag(battery_img, LV_OBJ_FLAG_HIDDEN);
    }
}
