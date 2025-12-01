#include "bsp.h"
#include "common_types.h"
#include "display.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_log.h"

static const char *TAG = "display_ssd1306";

// Forward declaration from display.c
extern esp_err_t display_panel_common_init(esp_lcd_panel_handle_t panel);

esp_err_t display_ssd1306_init(display_config_t *config)
{
    ESP_LOGI(TAG, "Initializing SSD1306 OLED display");

    i2c_master_bus_handle_t i2c_bus = bsp_get_i2c_bus();
    if (!i2c_bus) {
        ESP_LOGE(TAG, "Failed to get I2C bus from BSP");
        return ESP_FAIL;
    }

    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr            = DISPLAY_SSD1306_I2C_ADDR,
        .scl_speed_hz        = DISPLAY_SSD1306_I2C_SPEED,
        .control_phase_bytes = I2C_CONTROL_PHASE_BYTES,
        .dc_bit_offset       = I2C_DC_BIT_OFFSET,
        .lcd_cmd_bits        = LCD_CMD_BITS,
        .lcd_param_bits      = LCD_PARAM_BITS,
    };

    esp_err_t ret = esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &config->io_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .bits_per_pixel = DISPLAY_BITS_PER_PIXEL_MONO,
    };

    ret = esp_lcd_new_panel_ssd1306(config->io_handle, &panel_config, &config->panel);
    if (ret != ESP_OK) {
        goto cleanup_io;
    }

    ret = display_panel_common_init(config->panel);
    if (ret != ESP_OK) {
        goto cleanup_panel;
    }

    ESP_ERROR_CHECK(esp_lcd_panel_mirror(config->panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(config->panel, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(config->panel, true));

    display_set_dimensions(config, DISPLAY_SSD1306_WIDTH, DISPLAY_SSD1306_HEIGHT);
    config->epaper_state = NULL;

    ESP_LOGI(TAG, "SSD1306 initialized: %dx%d", config->width, config->height);
    return ESP_OK;

cleanup_panel:
    esp_lcd_panel_del(config->panel);
cleanup_io:
    esp_lcd_panel_io_del(config->io_handle);
    return ret;
}

esp_err_t display_ssd1306_sleep(display_config_t *config)
{
    VALIDATE_CONFIG(config);
    return esp_lcd_panel_disp_on_off(config->panel, false);
}

esp_err_t display_ssd1306_wake(display_config_t *config)
{
    VALIDATE_CONFIG(config);
    return esp_lcd_panel_disp_on_off(config->panel, true);
}

esp_err_t display_ssd1306_set_contrast(display_config_t *config, uint8_t contrast)
{
    VALIDATE_CONFIG(config);
    VALIDATE_ARG(config->io_handle);

    return esp_lcd_panel_io_tx_param(config->io_handle, DISPLAY_SSD1306_CMD_CONTRAST, (uint8_t[]){contrast}, 1);
}
