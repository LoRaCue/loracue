#include "bsp.h"
#include "esp_log.h"
#include "general_config.h"
#include "lv_port_disp.h"
#include "lvgl.h"
#include "screens.h"
#include "ui_components.h"
#include "ui_navigator.h"

static const char *TAG         = "contrast";
static ui_edit_screen_t *screen = NULL;
static uint8_t contrast_value = 128;
static bool preserved_edit_mode = false;

void screen_contrast_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    if (!screen) {
        screen            = ui_edit_screen_create("CONTRAST");
        screen->edit_mode = preserved_edit_mode;
    }

    char value_text[32];
    snprintf(value_text, sizeof(value_text), "%d", contrast_value);
    ui_edit_screen_render(screen, parent, "CONTRAST", value_text, contrast_value, 255);
}

void screen_contrast_init(void)
{
    general_config_t config;
    general_config_get(&config);
    contrast_value = config.display_contrast;
    if (screen) {
        screen->edit_mode = false;
    }
}

void screen_contrast_navigate_down(void)
{
    if (!screen || !screen->edit_mode)
        return;

    if (contrast_value <= 250) {
        contrast_value += 5;
    } else {
        contrast_value = 255;
    }
    display_safe_set_contrast(contrast_value);
}

void screen_contrast_navigate_up(void)
{
    if (!screen || !screen->edit_mode)
        return;

    if (contrast_value >= 5) {
        contrast_value -= 5;
    } else {
        contrast_value = 0;
    }
    display_safe_set_contrast(contrast_value);
}

void screen_contrast_select(void)
{
    if (!screen)
        return;

    if (screen->edit_mode) {
        general_config_t config;
        general_config_get(&config);
        config.display_contrast = contrast_value;
        general_config_set(&config);
        display_safe_set_contrast(contrast_value);
        ESP_LOGI(TAG, "Contrast saved: %d", contrast_value);
        screen->edit_mode = false;
    } else {
        screen->edit_mode = true;
    }
}

bool screen_contrast_is_edit_mode(void)
{
    return screen ? screen->edit_mode : false;
}

static void handle_input(button_event_type_t event)
{
    if (screen_contrast_is_edit_mode()) {
        if (event == BUTTON_EVENT_SHORT) {
            screen_contrast_navigate_down();
            ui_navigator_switch_to(UI_SCREEN_CONTRAST);
        } else if (event == BUTTON_EVENT_DOUBLE) {
            screen_contrast_navigate_up();
            ui_navigator_switch_to(UI_SCREEN_CONTRAST);
        } else if (event == BUTTON_EVENT_LONG) {
            screen_contrast_select();
            // If we exited edit mode, go back to menu
            if (!screen_contrast_is_edit_mode()) {
                ui_navigator_switch_to(UI_SCREEN_MENU);
            } else {
                ui_navigator_switch_to(UI_SCREEN_CONTRAST);
            }
        }
    } else {
        if (event == BUTTON_EVENT_SHORT) {
            ui_navigator_switch_to(UI_SCREEN_MENU);
        } else if (event == BUTTON_EVENT_LONG) {
            screen_contrast_select();
            ui_navigator_switch_to(UI_SCREEN_CONTRAST);
        }
    }
}

static void on_screen_enter(void)
{
    preserved_edit_mode = screen ? screen->edit_mode : false;
}

static void on_screen_exit(void)
{
    if (screen) {
        preserved_edit_mode = screen->edit_mode;
    }
}

static ui_screen_t screen_interface = {
    .create      = screen_contrast_create,
    .handle_input = handle_input,
    .on_enter    = on_screen_enter,
    .on_exit     = on_screen_exit,
};

ui_screen_t *screen_contrast_get_interface(void)
{
    return &screen_interface;
}
