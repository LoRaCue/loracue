#include "input_manager.h"
#include "esp_log.h"
#include "input_manager.h"
#include "lora_bands.h"
#include "lora_driver.h"
#include "lvgl.h"
#include "ui_components.h"

static const char *TAG           = "lora_freq";
static ui_numeric_input_t *input = NULL;
static uint32_t min_freq_khz     = 863000;
static uint32_t max_freq_khz     = 870000;

static float current_freq_mhz   = 0;
static bool is_editing          = false;
static bool preserved_edit_mode = false; // Preserve edit mode across recreations

void screen_lora_frequency_on_enter(void)
{
    lora_config_t config;
    lora_get_config(&config);
    current_freq_mhz = config.frequency / 1000000.0f;
    is_editing       = false;
}

void screen_lora_frequency_init(void)
{
    lora_config_t config;
    lora_get_config(&config);

    // Initialize current frequency from config
    current_freq_mhz = config.frequency / 1000000.0f;

    // Get limits from band profile
    const lora_band_profile_t *profile = lora_bands_get_profile_by_id(config.band_id);
    if (profile) {
        min_freq_khz = profile->optimal_freq_min_khz;
        max_freq_khz = profile->optimal_freq_max_khz;
    }

    if (!input) {
        input = ui_numeric_input_create(current_freq_mhz, min_freq_khz / 1000.0f, max_freq_khz / 1000.0f, 0.1f);
        // Restore preserved edit mode if available
        input->edit_mode = preserved_edit_mode ? preserved_edit_mode : is_editing;
    }
}

void screen_lora_frequency_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!input)
        screen_lora_frequency_init();
    ui_numeric_input_render(input, parent, "FREQUENCY", "MHz");
}

void screen_lora_frequency_navigate_down(void)
{
    if (input && input->edit_mode) {
        ui_numeric_input_increment(input);
        current_freq_mhz = input->value;
    }
}

void screen_lora_frequency_navigate_up(void)
{
    if (input && input->edit_mode) {
        ui_numeric_input_decrement(input);
        current_freq_mhz = input->value;
    }
}

void screen_lora_frequency_select(void)
{
    if (!input)
        return;

    if (input->edit_mode) {
        // Save to config
        lora_config_t config;
        lora_get_config(&config);
        config.frequency = (uint32_t)(input->value * 1000000.0f);
        lora_set_config(&config);
        ESP_LOGI(TAG, "Frequency saved: %.1f MHz", input->value);
        input->edit_mode = false;
        is_editing       = false;
    } else {
        input->edit_mode = true;
        is_editing       = true;
    }
}

bool screen_lora_frequency_is_edit_mode(void)
{
    return input ? input->edit_mode : false;
}

#include "ui_navigator.h"
#include "ui_screen_interface.h"

static void screen_lora_frequency_handle_input(button_event_type_t event)
{
    // Use our static state for check, or the helper (which checks input)
    // But input might be NULL if not created? No, handle_input is called when screen is active.

    bool is_edit = is_editing; // Use static state

    if (event == BUTTON_EVENT_SHORT && is_edit) {
        screen_lora_frequency_navigate_down();
        ui_navigator_switch_to(UI_SCREEN_LORA_FREQUENCY);
    } else if (event == BUTTON_EVENT_DOUBLE && is_edit) {
        screen_lora_frequency_navigate_up();
        ui_navigator_switch_to(UI_SCREEN_LORA_FREQUENCY);
    } else if (event == BUTTON_EVENT_LONG) {
        screen_lora_frequency_select();

        if (!is_editing) {
            ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
        } else {
            // Entered edit mode
            ui_navigator_switch_to(UI_SCREEN_LORA_FREQUENCY);
        }
    } else if (event == BUTTON_EVENT_DOUBLE && !is_edit) {
        // Back to submenu
        ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
    }
}

void screen_lora_frequency_reset(void)
{
    if (input) {
        preserved_edit_mode = input->edit_mode; // Save before destroying
        free(input);
        input = NULL;
    }
}

ui_screen_t *screen_lora_frequency_get_interface(void)
{
    static ui_screen_t screen = {.type         = UI_SCREEN_LORA_FREQUENCY,
                                 .create       = screen_lora_frequency_create,
                                 .destroy      = screen_lora_frequency_reset,
                                 .handle_input = screen_lora_frequency_handle_input,
                                 .on_enter     = screen_lora_frequency_on_enter,
                                 .on_exit      = NULL};
    return &screen;
}
