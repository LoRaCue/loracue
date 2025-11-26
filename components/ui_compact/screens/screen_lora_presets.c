#include "lvgl.h"
#include "ui_components.h"
#include "screens.h"
#include "lora_driver.h"
#include "lora_bands.h"
#include "esp_log.h"

static const char *TAG = "lora_presets";
static ui_radio_select_t *radio = NULL;

typedef enum {
    PRESET_CONFERENCE = 0,
    PRESET_AUDITORIUM,
    PRESET_STADIUM,
    PRESET_COUNT
} lora_preset_t;

static const char *PRESET_NAMES[PRESET_COUNT] = {
    [PRESET_CONFERENCE] = "Conference (100m)",
    [PRESET_AUDITORIUM] = "Auditorium (250m)",
    [PRESET_STADIUM] = "Stadium (500m)"
};

static const lora_config_t PRESETS[PRESET_COUNT] = {
    [PRESET_CONFERENCE] = {
        .spreading_factor = 7,
        .bandwidth = 500,
        .coding_rate = 5,
    },
    [PRESET_AUDITORIUM] = {
        .spreading_factor = 9,
        .bandwidth = 125,
        .coding_rate = 7,
    },
    [PRESET_STADIUM] = {
        .spreading_factor = 10,
        .bandwidth = 125,
        .coding_rate = 8,
    }
};

static int get_current_preset(void) {
    lora_config_t config;
    if (lora_get_config(&config) != ESP_OK) {
        return 0;
    }
    
    for (int i = 0; i < PRESET_COUNT; i++) {
        if (PRESETS[i].spreading_factor == config.spreading_factor &&
            PRESETS[i].bandwidth == config.bandwidth &&
            PRESETS[i].coding_rate == config.coding_rate) {
            return i;
        }
    }
    return 0;
}

static int8_t get_tx_power_for_band(const char *band_id) {
    const lora_band_profile_t *band = lora_bands_get_profile_by_id(band_id);
    return band ? band->max_power_dbm : 14;
}

void screen_lora_presets_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    if (!radio) {
        radio = ui_radio_select_create(PRESET_COUNT, UI_RADIO_SINGLE);
        radio->selected_index = get_current_preset();
    }
    
    ui_radio_select_render(radio, parent, "LORA PRESETS", PRESET_NAMES);
}

void screen_lora_presets_navigate_down(void) {
    if (!radio) return;
    radio->selected_index = (radio->selected_index + 1) % PRESET_COUNT;
}

void screen_lora_presets_select(void) {
    if (!radio) return;
    
    lora_config_t config;
    if (lora_get_config(&config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get current config");
        return;
    }
    
    lora_config_t new_config = PRESETS[radio->selected_index];
    new_config.frequency = config.frequency;
    strncpy(new_config.band_id, config.band_id, sizeof(new_config.band_id) - 1);
    new_config.tx_power = get_tx_power_for_band(config.band_id);
    
    if (lora_set_config(&new_config) == ESP_OK) {
        ESP_LOGI(TAG, "Applied preset: %s", PRESET_NAMES[radio->selected_index]);
    } else {
        ESP_LOGE(TAG, "Failed to apply preset");
    }
}

void screen_lora_presets_reset(void) {
    if (radio) {
        free(radio);
        radio = NULL;
    }
}
