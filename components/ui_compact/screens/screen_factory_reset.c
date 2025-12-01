#include "input_manager.h"
#include "esp_log.h"
#include "input_manager.h"
#include "esp_system.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "screens.h"
#include "ui_components.h"
#include "ui_navigator.h"

static const char *TAG            = "factory_reset";
static ui_confirmation_t *confirm = NULL;

void screen_factory_reset_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    if (!confirm) {
        confirm = ui_confirmation_create();
    }

    ui_confirmation_render(confirm, parent, "FACTORY RESET", "Erase all data?");
}

void screen_factory_reset_check_hold(bool button_pressed)
{
    if (confirm && ui_confirmation_check_hold(confirm, button_pressed, 5000)) {
        ESP_LOGI(TAG, "Factory reset confirmed - erasing NVS");
        nvs_flash_erase();
        esp_restart();
    }
}

void screen_factory_reset_reset(void)
{
    if (confirm) {
        free(confirm);
        confirm = NULL;
    }
}

static void handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    } else if (event == INPUT_EVENT_NEXT_LONG) {
        screen_factory_reset_check_hold(true);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
            ui_navigator_switch_to(UI_SCREEN_MENU);
            break;
        case INPUT_EVENT_NEXT_SHORT:
            screen_factory_reset_check_hold(true);
            break;
        default:
            break;
    }
#endif
}

static ui_screen_t factory_reset_screen = {
    .type               = UI_SCREEN_FACTORY_RESET,
    .create             = screen_factory_reset_create,
    .destroy            = screen_factory_reset_reset,
    .handle_input_event = handle_input_event,
};

ui_screen_t *screen_factory_reset_get_interface(void)
{
    return &factory_reset_screen;
}
