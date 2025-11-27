#include "display.h"
#include "esp_log.h"
#include "esp_lcd_panel_ssd1681.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "bsp.h"
#include "lvgl.h"

static const char *TAG = "display_ssd1681";

#define EPAPER_WIDTH  250
#define EPAPER_HEIGHT 122

typedef struct {
    display_refresh_mode_t refresh_mode;
    uint8_t partial_refresh_count;
} epaper_state_t;

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    display_config_t *config = lv_display_get_user_data(disp);
    if (!config || !config->panel) {
        lv_display_flush_ready(disp);
        return;
    }

    epaper_state_t *state = (epaper_state_t *)config->user_data;
    
    // Draw bitmap to display
    esp_lcd_panel_draw_bitmap(config->panel, area->x1, area->y1, 
                              area->x2 + 1, area->y2 + 1, px_map);

    // Auto-switch to full refresh every 10 partial refreshes
    if (state && state->refresh_mode == DISPLAY_REFRESH_PARTIAL) {
        state->partial_refresh_count++;
        if (state->partial_refresh_count >= 10) {
            ESP_LOGI(TAG, "Triggering full refresh to prevent ghosting");
            state->refresh_mode = DISPLAY_REFRESH_FULL;
            state->partial_refresh_count = 0;
        }
    }
    
    lv_display_flush_ready(disp);
}

esp_err_t display_ssd1681_init(display_config_t *config) {
    ESP_LOGI(TAG, "Initializing SSD1681 E-Paper display");

    // Get SPI bus handle from BSP
    spi_device_handle_t spi_device = bsp_get_spi_device();
    if (!spi_device) {
        ESP_LOGE(TAG, "Failed to get SPI device from BSP");
        return ESP_FAIL;
    }

    // Create SPI panel IO
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = bsp_get_epaper_dc_pin(),
        .cs_gpio_num = bsp_get_epaper_cs_pin(),
        .pclk_hz = 4 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)spi_device, &io_config, &io_handle));

    // Create SSD1681 panel
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = bsp_get_epaper_rst_pin(),
        .bits_per_pixel = 1,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1681(io_handle, &panel_config, &config->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(config->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(config->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(config->panel, true));

    // Allocate E-Paper state
    epaper_state_t *state = calloc(1, sizeof(epaper_state_t));
    if (!state) {
        ESP_LOGE(TAG, "Failed to allocate E-Paper state");
        return ESP_ERR_NO_MEM;
    }
    state->refresh_mode = DISPLAY_REFRESH_PARTIAL;
    state->partial_refresh_count = 0;

    config->width = EPAPER_WIDTH;
    config->height = EPAPER_HEIGHT;
    config->user_data = state;

    ESP_LOGI(TAG, "SSD1681 initialized: %dx%d", config->width, config->height);
    return ESP_OK;
}

void *display_ssd1681_lvgl_flush_cb(display_config_t *config) {
    return (void *)lvgl_flush_cb;
}

esp_err_t display_ssd1681_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode) {
    if (!config || !config->user_data) {
        return ESP_ERR_INVALID_ARG;
    }

    epaper_state_t *state = (epaper_state_t *)config->user_data;
    state->refresh_mode = mode;
    
    if (mode == DISPLAY_REFRESH_FULL) {
        state->partial_refresh_count = 0;
    }

    ESP_LOGI(TAG, "Refresh mode set to: %s", 
             mode == DISPLAY_REFRESH_PARTIAL ? "PARTIAL" : "FULL");
    
    return ESP_OK;
}

esp_err_t display_ssd1681_sleep(display_config_t *config) {
    // E-Paper doesn't need sleep mode - it retains image without power
    return ESP_OK;
}

esp_err_t display_ssd1681_wake(display_config_t *config) {
    // E-Paper doesn't need wake - always ready
    return ESP_OK;
}
