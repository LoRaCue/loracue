#include "lv_port_disp.h"
#include "display.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include <stdlib.h>

static const char *TAG = "lv_port_disp";

static display_config_t display_config;
static lv_display_t *disp = NULL;

display_config_t *ui_lvgl_get_display_config(void) {
    return &display_config;
}

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, 
                                    esp_lcd_panel_io_event_data_t *edata, 
                                    void *user_ctx) {
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lvgl_port_flush_ready(disp);
    return false;
}

lv_display_t *lv_port_disp_init(void) {
    // Initialize display hardware
    esp_err_t ret = display_init(&display_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret));
        return NULL;
    }

    // Configure LVGL port for monochrome display
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = display_config.io_handle,
        .panel_handle = display_config.panel,
        .buffer_size = display_config.width * display_config.height,
        .double_buffer = true,
        .hres = display_config.width,
        .vres = display_config.height,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = true,
        }
    };
    
    disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to add LVGL display");
        return NULL;
    }

    // Register callback for flush ready notification
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    esp_lcd_panel_io_register_event_callbacks(display_config.io_handle, &cbs, disp);

    ESP_LOGI(TAG, "LVGL display initialized: %dx%d (monochrome)", 
             display_config.width, display_config.height);
    return disp;
}

void lv_port_disp_deinit(void) {
    if (disp) {
        lvgl_port_remove_disp(disp);
        disp = NULL;
    }
    display_deinit(&display_config);
}

// Safe wrapper implementations using macro
esp_err_t display_safe_set_brightness(uint8_t brightness) {
    display_config_t *cfg = ui_lvgl_get_display_config();
    return cfg ? display_set_brightness(cfg, brightness) : ESP_ERR_INVALID_STATE;
}

esp_err_t display_safe_sleep(void) {
    display_config_t *cfg = ui_lvgl_get_display_config();
    return cfg ? display_sleep(cfg) : ESP_ERR_INVALID_STATE;
}

esp_err_t display_safe_wake(void) {
    display_config_t *cfg = ui_lvgl_get_display_config();
    return cfg ? display_wake(cfg) : ESP_ERR_INVALID_STATE;
}
