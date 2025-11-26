#include "lvgl.h"
#include "ui_components.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "factory_reset";
static ui_confirmation_t *confirm = NULL;

void screen_factory_reset_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    if (!confirm) {
        confirm = ui_confirmation_create();
    }
    
    ui_confirmation_render(confirm, parent, "FACTORY RESET", "Erase all data?");
}

void screen_factory_reset_check_hold(bool button_pressed) {
    if (confirm && ui_confirmation_check_hold(confirm, button_pressed, 5000)) {
        ESP_LOGI(TAG, "Factory reset confirmed - erasing NVS");
        nvs_flash_erase();
        esp_restart();
    }
}

void screen_factory_reset_reset(void) {
    if (confirm) {
        free(confirm);
        confirm = NULL;
    }
}
