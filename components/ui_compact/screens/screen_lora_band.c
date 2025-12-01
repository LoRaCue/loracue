#include "input_manager.h"
#include "esp_log.h"
#include "input_manager.h"
#include "lora_bands.h"
#include "lora_driver.h"
#include "lvgl.h"
#include "ui_components.h"

static const char *TAG          = "lora_band";
static ui_radio_select_t *radio = NULL;
static int preserved_index      = -1;
static const char **band_names  = NULL;
static int band_count           = 0;

static int current_band_index = 0;

void screen_lora_band_on_enter(void)
{
    band_count = lora_bands_get_count();
    lora_config_t config;
    lora_get_config(&config);
    current_band_index = 0;
    for (int i = 0; i < band_count; i++) {
        const lora_band_profile_t *profile = lora_bands_get_profile(i);
        if (profile && strcmp(profile->id, config.band_id) == 0) {
            current_band_index = i;
            break;
        }
    }
}

void screen_lora_band_init(void)
{
    band_count = lora_bands_get_count();

    if (!band_names) {
        band_names = malloc(band_count * sizeof(char *));
        for (int i = 0; i < band_count; i++) {
            const lora_band_profile_t *profile = lora_bands_get_profile(i);
            band_names[i]                      = profile ? profile->name : "Unknown";
        }
    }

    if (!radio) {
        radio                 = ui_radio_select_create(band_count, UI_RADIO_SINGLE);
        radio->selected_index = preserved_index >= 0 ? preserved_index : current_band_index;

        // Set current band as selected (saved value)
        lora_config_t config;
        lora_get_config(&config);
        for (int i = 0; i < band_count; i++) {
            const lora_band_profile_t *profile = lora_bands_get_profile(i);
            if (profile && strcmp(profile->id, config.band_id) == 0) {
                if (radio->selected_items) {
                    ((int *)radio->selected_items)[0] = i;
                }
                break;
            }
        }
    }
}

void screen_lora_band_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!radio)
        screen_lora_band_init();
    ui_radio_select_render(radio, parent, "FREQUENCY BAND", band_names);
}

void screen_lora_band_navigate_down(void)
{
    ui_radio_select_navigate_down(radio);
    current_band_index = radio->selected_index;
}

void screen_lora_band_navigate_up(void)
{
    ui_radio_select_navigate_up(radio);
    current_band_index = radio->selected_index;
}

void screen_lora_band_select(void)
{
    if (!radio)
        return;

    const lora_band_profile_t *profile = lora_bands_get_profile(radio->selected_index);
    if (profile) {
        lora_config_t config;
        lora_get_config(&config);
        strncpy(config.band_id, profile->id, sizeof(config.band_id) - 1);
        config.band_id[sizeof(config.band_id) - 1] = '\0';
        lora_set_config(&config);

        // Update saved value
        if (radio->selected_items) {
            ((int *)radio->selected_items)[0] = radio->selected_index;
        }

        ESP_LOGI(TAG, "Band set to %s", profile->name);
    }
}

bool screen_lora_band_is_edit_mode(void)
{
    // Radio select doesn't have edit mode - always in selection mode
    return false;
}

#include "ui_navigator.h"
#include "ui_screen_interface.h"

static void screen_lora_band_handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_SHORT) {
        screen_lora_band_navigate_down();
        ui_navigator_switch_to(UI_SCREEN_LORA_BAND);
    } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
        screen_lora_band_navigate_up();
        ui_navigator_switch_to(UI_SCREEN_LORA_BAND);
    } else if (event == INPUT_EVENT_NEXT_LONG) {
        screen_lora_band_select();
        ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
            ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
            break;
        case INPUT_EVENT_ENCODER_CW:
            screen_lora_band_navigate_down();
            ui_navigator_switch_to(UI_SCREEN_LORA_BAND);
            break;
        case INPUT_EVENT_ENCODER_CCW:
            screen_lora_band_navigate_up();
            ui_navigator_switch_to(UI_SCREEN_LORA_BAND);
            break;
        case INPUT_EVENT_NEXT_SHORT:
        case INPUT_EVENT_ENCODER_BUTTON_SHORT:
            screen_lora_band_select();
            ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
            break;
        default:
            break;
    }
#endif
}

void screen_lora_band_reset(void)
{
    if (radio) {
        preserved_index = radio->selected_index;
        if (radio->selected_items) {
            free(radio->selected_items);
        }
        free(radio);
        radio = NULL;
    }
    if (band_names) {
        free(band_names);
        band_names = NULL;
    }
}

ui_screen_t *screen_lora_band_get_interface(void)
{
    static ui_screen_t screen = {.type               = UI_SCREEN_LORA_BAND,
                                 .create             = screen_lora_band_create,
                                 .destroy            = screen_lora_band_reset,
                                 .handle_input_event = screen_lora_band_handle_input_event,
                                 .on_enter     = screen_lora_band_on_enter,
                                 .on_exit      = NULL};
    return &screen;
}
