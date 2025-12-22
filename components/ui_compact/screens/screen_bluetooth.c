#include "input_manager.h"
#include "ui_strings.h"
#include "ble.h"
#include "input_manager.h"
#include "esp_log.h"
#include "config_manager.h"
#include "lvgl.h"
#include "screens.h"
#include "ui_components.h"
#include "ui_lvgl_config.h"
#include "ui_navigator.h"

static const char *TAG = "bluetooth";
static ui_menu_t *menu = NULL;

void screen_bluetooth_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    ui_create_header(parent, "BLUETOOTH");
    ui_create_footer(parent);

    general_config_t config;
    config_manager_get_general(&config);

    static char bt_text[32], pair_text[32];
    snprintf(bt_text, sizeof(bt_text), "Bluetooth: %s", config.bluetooth_enabled ? "ON" : "OFF");
    snprintf(pair_text, sizeof(pair_text), "Pairing: %s", config.bluetooth_pairing_enabled ? "ON" : "OFF");

    const char *items[] = {bt_text, pair_text};
    menu                = ui_menu_create(parent, items, 2);
    ui_menu_update(menu, items);
}

void screen_bluetooth_navigate_down(void)
{
    if (!menu)
        return;
    menu->selected_index = (menu->selected_index + 1) % 2;

    general_config_t config;
    config_manager_get_general(&config);

    static char bt_text[32], pair_text[32];
    snprintf(bt_text, sizeof(bt_text), "Bluetooth: %s", config.bluetooth_enabled ? "ON" : "OFF");
    snprintf(pair_text, sizeof(pair_text), "Pairing: %s", config.bluetooth_pairing_enabled ? "ON" : "OFF");

    const char *items[] = {bt_text, pair_text};
    ui_menu_update(menu, items);
}

void screen_bluetooth_navigate_up(void)
{
    if (!menu)
        return;
    menu->selected_index = (menu->selected_index + 1) % 2; // Only 2 items, so up = down

    general_config_t config;
    config_manager_get_general(&config);

    static char bt_text[32], pair_text[32];
    snprintf(bt_text, sizeof(bt_text), "Bluetooth: %s", config.bluetooth_enabled ? "ON" : "OFF");
    snprintf(pair_text, sizeof(pair_text), "Pairing: %s", config.bluetooth_pairing_enabled ? "ON" : "OFF");

    const char *items[] = {bt_text, pair_text};
    ui_menu_update(menu, items);
}

void screen_bluetooth_select(void)
{
    if (!menu)
        return;

    general_config_t config;
    config_manager_get_general(&config);

    if (menu->selected_index == 0) {
        config.bluetooth_enabled = !config.bluetooth_enabled;
        ble_set_enabled(config.bluetooth_enabled);
    } else {
        config.bluetooth_pairing_enabled = !config.bluetooth_pairing_enabled;
    }

    config_manager_set_general(&config);
    ESP_LOGI(TAG, "Bluetooth setting toggled: item %d", menu->selected_index);
}

void screen_bluetooth_reset(void)
{
    if (menu) {
        free(menu);
        menu = NULL;
    }
}

static void handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_SHORT) {
        screen_bluetooth_navigate_down();
    } else if (event == INPUT_EVENT_NEXT_LONG) {
        screen_bluetooth_select();
        ui_navigator_switch_to(UI_SCREEN_BLUETOOTH);
    } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
        case INPUT_EVENT_ENCODER_BUTTON_SHORT:
            ui_navigator_switch_to(UI_SCREEN_MENU);
            break;
        case INPUT_EVENT_ENCODER_CW:
        case INPUT_EVENT_NEXT_SHORT:
            screen_bluetooth_navigate_down();
            break;
        case INPUT_EVENT_ENCODER_CCW:
            screen_bluetooth_navigate_up();
            break;
        case INPUT_EVENT_ENCODER_BUTTON_LONG:
            screen_bluetooth_select();
            ui_navigator_switch_to(UI_SCREEN_BLUETOOTH);
            break;
        default:
            break;
    }
#endif
}

static ui_screen_t bluetooth_screen = {
    .type               = UI_SCREEN_BLUETOOTH,
    .create             = screen_bluetooth_create,
    .destroy            = screen_bluetooth_reset,
    .handle_input_event = handle_input_event,
};

ui_screen_t *screen_bluetooth_get_interface(void)
{
    return &bluetooth_screen;
}
