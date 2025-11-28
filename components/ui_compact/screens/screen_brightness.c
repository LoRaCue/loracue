#include "bsp.h"
#include "esp_log.h"
#include "general_config.h"
#include "lvgl.h"
#include "screens.h"
#include "ui_components.h"
#include "ui_navigator.h"

static const char *TAG          = "brightness";
static ui_edit_screen_t *screen = NULL;
static uint8_t brightness_value = 128;
static bool preserved_edit_mode = false;

void screen_brightness_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    if (!screen) {
        screen = ui_edit_screen_create("BRIGHTNESS");
        screen->edit_mode = preserved_edit_mode;
    }

    char value_text[32];
    snprintf(value_text, sizeof(value_text), "%d", brightness_value);
    ui_edit_screen_render(screen, parent, "BRIGHTNESS", value_text, brightness_value, 255);
}

void screen_brightness_init(void)
{
    general_config_t config;
    general_config_get(&config);
    brightness_value = config.display_brightness;
    if (screen) {
        screen->edit_mode = false;
    }
}

void screen_brightness_navigate_down(void)
{
    if (!screen || !screen->edit_mode)
        return;

    if (brightness_value <= 250) {
        brightness_value += 5;
    } else {
        brightness_value = 255;
    }
    bsp_set_display_brightness(brightness_value);
}

void screen_brightness_navigate_up(void)
{
    if (!screen || !screen->edit_mode)
        return;

    if (brightness_value >= 5) {
        brightness_value -= 5;
    } else {
        brightness_value = 0;
    }
    bsp_set_display_brightness(brightness_value);
}

void screen_brightness_select(void)
{
    if (!screen)
        return;

    if (screen->edit_mode) {
        general_config_t config;
        general_config_get(&config);
        config.display_brightness = brightness_value;
        general_config_set(&config);
        bsp_set_display_brightness(brightness_value);
        ESP_LOGI(TAG, "Brightness saved: %d", brightness_value);
        screen->edit_mode = false;
    } else {
        screen->edit_mode = true;
    }
}

bool screen_brightness_is_edit_mode(void)
{
    return screen ? screen->edit_mode : false;
}

static void handle_input(button_event_type_t event)
{
    if (screen_brightness_is_edit_mode()) {
        if (event == BUTTON_EVENT_SHORT) {
            screen_brightness_navigate_down();
            ui_navigator_switch_to(UI_SCREEN_BRIGHTNESS);
        } else if (event == BUTTON_EVENT_DOUBLE) {
            screen_brightness_navigate_up();
            ui_navigator_switch_to(UI_SCREEN_BRIGHTNESS);
        } else if (event == BUTTON_EVENT_LONG) {
            screen_brightness_select();
            // If we exited edit mode, go back to menu
            if (!screen_brightness_is_edit_mode()) {
                ui_navigator_switch_to(UI_SCREEN_MENU);
            } else {
                ui_navigator_switch_to(UI_SCREEN_BRIGHTNESS);
            }
        }
    } else {
        if (event == BUTTON_EVENT_LONG) {
            screen_brightness_select();
            ui_navigator_switch_to(UI_SCREEN_BRIGHTNESS);
        } else if (event == BUTTON_EVENT_DOUBLE) {
            ui_navigator_switch_to(UI_SCREEN_MENU);
        }
    }
}

static void screen_brightness_reset(void)
{
    if (screen) {
        preserved_edit_mode = screen->edit_mode;
        free(screen);
        screen = NULL;
    }
}

static ui_screen_t brightness_screen = {
    .type         = UI_SCREEN_BRIGHTNESS,
    .create       = screen_brightness_create,
    .destroy      = screen_brightness_reset,
    .handle_input = handle_input,
};

ui_screen_t *screen_brightness_get_interface(void)
{
    return &brightness_screen;
}
