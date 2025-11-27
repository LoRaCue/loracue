#include "lvgl.h"
#include "ui_components.h"
#include "lora_driver.h"
#include "esp_log.h"

static const char *TAG = "lora_txpower";
static ui_numeric_input_t *input = NULL;

void screen_lora_txpower_init(void) {
    lora_config_t config;
    lora_get_config(&config);
    
    if (!input) {
        input = ui_numeric_input_create(config.tx_power, -9, 22, 1);
    } else {
        input->value = config.tx_power;
    }
}

void screen_lora_txpower_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!input) screen_lora_txpower_init();
    ui_numeric_input_render(input, parent, "TX POWER", "dBm");
}

void screen_lora_txpower_navigate_down(void) {
    if (input && input->edit_mode) {
        ui_numeric_input_increment(input);
    }
}

void screen_lora_txpower_navigate_up(void) {
    if (input && input->edit_mode) {
        ui_numeric_input_decrement(input);
    }
}

void screen_lora_txpower_select(void) {
    if (!input) return;
    
    if (!input->edit_mode) {
        input->edit_mode = true;
    } else {
        lora_config_t config;
        lora_get_config(&config);
        config.tx_power = (int8_t)input->value;
        lora_set_config(&config);
        input->edit_mode = false;
        ESP_LOGI(TAG, "TX power set to %d dBm", config.tx_power);
    }
}

bool screen_lora_txpower_is_edit_mode(void) {
    return input && input->edit_mode;
}

void screen_lora_txpower_reset(void) {
    if (input) {
        free(input);
        input = NULL;
    }
}
