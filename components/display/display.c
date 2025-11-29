#include "display.h"
#include "esp_log.h"
#include "bsp.h"
#include "sdkconfig.h"

static const char *TAG = "display";

// Forward declarations
#if defined(CONFIG_BOARD_HELTEC_V3)
    esp_err_t display_ssd1306_init(display_config_t *config);
    void *display_ssd1306_lvgl_flush_cb(display_config_t *config);
    esp_err_t display_ssd1306_sleep(display_config_t *config);
    esp_err_t display_ssd1306_wake(display_config_t *config);
    esp_err_t display_ssd1306_set_brightness(display_config_t *config, uint8_t brightness);
#elif defined(CONFIG_BOARD_LILYGO_T5) || defined(CONFIG_BOARD_LILYGO_T3)
    esp_err_t display_ssd1681_init(display_config_t *config);
    void *display_ssd1681_lvgl_flush_cb(display_config_t *config);
    esp_err_t display_ssd1681_sleep(display_config_t *config);
    esp_err_t display_ssd1681_wake(display_config_t *config);
    esp_err_t display_ssd1681_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode);
#else
    // Default to SSD1306 (e.g. Wokwi or fallback)
    esp_err_t display_ssd1306_init(display_config_t *config);
    void *display_ssd1306_lvgl_flush_cb(display_config_t *config);
    esp_err_t display_ssd1306_sleep(display_config_t *config);
    esp_err_t display_ssd1306_wake(display_config_t *config);
    esp_err_t display_ssd1306_set_brightness(display_config_t *config, uint8_t brightness);
#endif

esp_err_t display_init(display_config_t *config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

#if defined(CONFIG_BOARD_LILYGO_T5) || defined(CONFIG_BOARD_LILYGO_T3)
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

#if defined(CONFIG_BOARD_LILYGO_T5) || defined(CONFIG_BOARD_LILYGO_T3)
    return display_ssd1681_lvgl_flush_cb(config);
#else
    return display_ssd1306_lvgl_flush_cb(config);
#endif
}

esp_err_t display_epaper_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode) {
#if defined(CONFIG_BOARD_LILYGO_T5) || defined(CONFIG_BOARD_LILYGO_T3)
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    return display_ssd1681_set_refresh_mode(config, mode);
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
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

#if defined(CONFIG_BOARD_LILYGO_T5) || defined(CONFIG_BOARD_LILYGO_T3)
    return display_ssd1681_sleep(config);
#else
    return display_ssd1306_sleep(config);
#endif
}

esp_err_t display_wake(display_config_t *config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

#if defined(CONFIG_BOARD_LILYGO_T5) || defined(CONFIG_BOARD_LILYGO_T3)
    return display_ssd1681_wake(config);
#else
    return display_ssd1306_wake(config);
#endif
}

esp_err_t display_set_brightness(display_config_t *config, uint8_t brightness) {
#if defined(CONFIG_BOARD_LILYGO_T5) || defined(CONFIG_BOARD_LILYGO_T3)
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    return display_ssd1306_set_brightness(config, brightness);
#endif
}
