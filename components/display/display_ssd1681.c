#include "display.h"
#include "esp_log.h"
#include "esp_lcd_panel_ssd1681.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "bsp.h"

static const char *TAG = "display_ssd1681";

// Forward declaration from display.c
extern esp_err_t display_panel_common_init(esp_lcd_panel_handle_t panel);

esp_err_t display_ssd1681_init(display_config_t *config) {
    ESP_LOGI(TAG, "Initializing SSD1681 E-Paper display");

    spi_device_handle_t spi_device = bsp_get_spi_device();
    if (!spi_device) {
        ESP_LOGE(TAG, "Failed to get SPI device from BSP");
        return ESP_FAIL;
    }

    const bsp_epaper_pins_t *pins = bsp_get_epaper_pins();
    if (!pins) {
        ESP_LOGE(TAG, "Failed to get E-Paper pins from BSP");
        return ESP_FAIL;
    }

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = pins->dc,
        .cs_gpio_num = pins->cs,
        .pclk_hz = DISPLAY_SSD1681_SPI_SPEED,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = SPI_MODE,
    };
    
    esp_err_t ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)spi_device, &io_config, &config->io_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = pins->rst,
        .bits_per_pixel = 1,
    };
    
    ret = esp_lcd_new_panel_ssd1681(config->io_handle, &panel_config, &config->panel);
    if (ret != ESP_OK) {
        goto cleanup_io;
    }
    
    ret = display_panel_common_init(config->panel);
    if (ret != ESP_OK) {
        goto cleanup_panel;
    }

    // Allocate E-Paper state
    config->epaper_state = calloc(1, sizeof(epaper_state_t));
    if (!config->epaper_state) {
        ESP_LOGE(TAG, "Failed to allocate E-Paper state");
        ret = ESP_ERR_NO_MEM;
        goto cleanup_panel;
    }
    config->epaper_state->refresh_mode = DISPLAY_REFRESH_PARTIAL;

    config->width = DISPLAY_SSD1681_WIDTH;
    config->height = DISPLAY_SSD1681_HEIGHT;

    ESP_LOGI(TAG, "SSD1681 initialized: %dx%d", config->width, config->height);
    return ESP_OK;

cleanup_panel:
    esp_lcd_panel_del(config->panel);
cleanup_io:
    esp_lcd_panel_io_del(config->io_handle);
    return ret;
}

esp_err_t display_ssd1681_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode) {
    VALIDATE_CONFIG(config);
    VALIDATE_ARG(config->epaper_state);

    config->epaper_state->refresh_mode = mode;
    
    if (mode == DISPLAY_REFRESH_FULL) {
        config->epaper_state->partial_refresh_count = 0;
    }

    ESP_LOGI(TAG, "Refresh mode set to: %s", 
             mode == DISPLAY_REFRESH_PARTIAL ? "PARTIAL" : "FULL");
    
    return ESP_OK;
}

esp_err_t display_ssd1681_sleep(const display_config_t *config) {
    // E-Paper doesn't need sleep mode - it retains image without power
    return ESP_OK;
}

esp_err_t display_ssd1681_wake(const display_config_t *config) {
    // E-Paper doesn't need wake - always ready
    return ESP_OK;
}
