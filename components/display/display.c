#include "display.h"
#include "common_types.h"
#include "esp_log.h"
#include "bsp.h"
#include "sdkconfig.h"

static const char *TAG = "display";

// Forward declarations
#if IS_EPAPER_BOARD
    esp_err_t display_ssd1681_init(display_config_t *config);
    void *display_ssd1681_lvgl_flush_cb(display_config_t *config);
    esp_err_t display_ssd1681_sleep(display_config_t *config);
    esp_err_t display_ssd1681_wake(display_config_t *config);
    esp_err_t display_ssd1681_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode);
#else
    esp_err_t display_ssd1306_init(display_config_t *config);
    void *display_ssd1306_lvgl_flush_cb(display_config_t *config);
    esp_err_t display_ssd1306_sleep(display_config_t *config);
    esp_err_t display_ssd1306_wake(display_config_t *config);
    esp_err_t display_ssd1306_set_brightness(display_config_t *config, uint8_t brightness);
#endif

esp_err_t display_init(display_config_t *config) {
    VALIDATE_ARG(config);

#if IS_EPAPER_BOARD
    config->type = DISPLAY_TYPE_EPAPER_SSD1681;
    return display_ssd1681_init(config);
#else
    config->type = DISPLAY_TYPE_OLED_SSD1306;
    return display_ssd1306_init(config);
#endif
}

void *display_lvgl_flush_cb(display_config_t *config) {
    if (!config) {
        return NULL;
    }

#if IS_EPAPER_BOARD
    return display_ssd1681_lvgl_flush_cb(config);
#else
    return display_ssd1306_lvgl_flush_cb(config);
#endif
}

esp_err_t display_epaper_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode) {
#if IS_EPAPER_BOARD
    VALIDATE_ARG(config);
    return display_ssd1681_set_refresh_mode(config, mode);
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t display_deinit(display_config_t *config) {
    VALIDATE_ARG(config);
    VALIDATE_ARG(config->panel);

    esp_lcd_panel_del(config->panel);
    config->panel = NULL;
    
    if (config->user_data) {
        free(config->user_data);
        config->user_data = NULL;
    }

    return ESP_OK;
}

esp_err_t display_sleep(display_config_t *config) {
    VALIDATE_ARG(config);

#if IS_EPAPER_BOARD
    return display_ssd1681_sleep(config);
#else
    return display_ssd1306_sleep(config);
#endif
}

esp_err_t display_wake(display_config_t *config) {
    VALIDATE_ARG(config);

#if IS_EPAPER_BOARD
    return display_ssd1681_wake(config);
#else
    return display_ssd1306_wake(config);
#endif
}

esp_err_t display_set_brightness(display_config_t *config, uint8_t brightness) {
#if IS_EPAPER_BOARD
    return ESP_ERR_NOT_SUPPORTED;
#else
    VALIDATE_ARG(config);
    return display_ssd1306_set_brightness(config, brightness);
#endif
}
