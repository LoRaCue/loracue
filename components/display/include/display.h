#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "lvgl.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

// Validation macros
#define VALIDATE_ARG(arg) do { \
    if (!(arg)) return ESP_ERR_INVALID_ARG; \
} while(0)

#define VALIDATE_CONFIG(cfg) do { \
    if (!(cfg) || !(cfg)->panel) return ESP_ERR_INVALID_ARG; \
} while(0)

// Display hardware constants
#define DISPLAY_SSD1306_WIDTH       128
#define DISPLAY_SSD1306_HEIGHT      64
#define DISPLAY_SSD1306_I2C_ADDR    0x3C
#define DISPLAY_SSD1306_I2C_SPEED   400000
#define DISPLAY_SSD1306_CMD_CONTRAST 0x81

#define DISPLAY_SSD1681_WIDTH       250
#define DISPLAY_SSD1681_HEIGHT      122
#define DISPLAY_SSD1681_SPI_SPEED   (4 * 1000 * 1000)

#define EPAPER_PARTIAL_REFRESH_CYCLE 10

// SPI transfer sizes
#define SPI_TRANSFER_SIZE_LORA      256
#define SPI_TRANSFER_SIZE_EPAPER    4096

// BSP stub constants
#define BSP_STUB_BATTERY_VOLTAGE    4.2f
#define BSP_STUB_SERIAL_PREFIX      "STUB"

// I2C/SPI configuration constants
#define LCD_CMD_BITS                8
#define LCD_PARAM_BITS              8
#define I2C_CONTROL_PHASE_BYTES     1
#define I2C_DC_BIT_OFFSET           6
#define SPI_MODE_DEFAULT            0
#define SPI_QUEUE_SIZE_DEFAULT      1

// Display panel constants
#define DISPLAY_BITS_PER_PIXEL_MONO 1

// Helper: Convert LVGL area (inclusive) to esp_lcd coords (exclusive)
static inline void area_to_lcd_coords(const lv_area_t *area, int *x1, int *y1, int *x2, int *y2) {
    *x1 = area->x1;
    *y1 = area->y1;
    *x2 = area->x2 + 1;
    *y2 = area->y2 + 1;
}

// Board type helper macro
#if defined(CONFIG_BOARD_LILYGO_T5) || defined(CONFIG_BOARD_LILYGO_T3)
    #define IS_EPAPER_BOARD 1
#else
    #define IS_EPAPER_BOARD 0
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
 * @brief E-Paper driver state
 */
typedef struct {
    display_refresh_mode_t refresh_mode;  // cppcheck-suppress unusedStructMember
    uint8_t partial_refresh_count;        // cppcheck-suppress unusedStructMember
} epaper_state_t;

/**
 * @brief Display configuration structure
 */
typedef struct {
    display_type_t type;                  // cppcheck-suppress unusedStructMember
    int width;                            // cppcheck-suppress unusedStructMember
    int height;                           // cppcheck-suppress unusedStructMember
    esp_lcd_panel_handle_t panel;         ///< LCD panel handle
    esp_lcd_panel_io_handle_t io_handle;  ///< Panel I/O handle (for LVGL callbacks)
    epaper_state_t *epaper_state;         // cppcheck-suppress unusedStructMember
} display_config_t;

// Helper: Set display dimensions
static inline void display_set_dimensions(display_config_t *config, int width, int height) {
    config->width = width;
    config->height = height;
}

/**
 * @brief Common LVGL flush callback for all display types
 * 
 * @param disp LVGL display object
 * @param area Area to flush
 * @param px_map Pixel data
 */
void display_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

/**
 * @brief Initialize display subsystem
 * 
 * @param config Display configuration (output)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t display_init(display_config_t *config);

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
