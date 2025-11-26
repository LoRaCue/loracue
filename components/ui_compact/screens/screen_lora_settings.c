#include "lvgl.h"
#include "ui_components.h"
#include "lora_driver.h"
#include "esp_log.h"

// Bandwidth screen
static const char *TAG_BW = "lora_bw";
static ui_dropdown_t *bw_dropdown = NULL;
static const char *bw_options[] = {"125 kHz", "250 kHz", "500 kHz"};
#define BW_COUNT 3

void screen_lora_bw_init(void) {
    uint32_t bw = lora_get_bandwidth();
    int index = (bw == 125000) ? 0 : (bw == 250000) ? 1 : 2;
    bw_dropdown = ui_dropdown_create(index, BW_COUNT);
}

void screen_lora_bw_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!bw_dropdown) screen_lora_bw_init();
    ui_dropdown_render(bw_dropdown, parent, "BANDWIDTH", bw_options);
}

void screen_lora_bw_navigate_down(void) {
    if (bw_dropdown) ui_dropdown_next(bw_dropdown);
}

void screen_lora_bw_select(void) {
    if (!bw_dropdown) return;
    if (bw_dropdown->edit_mode) {
        uint32_t bw = (bw_dropdown->selected_index == 0) ? 125000 : (bw_dropdown->selected_index == 1) ? 250000 : 500000;
        lora_set_bandwidth(bw);
        ESP_LOGI(TAG_BW, "BW saved: %lu Hz", bw);
        bw_dropdown->edit_mode = false;
    } else {
        bw_dropdown->edit_mode = true;
    }
}

bool screen_lora_bw_is_edit_mode(void) {
    return bw_dropdown ? bw_dropdown->edit_mode : false;
}

// Coding Rate screen
static const char *TAG_CR = "lora_cr";
static ui_dropdown_t *cr_dropdown = NULL;
static const char *cr_options[] = {"4/5", "4/6", "4/7", "4/8"};
#define CR_COUNT 4

void screen_lora_cr_init(void) {
    uint8_t cr = lora_get_coding_rate();
    int index = cr - 5;  // CR 5=4/5 (index 0), 6=4/6 (index 1), etc.
    if (index < 0) index = 0;
    if (index >= CR_COUNT) index = CR_COUNT - 1;
    cr_dropdown = ui_dropdown_create(index, CR_COUNT);
}

void screen_lora_cr_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!cr_dropdown) screen_lora_cr_init();
    ui_dropdown_render(cr_dropdown, parent, "CODING RATE", cr_options);
}

void screen_lora_cr_navigate_down(void) {
    if (cr_dropdown) ui_dropdown_next(cr_dropdown);
}

void screen_lora_cr_select(void) {
    if (!cr_dropdown) return;
    if (cr_dropdown->edit_mode) {
        uint8_t cr = cr_dropdown->selected_index + 5;
        lora_set_coding_rate(cr);
        ESP_LOGI(TAG_CR, "CR saved: %d", cr);
        cr_dropdown->edit_mode = false;
    } else {
        cr_dropdown->edit_mode = true;
    }
}

bool screen_lora_cr_is_edit_mode(void) {
    return cr_dropdown ? cr_dropdown->edit_mode : false;
}

// TX Power screen
static const char *TAG_TX = "lora_tx";
static ui_numeric_input_t *tx_input = NULL;

void screen_lora_txpower_init(void) {
    int8_t power = lora_get_tx_power();
    tx_input = ui_numeric_input_create((float)power, -9.0f, 22.0f, 1.0f);
}

void screen_lora_txpower_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!tx_input) screen_lora_txpower_init();
    ui_numeric_input_render(tx_input, parent, "TX POWER", "dBm");
}

void screen_lora_txpower_increment(void) {
    if (tx_input) ui_numeric_input_increment(tx_input);
}

void screen_lora_txpower_select(void) {
    if (!tx_input) return;
    if (tx_input->edit_mode) {
        int8_t power = (int8_t)tx_input->value;
        lora_set_tx_power(power);
        ESP_LOGI(TAG_TX, "TX Power saved: %d dBm", power);
        tx_input->edit_mode = false;
    } else {
        tx_input->edit_mode = true;
    }
}

bool screen_lora_txpower_is_edit_mode(void) {
    return tx_input ? tx_input->edit_mode : false;
}

// Band screen
static const char *TAG_BAND = "lora_band";
static ui_dropdown_t *band_dropdown = NULL;
static const char *band_options[] = {"868 MHz", "915 MHz"};
#define BAND_COUNT 2

void screen_lora_band_init(void) {
    uint32_t freq = lora_get_frequency();
    int index = (freq < 900000000) ? 0 : 1;
    band_dropdown = ui_dropdown_create(index, BAND_COUNT);
}

void screen_lora_band_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    if (!band_dropdown) screen_lora_band_init();
    ui_dropdown_render(band_dropdown, parent, "BAND", band_options);
}

void screen_lora_band_navigate_down(void) {
    if (band_dropdown) ui_dropdown_next(band_dropdown);
}

void screen_lora_band_select(void) {
    if (!band_dropdown) return;
    if (band_dropdown->edit_mode) {
        uint32_t freq = (band_dropdown->selected_index == 0) ? 868000000 : 915000000;
        lora_set_frequency(freq);
        ESP_LOGI(TAG_BAND, "Band saved: %lu Hz", freq);
        band_dropdown->edit_mode = false;
    } else {
        band_dropdown->edit_mode = true;
    }
}

bool screen_lora_band_is_edit_mode(void) {
    return band_dropdown ? band_dropdown->edit_mode : false;
}
