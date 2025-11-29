#pragma once

#include "lvgl.h"
#include "display.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL display port
 * 
 * @return lv_display_t* Display object or NULL on error
 */
lv_display_t *lv_port_disp_init(void);

/**
 * @brief Deinitialize LVGL display port
 */
void lv_port_disp_deinit(void);

/**
 * @brief Get display configuration
 * @return Display configuration or NULL if not initialized
 */
display_config_t *ui_lvgl_get_display_config(void);

/**
 * @brief Safe wrapper: Set display brightness
 * @param brightness Brightness value (0-255)
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if display not initialized
 */
esp_err_t display_safe_set_brightness(uint8_t brightness);

/**
 * @brief Safe wrapper: Put display to sleep
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if display not initialized
 */
esp_err_t display_safe_sleep(void);

/**
 * @brief Safe wrapper: Wake display from sleep
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if display not initialized
 */
esp_err_t display_safe_wake(void);

#ifdef __cplusplus
}
#endif
