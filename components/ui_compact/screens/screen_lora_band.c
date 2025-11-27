#include "lvgl.h"
#include "ui_components.h"
#include "lora_driver.h"
#include "lora_bands.h"
#include "esp_log.h"

static const char *TAG = "lora_band";
static ui_radio_select_t *radio = NULL;
static const char **band_names = NULL;
static int band_count = 0;

void screen_lora_band_init(void) {
    band_count = lora_bands_get_count();
    
    if (!band_names) {
        band_names = malloc(band_count * sizeof(char*));
        for (int i = 0; i < band_count; i++) {
            const lora_band_profile_t *profile = lora_bands_get_profile(i);
            band_names[i] = profile ? profile->name : "Unknown";
        }
    }
    
    if (!radio) {
        radio = ui_radio_select_create(band_count, UI_RADIO_SINGLE);
        
        // Set current band as selected
        lora_config_t config;
        lora_get_config(&config);
        for (int i = 0; i < band_count; i++) {
            const lora_band_profile_t *profile = lora_bands_get_profile(i);
            if (profile && strcmp(profile->id, config.band_id) == 0) {
                radio->selected_index = i;
                if (radio->selected_items) {
                    ((int*)radio->selected_items)[0] = i;
                }
                break;
            }
        }
    }
}

void screen_lora_band_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!radio) screen_lora_band_init();
    ui_radio_select_render(radio, parent, "FREQUENCY BAND", band_names);
}

void screen_lora_band_navigate_down(void) {
    ui_radio_select_navigate_down(radio);
}

void screen_lora_band_select(void) {
    if (!radio) return;
    
    const lora_band_profile_t *profile = lora_bands_get_profile(radio->selected_index);
    if (profile) {
        lora_config_t config;
        lora_get_config(&config);
        strncpy(config.band_id, profile->id, sizeof(config.band_id) - 1);
        config.band_id[sizeof(config.band_id) - 1] = '\0';
        lora_set_config(&config);
        
        // Update saved value
        if (radio->selected_items) {
            ((int*)radio->selected_items)[0] = radio->selected_index;
        }
        
        ESP_LOGI(TAG, "Band set to %s", profile->name);
    }
}

void screen_lora_band_reset(void) {
    if (radio) {
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
