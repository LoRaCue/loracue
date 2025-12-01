#include "input_manager.h"
#include "ui_strings.h"
#include "esp_log.h"
#include "input_manager.h"
#include "lora_driver.h"
#include "lvgl.h"
#include "ui_components.h"

static const char *TAG           = "lora_txpower";
static ui_numeric_input_t *input = NULL;

static float current_tx_power   = 0;
static bool is_editing          = false;
static bool preserved_edit_mode = false;

void screen_lora_txpower_on_enter(void)
{
    lora_config_t config;
    lora_get_config(&config);
    current_tx_power = config.tx_power;
    is_editing       = false;
}

void screen_lora_txpower_init(void)
{
    if (!input) {
        input            = ui_numeric_input_create(current_tx_power, -9, 22, 1);
        input->edit_mode = preserved_edit_mode ? preserved_edit_mode : is_editing;
    }
}

void screen_lora_txpower_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!input)
        screen_lora_txpower_init();
    ui_numeric_input_render(input, parent, "TX POWER", "dBm");
}

void screen_lora_txpower_navigate_down(void)
{
    if (input && input->edit_mode) {
        ui_numeric_input_increment(input);
        current_tx_power = input->value;
    }
}

void screen_lora_txpower_navigate_up(void)
{
    if (input && input->edit_mode) {
        ui_numeric_input_decrement(input);
        current_tx_power = input->value;
    }
}

void screen_lora_txpower_select(void)
{
    if (!input)
        return;

    if (!input->edit_mode) {
        input->edit_mode = true;
        is_editing       = true;
    } else {
        lora_config_t config;
        lora_get_config(&config);
        config.tx_power = (int8_t)input->value;
        lora_set_config(&config);
        input->edit_mode = false;
        is_editing       = false;
        ESP_LOGI(TAG, "TX power set to %d dBm", config.tx_power);
    }
}

bool screen_lora_txpower_is_edit_mode(void)
{
    return input && input->edit_mode;
}

#include "ui_navigator.h"
#include "ui_screen_interface.h"

static void screen_lora_txpower_handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    bool is_edit = is_editing;
    if (event == INPUT_EVENT_NEXT_SHORT && is_edit) {
        screen_lora_txpower_navigate_down();
        ui_navigator_switch_to(UI_SCREEN_LORA_TXPOWER);
    } else if (event == INPUT_EVENT_NEXT_DOUBLE && is_edit) {
        screen_lora_txpower_navigate_up();
        ui_navigator_switch_to(UI_SCREEN_LORA_TXPOWER);
    } else if (event == INPUT_EVENT_NEXT_LONG) {
        screen_lora_txpower_select();
        if (!is_editing) {
            ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
        } else {
            ui_navigator_switch_to(UI_SCREEN_LORA_TXPOWER);
        }
    } else if (event == INPUT_EVENT_NEXT_DOUBLE && !is_edit) {
        ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    bool is_edit = is_editing;
    if (is_edit) {
        switch (event) {
            case INPUT_EVENT_PREV_SHORT:
            case INPUT_EVENT_ENCODER_BUTTON_SHORT:
                if (input) {
                    input->edit_mode = false;
                }
                is_editing = false;
                ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
                break;
            case INPUT_EVENT_ENCODER_CW:
                screen_lora_txpower_navigate_down();
                ui_navigator_switch_to(UI_SCREEN_LORA_TXPOWER);
                break;
            case INPUT_EVENT_ENCODER_CCW:
                screen_lora_txpower_navigate_up();
                ui_navigator_switch_to(UI_SCREEN_LORA_TXPOWER);
                break;
            case INPUT_EVENT_ENCODER_BUTTON_LONG:
                screen_lora_txpower_select();
                ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
                break;
            default:
                break;
        }
    } else {
        switch (event) {
            case INPUT_EVENT_PREV_SHORT:
            case INPUT_EVENT_ENCODER_BUTTON_SHORT:
                ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
                break;
            case INPUT_EVENT_ENCODER_BUTTON_LONG:
                screen_lora_txpower_select();
                ui_navigator_switch_to(UI_SCREEN_LORA_TXPOWER);
                break;
            default:
                break;
        }
    }
#endif
}

void screen_lora_txpower_reset(void)
{
    if (input) {
        preserved_edit_mode = input->edit_mode;
        free(input);
        input = NULL;
    }
}

ui_screen_t *screen_lora_txpower_get_interface(void)
{
    static ui_screen_t screen = {.type               = UI_SCREEN_LORA_TXPOWER,
                                 .create             = screen_lora_txpower_create,
                                 .destroy            = screen_lora_txpower_reset,
                                 .handle_input_event = screen_lora_txpower_handle_input_event,
                                 .on_enter           = screen_lora_txpower_on_enter,
                                 .on_exit      = NULL};
    return &screen;
}
