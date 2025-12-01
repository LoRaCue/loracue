#include "input_manager.h"
#include "esp_log.h"
#include "input_manager.h"
#include "lvgl.h"
#include "screens.h"
#include "ui_compact_fonts.h"
#include "ui_components.h"
#include "ui_lvgl_config.h"
#include "ui_navigator.h"
#include "usb_pairing.h"

static const char *TAG = "pairing";

LV_IMG_DECLARE(button_double_press);

static lv_obj_t *status_label = NULL;
static lv_obj_t *msg1         = NULL;
static lv_obj_t *msg2         = NULL;

static void pairing_callback(bool success, uint16_t device_id, const char *device_name)
{
    if (status_label) {
        // Hide hint messages and show status centered
        if (msg1)
            lv_obj_add_flag(msg1, LV_OBJ_FLAG_HIDDEN);
        if (msg2)
            lv_obj_add_flag(msg2, LV_OBJ_FLAG_HIDDEN);
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

void screen_pairing_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    ui_create_header(parent, "USB PAIRING");

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

    ui_create_footer(parent);
#if CONFIG_INPUT_HAS_DUAL_BUTTONS
    ui_draw_bottom_bar_alpha_plus(parent, &nav_left, "Back", &rotary, "Scroll", &nav_right, "Select");
#else
    ui_draw_icon_text(parent, &button_double_press, "Back", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
#endif
}

void screen_pairing_reset(void)
{
    ESP_LOGW(TAG, "=== usb_pairing_stop returned ===");
    status_label = NULL;
    msg1         = NULL;
    msg2         = NULL;
}

static void handle_input(button_event_type_t event)
{
    if (event == BUTTON_EVENT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
}

static ui_screen_t pairing_screen = {
    .type         = UI_SCREEN_PAIRING,
    .create       = screen_pairing_create,
    .destroy      = screen_pairing_reset,
    .handle_input = handle_input,
};

ui_screen_t *screen_pairing_get_interface(void)
{
    return &pairing_screen;
}
