#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL core system
 *
 * Initializes LVGL library and port. Display-specific implementations
 * should call this first, then initialize their own display drivers.
 *
 * @return ESP_OK on success
 */
esp_err_t ui_lvgl_init(void);

/**
 * @brief Deinitialize LVGL core system
 *
 * @return ESP_OK on success
 */
esp_err_t ui_lvgl_deinit(void);

/**
 * @brief Lock LVGL for thread-safe access
 */
void ui_lvgl_lock(void);

/**
 * @brief Unlock LVGL after thread-safe access
 */
void ui_lvgl_unlock(void);

/**
 * @brief Get LVGL display object
 *
 * @return lv_display_t* Display object or NULL if not set
 */
lv_display_t *ui_lvgl_get_display(void);

/**
 * @brief Set LVGL display object
 *
 * @param disp Display object to set
 */
void ui_lvgl_set_display(lv_display_t *disp);

/**
 * @brief Get LVGL input device object
 *
 * @return lv_indev_t* Input device object or NULL if not set
 */
lv_indev_t *ui_lvgl_get_indev(void);

/**
 * @brief Set LVGL input device object
 *
 * @param dev Input device object to set
 */
void ui_lvgl_set_indev(lv_indev_t *dev);

#ifdef __cplusplus
}
#endif
