#include "input_manager.h"
#include "ui_strings.h"
#include "esp_log.h"
#include "input_manager.h"
#include "lora_bands.h"
#include "lora_driver.h"
#include "lvgl.h"
#include "screens.h"
#include "ui_components.h"

static const char *TAG          = "lora_presets";
static ui_radio_select_t *radio = NULL;
static int preserved_index      = -1;

typedef enum { PRESET_CONFERENCE = 0, PRESET_AUDITORIUM, PRESET_STADIUM, PRESET_COUNT } lora_preset_t;

static const char *PRESET_NAMES[PRESET_COUNT] = {[PRESET_CONFERENCE] = "Conference (100m)",
                                                 [PRESET_AUDITORIUM] = "Auditorium (250m)",
                                                 [PRESET_STADIUM]    = "Stadium (500m)"};

static const lora_config_t PRESETS[PRESET_COUNT] = {[PRESET_CONFERENCE] =
                                                        {
                                                            .spreading_factor = 7,
                                                            .bandwidth        = 500,
                                                            .coding_rate      = 5,
                                                        },
                                                    [PRESET_AUDITORIUM] =
                                                        {
                                                            .spreading_factor = 9,
                                                            .bandwidth        = 125,
                                                            .coding_rate      = 7,
                                                        },
                                                    [PRESET_STADIUM] = {
                                                        .spreading_factor = 10,
                                                        .bandwidth        = 125,
                                                        .coding_rate      = 8,
                                                    }};

static int get_current_preset(void)
{
    lora_config_t config;
    if (lora_get_config(&config) != ESP_OK) {
        return 0;
    }

    for (int i = 0; i < PRESET_COUNT; i++) {
        if (PRESETS[i].spreading_factor == config.spreading_factor && PRESETS[i].bandwidth == config.bandwidth &&
            PRESETS[i].coding_rate == config.coding_rate) {
            return i;
        }
    }
    return 0;
}

static int8_t get_tx_power_for_band(const char *band_id)
{
    const lora_hardware_t *hardware = lora_hardware_get_profile_by_id(band_id);
    // TODO: Should use regulatory compliance rules instead of hardware limits
    return hardware ? hardware->max_tx_power : 14; // Default power, needs regulatory lookup
}

static int current_nav_index = 0;

void screen_lora_presets_on_enter(void)
{
    current_nav_index = get_current_preset();
}

void screen_lora_presets_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    if (!radio) {
        radio                 = ui_radio_select_create(PRESET_COUNT, UI_RADIO_SINGLE);
        radio->selected_index = preserved_index >= 0 ? preserved_index : current_nav_index;

        // Mark the currently active preset (saved value)
        if (radio->selected_items) {
            ((int *)radio->selected_items)[0] = get_current_preset();
        }
    }

    ui_radio_select_render(radio, parent, "LORA PRESETS", PRESET_NAMES);
}

void screen_lora_presets_navigate_down(void)
{
    current_nav_index = (current_nav_index + 1) % PRESET_COUNT;
}

void screen_lora_presets_navigate_up(void)
{
    current_nav_index = (current_nav_index - 1 + PRESET_COUNT) % PRESET_COUNT;
}

void screen_lora_presets_select(void)
{
    lora_config_t config;
    if (lora_get_config(&config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get current config");
        return;
    }

    lora_config_t new_config = PRESETS[current_nav_index];
    new_config.frequency     = config.frequency;
    strncpy(new_config.band_id, config.band_id, sizeof(new_config.band_id) - 1);
    new_config.tx_power = get_tx_power_for_band(config.band_id);

    if (lora_set_config(&new_config) == ESP_OK) {
        ESP_LOGI(TAG, "Applied preset: %s", PRESET_NAMES[current_nav_index]);
    } else {
        ESP_LOGE(TAG, "Failed to apply preset");
    }
}

#include "ui_navigator.h"
#include "ui_screen_interface.h"

static void screen_lora_presets_handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_SHORT) {
        screen_lora_presets_navigate_down();
        ui_navigator_switch_to(UI_SCREEN_LORA_PRESETS);
    } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
        screen_lora_presets_navigate_up();
        ui_navigator_switch_to(UI_SCREEN_LORA_PRESETS);
    } else if (event == INPUT_EVENT_NEXT_LONG) {
        screen_lora_presets_select();
        ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
        case INPUT_EVENT_ENCODER_BUTTON_SHORT:
            ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
            break;
        case INPUT_EVENT_ENCODER_CW:
            screen_lora_presets_navigate_down();
            ui_navigator_switch_to(UI_SCREEN_LORA_PRESETS);
            break;
        case INPUT_EVENT_ENCODER_CCW:
            screen_lora_presets_navigate_up();
            ui_navigator_switch_to(UI_SCREEN_LORA_PRESETS);
            break;
        case INPUT_EVENT_NEXT_SHORT:
        case INPUT_EVENT_ENCODER_BUTTON_LONG:
            screen_lora_presets_select();
            ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
            break;
        default:
            break;
    }
#endif
}

void screen_lora_presets_reset(void)
{
    if (radio) {
        preserved_index = radio->selected_index;
        free(radio);
        radio = NULL;
    }
}

ui_screen_t *screen_lora_presets_get_interface(void)
{
    static ui_screen_t screen = {.type               = UI_SCREEN_LORA_PRESETS,
                                 .create             = screen_lora_presets_create,
                                 .destroy            = screen_lora_presets_reset,
                                 .handle_input_event = screen_lora_presets_handle_input_event,
                                 .on_enter           = screen_lora_presets_on_enter,
                                 .on_exit            = NULL};
    return &screen;
}
