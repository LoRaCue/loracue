#include "lvgl.h"
#include "ui_components.h"
#include "lora_driver.h"
#include "esp_log.h"

static const char *TAG = "lora_bw";
static ui_dropdown_t *dropdown = NULL;

static const char *bw_options[] = {"125 kHz", "250 kHz", "500 kHz"};
static const uint16_t bw_values[] = {125, 250, 500};
#define BW_OPTION_COUNT 3

void screen_lora_bw_init(void) {
    lora_config_t config;
    lora_get_config(&config);
    
    int index = 0;
    for (int i = 0; i < BW_OPTION_COUNT; i++) {
        if (config.bandwidth == bw_values[i]) {
            index = i;
            break;
        }
    }
    
    if (!dropdown) {
        dropdown = ui_dropdown_create(index, BW_OPTION_COUNT);
    } else {
        dropdown->selected_index = index;
    }
}

void screen_lora_bw_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!dropdown) screen_lora_bw_init();
    ui_dropdown_render(dropdown, parent, "BANDWIDTH", bw_options);
}

void screen_lora_bw_navigate_down(void) {
    if (dropdown && dropdown->edit_mode) {
        ui_dropdown_next(dropdown);
    }
}

void screen_lora_bw_select(void) {
    if (!dropdown) return;
    
    if (dropdown->edit_mode) {
        // Save to config
        lora_config_t config;
        lora_get_config(&config);
        config.bandwidth = bw_values[dropdown->selected_index];
        lora_set_config(&config);
        ESP_LOGI(TAG, "BW saved: %d kHz", config.bandwidth);
        dropdown->edit_mode = false;
    } else {
        dropdown->edit_mode = true;
    }
}

bool screen_lora_bw_is_edit_mode(void) {
    return dropdown ? dropdown->edit_mode : false;
}

void screen_lora_bw_reset(void) {
    if (dropdown) {
        free(dropdown);
        dropdown = NULL;
    }
}
