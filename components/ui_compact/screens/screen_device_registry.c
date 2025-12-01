#include "input_manager.h"
#include "esp_log.h"
#include "input_manager.h"
#include "lvgl.h"
#include "screens.h"
#include "ui_compact_fonts.h"
#include "ui_components.h"
#include "ui_lvgl_config.h"
#include "ui_navigator.h"

static const char *TAG = "device_registry";

LV_IMG_DECLARE(button_double_press);
LV_IMG_DECLARE(nav_left);
LV_IMG_DECLARE(nav_right);
LV_IMG_DECLARE(rotary);

void screen_device_registry_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    ui_create_header(parent, "DEVICE REGISTRY");

    lv_obj_t *msg1 = lv_label_create(parent);
    lv_label_set_text(msg1, "No devices paired");
    lv_obj_set_style_text_color(msg1, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg1, &lv_font_pixolletta_10, 0);
    lv_obj_align(msg1, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *msg2 = lv_label_create(parent);
    lv_label_set_text(msg2, "Use config mode to");
    lv_obj_set_style_text_color(msg2, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg2, &lv_font_pixolletta_10, 0);
    lv_obj_align(msg2, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *msg3 = lv_label_create(parent);
    lv_label_set_text(msg3, "pair new devices");
    lv_obj_set_style_text_color(msg3, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg3, &lv_font_pixolletta_10, 0);
    lv_obj_align(msg3, LV_ALIGN_CENTER, 0, 10);

    ui_create_footer(parent);
#if CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    ui_draw_bottom_bar_alpha_plus(parent, &nav_left, "Back", &rotary, "Scroll", &nav_right, "Select");
#else
    ui_draw_icon_text(parent, &button_double_press, "Back", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
#endif
}

void screen_device_registry_reset(void)
{
    ESP_LOGI(TAG, "Device registry screen reset");
}

static void handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    if (event == INPUT_EVENT_PREV_SHORT) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#endif
}

static ui_screen_t device_registry_screen = {
    .type               = UI_SCREEN_DEVICE_REGISTRY,
    .create             = screen_device_registry_create,
    .destroy            = screen_device_registry_reset,
    .handle_input_event = handle_input_event,
};

ui_screen_t *screen_device_registry_get_interface(void)
{
    return &device_registry_screen;
}
