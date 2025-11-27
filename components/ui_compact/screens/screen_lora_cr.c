#include "lvgl.h"
#include "ui_components.h"
#include "lora_driver.h"
#include "esp_log.h"

static const char *TAG = "lora_cr";
static ui_dropdown_t *dropdown = NULL;

static const char *cr_options[] = {"4/5", "4/6", "4/7", "4/8"};
#define CR_OPTION_COUNT 4

void screen_lora_cr_init(void) {
    lora_config_t config;
    lora_get_config(&config);
    int index = config.coding_rate - 1;  // CR 1=4/5, 2=4/6, etc.
    if (index < 0) index = 0;
    if (index >= CR_OPTION_COUNT) index = CR_OPTION_COUNT - 1;
    
    if (!dropdown) {
        dropdown = ui_dropdown_create(index, CR_OPTION_COUNT);
    } else {
        dropdown->selected_index = index;
    }
}

void screen_lora_cr_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!dropdown) screen_lora_cr_init();
    ui_dropdown_render(dropdown, parent, "CODING RATE", cr_options);
}

void screen_lora_cr_navigate_down(void) {
    if (dropdown && dropdown->edit_mode) {
        ui_dropdown_next(dropdown);
    }
}

void screen_lora_cr_navigate_up(void) {
    if (dropdown && dropdown->edit_mode) {
        ui_dropdown_prev(dropdown);
    }
}

void screen_lora_cr_select(void) {
    if (!dropdown) return;
    
    if (!dropdown->edit_mode) {
        ESP_LOGI(TAG, "Entering edit mode");
        dropdown->edit_mode = true;
    } else {
        lora_config_t config;
        lora_get_config(&config);
        config.coding_rate = dropdown->selected_index + 1;  // 0->1, 1->2, etc.
        lora_set_config(&config);
        dropdown->edit_mode = false;
        ESP_LOGI(TAG, "Coding rate set to %s", cr_options[dropdown->selected_index]);
    }
}

bool screen_lora_cr_is_edit_mode(void) {
    return dropdown && dropdown->edit_mode;
}

void screen_lora_cr_reset(void) {
    if (dropdown) {
        free(dropdown);
        dropdown = NULL;
    }
}
