#include "lvgl.h"
#include "ui_components.h"
#include "lora_driver.h"
#include "lora_bands.h"
#include "esp_log.h"

static const char *TAG = "lora_freq";
static ui_numeric_input_t *input = NULL;
static uint32_t min_freq_khz = 863000;
static uint32_t max_freq_khz = 870000;

void screen_lora_frequency_init(void) {
    lora_config_t config;
    lora_get_config(&config);
    float freq_mhz = config.frequency / 1000000.0f;
    
    // Get limits from band profile
    const lora_band_profile_t *profile = lora_bands_get_profile_by_id(config.band_id);
    if (profile) {
        min_freq_khz = profile->optimal_freq_min_khz;
        max_freq_khz = profile->optimal_freq_max_khz;
    }
    
    if (!input) {
        input = ui_numeric_input_create(freq_mhz, min_freq_khz / 1000.0f, max_freq_khz / 1000.0f, 0.1f);
    } else {
        input->value = freq_mhz;
        input->min = min_freq_khz / 1000.0f;
        input->max = max_freq_khz / 1000.0f;
    }
}

void screen_lora_frequency_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!input) screen_lora_frequency_init();
    ui_numeric_input_render(input, parent, "FREQUENCY", "MHz");
}

void screen_lora_frequency_navigate_down(void) {
    if (input && input->edit_mode) {
        ui_numeric_input_increment(input);
    }
}

void screen_lora_frequency_navigate_up(void) {
    if (input && input->edit_mode) {
        ui_numeric_input_decrement(input);
    }
}

void screen_lora_frequency_select(void) {
    if (!input) return;
    
    if (input->edit_mode) {
        // Save to config
        lora_config_t config;
        lora_get_config(&config);
        config.frequency = (uint32_t)(input->value * 1000000.0f);
        lora_set_config(&config);
        ESP_LOGI(TAG, "Frequency saved: %.1f MHz", input->value);
        input->edit_mode = false;
    } else {
        input->edit_mode = true;
    }
}

bool screen_lora_frequency_is_edit_mode(void) {
    return input ? input->edit_mode : false;
}

void screen_lora_frequency_reset(void) {
    if (input) {
        free(input);
        input = NULL;
    }
}
