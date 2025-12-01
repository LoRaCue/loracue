#include "input_manager.h"
#include "ui_strings.h"
#include "esp_log.h"
#include "input_manager.h"
#include "lora_driver.h"
#include "lvgl.h"
#include "ui_components.h"
#include "ui_navigator.h"
#include "ui_screen_interface.h"

static const char *TAG          = "lora_cr";
static ui_radio_select_t *radio = NULL;

static const char *cr_options[]  = {"4/5", "4/6", "4/7", "4/8"};
static const uint8_t cr_values[] = {5, 6, 7, 8};
#define CR_OPTION_COUNT 4

static int current_cr_index = 0;
static int preserved_index  = -1;

void screen_lora_cr_on_enter(void)
{
    lora_config_t config;
    lora_get_config(&config);
    current_cr_index = 0;
    for (int i = 0; i < CR_OPTION_COUNT; i++) {
        if (config.coding_rate == cr_values[i]) {
            current_cr_index = i;
            break;
        }
    }
}

void screen_lora_cr_init(void)
{
    if (!radio) {
        radio                 = ui_radio_select_create(CR_OPTION_COUNT, UI_RADIO_SINGLE);
        radio->selected_index = preserved_index >= 0 ? preserved_index : current_cr_index;

        if (radio->selected_items) {
            ((int *)radio->selected_items)[0] = current_cr_index;
        }
    }
}

void screen_lora_cr_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!radio)
        screen_lora_cr_init();
    ui_radio_select_render(radio, parent, "CODING RATE", cr_options);
}

void screen_lora_cr_navigate_down(void)
{
    ui_radio_select_navigate_down(radio);
}

void screen_lora_cr_select(void)
{
    if (!radio)
        return;

    lora_config_t config;
    lora_get_config(&config);
    config.coding_rate = cr_values[radio->selected_index];
    lora_set_config(&config);
    ESP_LOGI(TAG, "CR saved: 4/%d", config.coding_rate);

    if (radio->selected_items) {
        ((int *)radio->selected_items)[0] = radio->selected_index;
    }
}

static void screen_lora_cr_handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_SHORT) {
        screen_lora_cr_navigate_down();
        ui_navigator_switch_to(UI_SCREEN_LORA_CR);
    } else if (event == INPUT_EVENT_NEXT_LONG) {
        screen_lora_cr_select();
        ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
    } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
            ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
            break;
        case INPUT_EVENT_ENCODER_CW:
            screen_lora_cr_navigate_down();
            ui_navigator_switch_to(UI_SCREEN_LORA_CR);
            break;
        case INPUT_EVENT_ENCODER_CCW:
            if (radio) {
                ui_radio_select_navigate_up(radio);
            }
            ui_navigator_switch_to(UI_SCREEN_LORA_CR);
            break;
        case INPUT_EVENT_NEXT_SHORT:
        case INPUT_EVENT_ENCODER_BUTTON_SHORT:
            screen_lora_cr_select();
            ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
            break;
        default:
            break;
    }
#endif
}

void screen_lora_cr_reset(void)
{
    if (radio) {
        preserved_index = radio->selected_index;
        if (radio->selected_items) {
            free(radio->selected_items);
        }
        free(radio);
        radio = NULL;
    }
}

ui_screen_t *screen_lora_cr_get_interface(void)
{
    static ui_screen_t screen = {.type               = UI_SCREEN_LORA_CR,
                                 .create             = screen_lora_cr_create,
                                 .destroy            = screen_lora_cr_reset,
                                 .handle_input_event = screen_lora_cr_handle_input_event,
                                 .on_enter           = screen_lora_cr_on_enter,
                                 .on_exit            = NULL};
    return &screen;
}
