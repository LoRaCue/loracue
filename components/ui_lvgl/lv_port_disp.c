#include "lv_port_disp.h"
#include "display.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "lv_port_disp";

static display_config_t display_config;
static lv_display_t *disp = NULL;

lv_display_t *lv_port_disp_init(void) {
    // Initialize display hardware
    esp_err_t ret = display_init(&display_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret));
        return NULL;
    }

    // Create LVGL display
    disp = lv_display_create(display_config.width, display_config.height);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return NULL;
    }

    // Allocate draw buffer (1/10 of screen for mono displays)
    size_t buf_size = display_config.width * display_config.height / 10;
    void *buf = malloc(buf_size);
    
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate draw buffer");
        return NULL;
    }

    lv_display_set_buffers(disp, buf, NULL, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set flush callback from display driver
    lv_display_set_flush_cb(disp, display_lvgl_flush_cb(&display_config));
    
    // Store display config as user data
    lv_display_set_user_data(disp, &display_config);

    // Configure for monochrome displays (white on black)
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_I1);
    
    // Set default background to black
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

    ESP_LOGI(TAG, "LVGL display initialized: %dx%d", display_config.width, display_config.height);
    return disp;
}

void lv_port_disp_deinit(void) {
    if (disp) {
        void *buf = lv_display_get_buf_active(disp);
        free(buf);
        
        lv_display_delete(disp);
        disp = NULL;
    }

    display_deinit(&display_config);
}
