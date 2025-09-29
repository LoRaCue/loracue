/**
 * @file oled_ui.h
 * @brief OLED User Interface using u8g2 graphics library
 * 
 * CONTEXT: LoRaCue enterprise presentation clicker OLED display
 * HARDWARE: SH1106 (Heltec V3) / SSD1306 (Wokwi) 128x64 OLED
 * PURPOSE: Rich graphics UI with icons, fonts, and animations
 */

#pragma once

#include "esp_err.h"
#include "u8g2.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OLED UI screen types
 */
typedef enum {
    OLED_SCREEN_BOOT,       ///< Boot/startup screen
    OLED_SCREEN_MAIN,       ///< Main status screen
    OLED_SCREEN_MENU,       ///< Settings menu
    OLED_SCREEN_PAIRING,    ///< Device pairing screen
    OLED_SCREEN_ERROR,      ///< Error display screen
} oled_screen_t;

/**
 * @brief Device status for display
 */
typedef struct {
    uint8_t battery_level;      ///< Battery percentage (0-100)
    bool battery_charging;      ///< Battery charging status
    bool lora_connected;        ///< LoRa connection status
    uint8_t lora_signal;        ///< LoRa signal strength (0-100)
    bool usb_connected;         ///< USB connection status
    uint16_t device_id;         ///< Device ID
    char device_name[32];       ///< Device name
} oled_status_t;

/**
 * @brief Initialize OLED UI system
 * 
 * Sets up u8g2 graphics library and initializes display.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t oled_ui_init(void);

/**
 * @brief Update device status and refresh display
 * 
 * @param status Device status structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t oled_ui_update_status(const oled_status_t *status);

/**
 * @brief Switch to different screen
 * 
 * @param screen Screen type to display
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t oled_ui_set_screen(oled_screen_t screen);

/**
 * @brief Show message on display
 * 
 * @param title Message title
 * @param message Message text
 * @param timeout_ms Auto-clear timeout (0 = no timeout)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t oled_ui_show_message(const char *title, const char *message, uint32_t timeout_ms);

/**
 * @brief Clear display
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t oled_ui_clear(void);

/**
 * @brief Get u8g2 instance for custom drawing
 * 
 * @return Pointer to u8g2 instance
 */
u8g2_t* oled_ui_get_u8g2(void);

#ifdef __cplusplus
}
#endif
