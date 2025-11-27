#include "lvgl.h"
#include "ui_components.h"
#include "lora_driver.h"
#include "esp_log.h"

static const char *TAG = "lora_sf";
static ui_dropdown_t *dropdown = NULL;

static const char *sf_options[] = {"SF7", "SF8", "SF9", "SF10", "SF11", "SF12"};
#define SF_OPTION_COUNT 6

void screen_lora_sf_init(void) {
    lora_config_t config;
    lora_get_config(&config);
    int index = config.spreading_factor - 7;  // SF7=0, SF8=1, etc.
    if (index < 0) index = 0;
    if (index >= SF_OPTION_COUNT) index = SF_OPTION_COUNT - 1;
    
    if (!dropdown) {
        dropdown = ui_dropdown_create(index, SF_OPTION_COUNT);
    } else {
        dropdown->selected_index = index;
    }
}

void screen_lora_sf_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!dropdown) screen_lora_sf_init();
    ui_dropdown_render(dropdown, parent, "SPREAD FACTOR", sf_options);
}

void screen_lora_sf_navigate_down(void) {
    if (dropdown && dropdown->edit_mode) {
        ui_dropdown_next(dropdown);
    }
}

void screen_lora_sf_navigate_up(void) {
    if (dropdown && dropdown->edit_mode) {
        ui_dropdown_prev(dropdown);
    }
}

void screen_lora_sf_select(void) {
    if (!dropdown) return;
    
    if (dropdown->edit_mode) {
        // Save to config
        lora_config_t config;
        lora_get_config(&config);
        config.spreading_factor = 7 + dropdown->selected_index;
        lora_set_config(&config);
        ESP_LOGI(TAG, "SF saved: SF%d", config.spreading_factor);
        dropdown->edit_mode = false;
    } else {
        dropdown->edit_mode = true;
    }
}

bool screen_lora_sf_is_edit_mode(void) {
    return dropdown ? dropdown->edit_mode : false;
}

void screen_lora_sf_reset(void) {
    if (dropdown) {
        free(dropdown);
        dropdown = NULL;
    }
}
