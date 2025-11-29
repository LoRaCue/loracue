#include "display.h"
#include "esp_log.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "bsp.h"
#include "lvgl.h"

static const char *TAG = "display_ssd1306";

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
        ESP_LOGE(TAG, "Flush called but no config/panel!");
        lv_display_flush_ready(disp);
        return;
    }

    ESP_LOGI(TAG, "Flush area: (%d,%d) to (%d,%d), first bytes: %02x %02x %02x %02x", 
             area->x1, area->y1, area->x2, area->y2,
             px_map[0], px_map[1], px_map[2], px_map[3]);

    int x1, y1, x2, y2;
    area_to_lcd_coords(area, &x1, &y1, &x2, &y2);
    esp_err_t ret = esp_lcd_panel_draw_bitmap(config->panel, x1, y1, x2, y2, px_map);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Draw failed: %s", esp_err_to_name(ret));
    }
    
    lv_display_flush_ready(disp);
}

esp_err_t display_ssd1306_init(display_config_t *config) {
    ESP_LOGI(TAG, "Initializing SSD1306 OLED display");

    i2c_master_bus_handle_t i2c_bus = bsp_get_i2c_bus();
    if (!i2c_bus) {
        ESP_LOGE(TAG, "Failed to get I2C bus from BSP");
        return ESP_FAIL;
    }

    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = DISPLAY_SSD1306_I2C_ADDR,
        .scl_speed_hz = DISPLAY_SSD1306_I2C_SPEED,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &config->io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .bits_per_pixel = 1,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(config->io_handle, &panel_config, &config->panel));
    ESP_ERROR_CHECK(display_panel_common_init(config->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(config->panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(config->panel, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(config->panel, true));

    config->width = DISPLAY_SSD1306_WIDTH;
    config->height = DISPLAY_SSD1306_HEIGHT;
    config->epaper_state = NULL;

    ESP_LOGI(TAG, "SSD1306 initialized: %dx%d", config->width, config->height);
    return ESP_OK;
}

esp_err_t display_ssd1306_sleep(display_config_t *config) {
    VALIDATE_CONFIG(config);
    return esp_lcd_panel_disp_on_off(config->panel, false);
}

esp_err_t display_ssd1306_wake(display_config_t *config) {
    VALIDATE_CONFIG(config);
    return esp_lcd_panel_disp_on_off(config->panel, true);
}

esp_err_t display_ssd1306_set_brightness(display_config_t *config, uint8_t brightness) {
    VALIDATE_CONFIG(config);
    VALIDATE_ARG(config->io_handle);
    
    return esp_lcd_panel_io_tx_param(config->io_handle, DISPLAY_SSD1306_CMD_CONTRAST, 
                                     (uint8_t[]){brightness}, 1);
}
