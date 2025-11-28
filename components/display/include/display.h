#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display type enumeration
 */
typedef enum {
    DISPLAY_TYPE_OLED_SSD1306,  ///< OLED display (SSD1306)
    DISPLAY_TYPE_EPAPER_SSD1681 ///< E-Paper display (SSD1681)
} display_type_t;

/**
 * @brief E-Paper refresh mode
 */
typedef enum {
    DISPLAY_REFRESH_PARTIAL, ///< Fast partial refresh (~0.3s)
    DISPLAY_REFRESH_FULL     ///< Full refresh with ghosting prevention (~2-3s)
} display_refresh_mode_t;

/**
 * @brief Display configuration structure
 */
typedef struct {
    display_type_t type;           ///< Display type
    int width;                     ///< Display width in pixels
    int height;                    ///< Display height in pixels
    esp_lcd_panel_handle_t panel;  ///< LCD panel handle
    void *user_data;               ///< User data for driver-specific state
} display_config_t;

/**
 * @brief Initialize display subsystem
 * 
 * @param config Display configuration (output)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t display_init(display_config_t *config);

/**
 * @brief Get LVGL flush callback for display
 * 
 * @param config Display configuration
 * @return void* Pointer to LVGL flush callback function
 */
void *display_lvgl_flush_cb(display_config_t *config);

/**
 * @brief Set E-Paper refresh mode (E-Paper displays only)
 * 
 * @param config Display configuration
 * @param mode Refresh mode (partial or full)
 * @return esp_err_t ESP_OK on success, ESP_ERR_NOT_SUPPORTED if not E-Paper
 */
esp_err_t display_epaper_set_refresh_mode(display_config_t *config, display_refresh_mode_t mode);

/**
 * @brief Deinitialize display subsystem
 * 
 * @param config Display configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t display_deinit(display_config_t *config);

/**
 * @brief Put display into sleep mode
 */
esp_err_t display_sleep(display_config_t *config);

/**
 * @brief Wake display from sleep mode
 */
esp_err_t display_wake(display_config_t *config);

/**
 * @brief Set display brightness/contrast
 * 
 * @param config Display configuration
 * @param brightness Brightness value (0-255)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t display_set_brightness(display_config_t *config, uint8_t brightness);

#ifdef __cplusplus
}
#endif
