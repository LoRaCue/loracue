#include "display.h"
#include "esp_log.h"
#include "bsp.h"

static const char *TAG = "display";

// Forward declarations for driver-specific functions
esp_err_t display_ssd1306_init(display_config_t *config);
esp_err_t display_ssd1681_init(display_config_t *config);
void *display_ssd1306_lvgl_flush_cb(display_config_t *config);
void *display_ssd1681_lvgl_flush_cb(display_config_t *config);

esp_err_t display_init(display_config_t *config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Get display type from BSP
    bsp_display_type_t bsp_type = bsp_get_display_type();
    
    switch (bsp_type) {
        case BSP_DISPLAY_TYPE_OLED_SSD1306:
            config->type = DISPLAY_TYPE_OLED_SSD1306;
            return display_ssd1306_init(config);
            
        case BSP_DISPLAY_TYPE_EPAPER_SSD1681:
            config->type = DISPLAY_TYPE_EPAPER_SSD1681;
            return display_ssd1681_init(config);
            
        default:
            ESP_LOGE(TAG, "Unsupported display type: %d", bsp_type);
            return ESP_ERR_NOT_SUPPORTED;
    }
}

void *display_lvgl_flush_cb(display_config_t *config) {
    if (!config) {
        return NULL;
    }

    switch (config->type) {
        case DISPLAY_TYPE_OLED_SSD1306:
            return display_ssd1306_lvgl_flush_cb(config);
            
        case DISPLAY_TYPE_EPAPER_SSD1681:
            return display_ssd1681_lvgl_flush_cb(config);
            
        default:
            return NULL;
    }
}

esp_err_t display_epaper_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->type != DISPLAY_TYPE_EPAPER_SSD1681) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    // E-Paper specific implementation in display_ssd1681.c
    extern esp_err_t display_ssd1681_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode);
    return display_ssd1681_set_refresh_mode(config, mode);
}

esp_err_t display_deinit(display_config_t *config) {
    if (!config || !config->panel) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_lcd_panel_del(config->panel);
    config->panel = NULL;
    
    if (config->user_data) {
        free(config->user_data);
        config->user_data = NULL;
    }

    return ESP_OK;
}
