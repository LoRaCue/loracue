#include "input_manager.h"
#include "esp_log.h"
#include "input_manager.h"
#include "lora_driver.h"
#include "lvgl.h"
#include "ui_components.h"
#include "ui_navigator.h"
#include "ui_screen_interface.h"

static const char *TAG          = "lora_sf";
static ui_radio_select_t *radio = NULL;

static const char *sf_options[] = {"SF7", "SF8", "SF9", "SF10", "SF11", "SF12"};
#define SF_OPTION_COUNT 6

static int current_sf_index = 0;
static int preserved_index  = -1;

void screen_lora_sf_on_enter(void)
{
    lora_config_t config;
    lora_get_config(&config);
    current_sf_index = config.spreading_factor - 7;
    if (current_sf_index < 0)
        current_sf_index = 0;
    if (current_sf_index >= SF_OPTION_COUNT)
        current_sf_index = SF_OPTION_COUNT - 1;
}

void screen_lora_sf_init(void)
{
    if (!radio) {
        radio                 = ui_radio_select_create(SF_OPTION_COUNT, UI_RADIO_SINGLE);
        radio->selected_index = preserved_index >= 0 ? preserved_index : current_sf_index;

        if (radio->selected_items) {
            ((int *)radio->selected_items)[0] = current_sf_index;
        }
    }
}

void screen_lora_sf_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!radio)
        screen_lora_sf_init();
    ui_radio_select_render(radio, parent, "SPREAD FACTOR", sf_options);
}

void screen_lora_sf_navigate_down(void)
{
    ui_radio_select_navigate_down(radio);
}

void screen_lora_sf_select(void)
{
    if (!radio)
        return;

    lora_config_t config;
    lora_get_config(&config);
    config.spreading_factor = 7 + radio->selected_index;
    lora_set_config(&config);
    ESP_LOGI(TAG, "SF saved: SF%d", config.spreading_factor);

    if (radio->selected_items) {
        ((int *)radio->selected_items)[0] = radio->selected_index;
    }
}

static void screen_lora_sf_handle_input(button_event_type_t event)
{
    if (event == BUTTON_EVENT_SHORT) {
        screen_lora_sf_navigate_down();
        ui_navigator_switch_to(UI_SCREEN_LORA_SF);
    } else if (event == BUTTON_EVENT_LONG) {
        screen_lora_sf_select();
        ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
    } else if (event == BUTTON_EVENT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
    }
}

void screen_lora_sf_reset(void)
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

ui_screen_t *screen_lora_sf_get_interface(void)
{
    static ui_screen_t screen = {.type         = UI_SCREEN_LORA_SF,
                                 .create       = screen_lora_sf_create,
                                 .destroy      = screen_lora_sf_reset,
                                 .handle_input = screen_lora_sf_handle_input,
                                 .on_enter     = screen_lora_sf_on_enter,
                                 .on_exit      = NULL};
    return &screen;
}
