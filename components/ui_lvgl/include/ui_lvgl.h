#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL UI system
 * 
 * Initializes LVGL, display driver, and loads boot screen.
 * 
 * @return ESP_OK on success
 */
esp_err_t ui_lvgl_init(void);

/**
 * @brief Deinitialize LVGL UI system
 * 
 * @return ESP_OK on success
 */
esp_err_t ui_lvgl_deinit(void);

/**
 * @brief Get LVGL display object
 * 
 * @return lv_display_t* Display object or NULL if not initialized
 */
lv_display_t *ui_lvgl_get_display(void);

/**
 * @brief LVGL tick timer callback (called every 1ms)
 */
void ui_lvgl_tick_cb(void *arg);

#ifdef __cplusplus
}
#endif
