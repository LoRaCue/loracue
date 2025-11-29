#include "display.h"
#include "esp_log.h"
#include "esp_lcd_panel_ssd1681.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "bsp.h"
#include "lvgl.h"

static const char *TAG = "display_ssd1681";

// Forward declaration from display.c
extern esp_err_t display_panel_common_init(esp_lcd_panel_handle_t panel);

// Helper: Convert LVGL area (inclusive) to esp_lcd coords (exclusive)
static inline void area_to_lcd_coords(const lv_area_t *area, int *x1, int *y1, int *x2, int *y2) {
    *x1 = area->x1;
    *y1 = area->y1;
    *x2 = area->x2 + 1;
    *y2 = area->y2 + 1;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    display_config_t *config = lv_display_get_user_data(disp);
    if (!config || !config->panel) {
        lv_display_flush_ready(disp);
        return;
    }

    int x1, y1, x2, y2;
    area_to_lcd_coords(area, &x1, &y1, &x2, &y2);
    esp_lcd_panel_draw_bitmap(config->panel, x1, y1, x2, y2, px_map);

    // Auto-switch to full refresh every N partial refreshes
    if (config->epaper_state && config->epaper_state->refresh_mode == DISPLAY_REFRESH_PARTIAL) {
        config->epaper_state->partial_refresh_count++;
        if (config->epaper_state->partial_refresh_count >= EPAPER_PARTIAL_REFRESH_CYCLE) {
            ESP_LOGI(TAG, "Triggering full refresh to prevent ghosting");
            config->epaper_state->refresh_mode = DISPLAY_REFRESH_FULL;
            config->epaper_state->partial_refresh_count = 0;
        }
    }
    
    lv_display_flush_ready(disp);
}

esp_err_t display_ssd1681_init(display_config_t *config) {
    ESP_LOGI(TAG, "Initializing SSD1681 E-Paper display");

    spi_device_handle_t spi_device = bsp_get_spi_device();
    if (!spi_device) {
        ESP_LOGE(TAG, "Failed to get SPI device from BSP");
        return ESP_FAIL;
    }

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = bsp_get_epaper_dc_pin(),
        .cs_gpio_num = bsp_get_epaper_cs_pin(),
        .pclk_hz = DISPLAY_SSD1681_SPI_SPEED,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)spi_device, &io_config, &config->io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = bsp_get_epaper_rst_pin(),
        .bits_per_pixel = 1,
    };
    
    esp_err_t ret = esp_lcd_new_panel_ssd1681(config->io_handle, &panel_config, &config->panel);
    if (ret != ESP_OK) {
        esp_lcd_panel_io_del(config->io_handle);
        return ret;
    }
    
    ret = display_panel_common_init(config->panel);
    if (ret != ESP_OK) {
        esp_lcd_panel_del(config->panel);
        esp_lcd_panel_io_del(config->io_handle);
        return ret;
    }

    // Allocate E-Paper state
    config->epaper_state = calloc(1, sizeof(epaper_state_t));
    if (!config->epaper_state) {
        ESP_LOGE(TAG, "Failed to allocate E-Paper state");
        esp_lcd_panel_del(config->panel);
        esp_lcd_panel_io_del(config->io_handle);
        return ESP_ERR_NO_MEM;
    }
    config->epaper_state->refresh_mode = DISPLAY_REFRESH_PARTIAL;

    config->width = DISPLAY_SSD1681_WIDTH;
    config->height = DISPLAY_SSD1681_HEIGHT;

    ESP_LOGI(TAG, "SSD1681 initialized: %dx%d", config->width, config->height);
    return ESP_OK;
}

void *display_ssd1681_lvgl_flush_cb(display_config_t *config) {
    return (void *)lvgl_flush_cb;
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

esp_err_t display_ssd1681_sleep(display_config_t *config) {
    // E-Paper doesn't need sleep mode - it retains image without power
    return ESP_OK;
}

esp_err_t display_ssd1681_wake(display_config_t *config) {
    // E-Paper doesn't need wake - always ready
    return ESP_OK;
}
