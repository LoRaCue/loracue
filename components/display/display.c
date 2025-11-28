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

esp_err_t display_sleep(display_config_t *config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    extern esp_err_t display_ssd1306_sleep(display_config_t *config);
    extern esp_err_t display_ssd1681_sleep(display_config_t *config);

    switch (config->type) {
        case DISPLAY_TYPE_OLED_SSD1306:
            return display_ssd1306_sleep(config);
        case DISPLAY_TYPE_EPAPER_SSD1681:
            return display_ssd1681_sleep(config);
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
}

esp_err_t display_wake(display_config_t *config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    extern esp_err_t display_ssd1306_wake(display_config_t *config);
    extern esp_err_t display_ssd1681_wake(display_config_t *config);

    switch (config->type) {
        case DISPLAY_TYPE_OLED_SSD1306:
            return display_ssd1306_wake(config);
        case DISPLAY_TYPE_EPAPER_SSD1681:
            return display_ssd1681_wake(config);
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
}

esp_err_t display_set_brightness(display_config_t *config, uint8_t brightness) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    extern esp_err_t display_ssd1306_set_brightness(display_config_t *config, uint8_t brightness);

    switch (config->type) {
        case DISPLAY_TYPE_OLED_SSD1306:
            return display_ssd1306_set_brightness(config, brightness);
        case DISPLAY_TYPE_EPAPER_SSD1681:
            return ESP_ERR_NOT_SUPPORTED;  // E-Paper doesn't support brightness
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
}
