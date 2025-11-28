#include "display.h"
#include "esp_log.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "bsp.h"
#include "lvgl.h"

static const char *TAG = "display_ssd1306";

#define OLED_WIDTH  128
#define OLED_HEIGHT 64

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    display_config_t *config = lv_display_get_user_data(disp);
    if (!config || !config->panel) {
        ESP_LOGE(TAG, "Flush called but no config/panel!");
        lv_display_flush_ready(disp);
        return;
    }

    ESP_LOGI(TAG, "Flush area: (%d,%d) to (%d,%d), first bytes: %02x %02x %02x %02x", 
             area->x1, area->y1, area->x2, area->y2,
             px_map[0], px_map[1], px_map[2], px_map[3]);

    // Draw bitmap to display
    esp_err_t ret = esp_lcd_panel_draw_bitmap(config->panel, area->x1, area->y1, 
                                               area->x2 + 1, area->y2 + 1, px_map);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Draw failed: %s", esp_err_to_name(ret));
    }
    
    lv_display_flush_ready(disp);
}

esp_err_t display_ssd1306_init(display_config_t *config) {
    ESP_LOGI(TAG, "Initializing SSD1306 OLED display");

    // Get I2C bus handle from BSP
    i2c_master_bus_handle_t i2c_bus = bsp_get_i2c_bus();
    if (!i2c_bus) {
        ESP_LOGE(TAG, "Failed to get I2C bus from BSP");
        return ESP_FAIL;
    }

    // Create I2C panel IO
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = 0x3C,
        .scl_speed_hz = 400000,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    // Create SSD1306 panel
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .bits_per_pixel = 1,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &config->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(config->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(config->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(config->panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(config->panel, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(config->panel, true));  // Invert for correct display
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(config->panel, true));

    config->width = OLED_WIDTH;
    config->height = OLED_HEIGHT;
    config->user_data = io_handle;  // Store io_handle for lvgl_port

    ESP_LOGI(TAG, "SSD1306 initialized: %dx%d", config->width, config->height);
    return ESP_OK;
}

void *display_ssd1306_lvgl_flush_cb(display_config_t *config) {
    return (void *)lvgl_flush_cb;
}

esp_err_t display_ssd1306_sleep(display_config_t *config) {
    if (!config || !config->panel) {
        return ESP_ERR_INVALID_ARG;
    }
    return esp_lcd_panel_disp_on_off(config->panel, false);
}

esp_err_t display_ssd1306_wake(display_config_t *config) {
    if (!config || !config->panel) {
        return ESP_ERR_INVALID_ARG;
    }
    return esp_lcd_panel_disp_on_off(config->panel, true);
}

esp_err_t display_ssd1306_set_brightness(display_config_t *config, uint8_t brightness) {
    if (!config || !config->user_data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // SSD1306 uses contrast control (0x81 command)
    esp_lcd_panel_io_handle_t io_handle = (esp_lcd_panel_io_handle_t)config->user_data;
    return esp_lcd_panel_io_tx_param(io_handle, 0x81, (uint8_t[]){brightness}, 1);
}
