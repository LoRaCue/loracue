#include "input_manager.h"
#include "ui_strings.h"
#include "esp_log.h"
#include "input_manager.h"
#include "general_config.h"
#include "lvgl.h"
#include "screens.h"
#include "system_events.h"
#include "ui_components.h"
#include "ui_navigator.h"

static const char *TAG          = "device_mode";
static ui_radio_select_t *radio = NULL;
static int preserved_index      = -1; // Preserve selection across screen recreations

static const char *mode_items[] = {UI_STR_PRESENTER, "PC"};
#define MODE_COUNT 2

void screen_device_mode_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    if (!radio) {
        radio = ui_radio_select_create(MODE_COUNT, UI_RADIO_SINGLE);

        // Restore preserved index if available, otherwise use config
        if (preserved_index >= 0) {
            radio->selected_index = preserved_index;
        } else {
            general_config_t config;
            general_config_get(&config);
            int current_mode_idx  = (config.device_mode == DEVICE_MODE_PRESENTER) ? 0 : 1;
            radio->selected_index = current_mode_idx;
        }

        // Set the saved/committed value (filled radio)
        if (radio->selected_items) {
            general_config_t config;
            general_config_get(&config);
            int current_mode_idx              = (config.device_mode == DEVICE_MODE_PRESENTER) ? 0 : 1;
            ((int *)radio->selected_items)[0] = current_mode_idx;
        }
    }

    ESP_LOGI(TAG, "Creating device mode screen: selected_index=%d", radio->selected_index);
    ui_radio_select_render(radio, parent, "DEVICE MODE", mode_items);
}

void screen_device_mode_navigate_down(void)
{
    ui_radio_select_navigate_down(radio);
    ESP_LOGI(TAG, "Navigate down: selected_index=%d", radio->selected_index);
}

void screen_device_mode_navigate_up(void)
{
    ui_radio_select_navigate_up(radio);
    ESP_LOGI(TAG, "Navigate up: selected_index=%d", radio->selected_index);
}

void screen_device_mode_select(void)
{
    if (!radio)
        return;

    general_config_t config;
    general_config_get(&config);

    device_mode_t new_mode = (radio->selected_index == 0) ? DEVICE_MODE_PRESENTER : DEVICE_MODE_PC;

    if (new_mode != config.device_mode) {
        config.device_mode = new_mode;
        general_config_set(&config);

        // Update the saved/committed value (filled radio)
        if (radio->selected_items) {
            ((int *)radio->selected_items)[0] = radio->selected_index;
        }

        system_event_mode_t evt = {.mode = new_mode};
        esp_event_post_to(system_events_get_loop(), SYSTEM_EVENTS, SYSTEM_EVENT_MODE_CHANGED, &evt, sizeof(evt), 0);

        ESP_LOGI(TAG, "Device mode changed to: %s", new_mode == DEVICE_MODE_PRESENTER ? UI_STR_PRESENTER : "PC");
    }
}

void screen_device_mode_reset(void)
{
    if (radio) {
        preserved_index = radio->selected_index; // Save before destroying
        if (radio->selected_items) {
            free(radio->selected_items);
        }
        free(radio);
        radio = NULL;
    }
}

static void handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_SHORT) {
        screen_device_mode_navigate_down();
        ui_navigator_switch_to(UI_SCREEN_DEVICE_MODE);
    } else if (event == INPUT_EVENT_NEXT_LONG) {
        screen_device_mode_select();
        ui_navigator_switch_to(UI_SCREEN_MENU);
    } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
            ui_navigator_switch_to(UI_SCREEN_MENU);
            break;
        case INPUT_EVENT_ENCODER_CW:
            screen_device_mode_navigate_down();
            ui_navigator_switch_to(UI_SCREEN_DEVICE_MODE);
            break;
        case INPUT_EVENT_ENCODER_CCW:
            screen_device_mode_navigate_up();
            ui_navigator_switch_to(UI_SCREEN_DEVICE_MODE);
            break;
        case INPUT_EVENT_NEXT_SHORT:
        case INPUT_EVENT_ENCODER_BUTTON_SHORT:
            screen_device_mode_select();
            ui_navigator_switch_to(UI_SCREEN_MENU);
            break;
        default:
            break;
    }
#endif
}

static ui_screen_t device_mode_screen = {
    .type               = UI_SCREEN_DEVICE_MODE,
    .create             = screen_device_mode_create,
    .destroy            = screen_device_mode_reset,
    .handle_input_event = handle_input_event,
};

ui_screen_t *screen_device_mode_get_interface(void)
{
    return &device_mode_screen;
}
