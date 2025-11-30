#include "display.h"
#include "bsp.h"
#include "common_types.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "display";

// display_lvgl_flush_cb removed as it is unused (handled by esp_lvgl_port)

// Forward declarations (needed for dispatch macro)
esp_err_t display_ssd1306_init(display_config_t *config);
esp_err_t display_ssd1306_sleep(display_config_t *config);
esp_err_t display_ssd1306_wake(display_config_t *config);
esp_err_t display_ssd1306_set_contrast(display_config_t *config, uint8_t contrast);

esp_err_t display_ssd1681_init(display_config_t *config);
esp_err_t display_ssd1681_sleep(const display_config_t *config);
esp_err_t display_ssd1681_wake(const display_config_t *config);
esp_err_t display_ssd1681_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode);

// Display driver dispatch macro
#define DISPLAY_DISPATCH(func, ...)                                                                                    \
    (IS_EPAPER_BOARD ? display_ssd1681_##func(__VA_ARGS__) : display_ssd1306_##func(__VA_ARGS__))

// Common panel initialization sequence
esp_err_t display_panel_common_init(esp_lcd_panel_handle_t panel)
{
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    return esp_lcd_panel_disp_on_off(panel, true);
}

esp_err_t display_init(display_config_t *config)
{
    VALIDATE_ARG(config);

#if IS_EPAPER_BOARD
    config->type = DISPLAY_TYPE_EPAPER_SSD1681;
#else
    config->type = DISPLAY_TYPE_OLED_SSD1306;
#endif

    return DISPLAY_DISPATCH(init, config);
}

esp_err_t display_epaper_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode)
{
#if IS_EPAPER_BOARD
    VALIDATE_ARG(config);
    return display_ssd1681_set_refresh_mode(config, mode);
#else
    (void)config;
    (void)mode;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t display_deinit(display_config_t *config)
{
    VALIDATE_CONFIG(config);

    esp_lcd_panel_del(config->panel);
    config->panel = NULL;

    if (config->epaper_state) {
        free(config->epaper_state);
        config->epaper_state = NULL;
    }

    return ESP_OK;
}

esp_err_t display_sleep(display_config_t *config)
{
    VALIDATE_CONFIG(config);
    return DISPLAY_DISPATCH(sleep, config);
}

esp_err_t display_wake(display_config_t *config)
{
    VALIDATE_CONFIG(config);
    return DISPLAY_DISPATCH(wake, config);
}

esp_err_t display_set_contrast(display_config_t *config, uint8_t contrast)
{
#if IS_EPAPER_BOARD
    (void)config;
    (void)contrast;
    return ESP_ERR_NOT_SUPPORTED;
#else
    VALIDATE_CONFIG(config);
    return display_ssd1306_set_contrast(config, contrast);
#endif
}
