/**
 * @file ui_mini.h
 * @brief Mini User Interface using u8g2 graphics library
 *
 * CONTEXT: LoRaCue enterprise presentation clicker OLED display
 * HARDWARE: SSD1306 128x64 OLED
 * PURPOSE: Rich graphics UI with icons, fonts, and animations
 */

#pragma once

#include "common_types.h"
#include "general_config.h"
#include "esp_err.h"
#include "u8g2.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Minimal UI state - event-driven architecture
 */
typedef struct {
    uint8_t battery_level;
    bool battery_charging;
    bool usb_connected;
    bool ble_enabled;
    int8_t lora_rssi;
    device_mode_t current_mode;
} ui_state_t;

// Global UI state (read-only for screens)
extern ui_state_t ui_state;

/**
 * @brief OLED UI screen types
 */
typedef enum {
    OLED_SCREEN_BOOT,            ///< Boot/startup screen
    OLED_SCREEN_MAIN,            ///< Main status screen
    OLED_SCREEN_PC_MODE,         ///< PC mode receiver screen
    OLED_SCREEN_MENU,            ///< Settings menu
    OLED_SCREEN_DEVICE_MODE,     ///< Device mode selection
    OLED_SCREEN_BATTERY,         ///< Battery status
    OLED_SCREEN_LORA_SUBMENU,    ///< LoRa settings submenu
    OLED_SCREEN_LORA_SETTINGS,   ///< LoRa presets
    OLED_SCREEN_LORA_FREQUENCY,  ///< LoRa frequency
    OLED_SCREEN_LORA_SF,         ///< LoRa spreading factor
    OLED_SCREEN_LORA_BW,         ///< LoRa bandwidth
    OLED_SCREEN_LORA_CR,         ///< LoRa coding rate
    OLED_SCREEN_LORA_TXPOWER,    ///< LoRa TX power
    OLED_SCREEN_LORA_BAND,       ///< LoRa frequency band
    OLED_SCREEN_SLOT,            ///< LoRa slot selection
    OLED_SCREEN_DEVICE_PAIRING,  ///< Device pairing
    OLED_SCREEN_DEVICE_REGISTRY, ///< Device registry
    OLED_SCREEN_BRIGHTNESS,      ///< Display brightness
    OLED_SCREEN_BLUETOOTH,       ///< Bluetooth settings
    OLED_SCREEN_CONFIG_MODE,     ///< Configuration mode
    OLED_SCREEN_CONFIG_ACTIVE,   ///< Config mode active
    OLED_SCREEN_DEVICE_INFO,     ///< Device information
    OLED_SCREEN_SYSTEM_INFO,     ///< System information
    OLED_SCREEN_FACTORY_RESET,   ///< Factory reset
    OLED_SCREEN_LOW_BATTERY,     ///< Low battery warning
    OLED_SCREEN_CONNECTION_LOST, ///< Connection lost
    OLED_SCREEN_OTA_UPDATE       ///< OTA firmware update
} ui_mini_screen_t;

/**
 * @brief Button types for navigation
 */
typedef enum {
    OLED_BUTTON_PREV = 0, ///< Previous/Back button
    OLED_BUTTON_NEXT,     ///< Next/Forward button
    OLED_BUTTON_BOTH      ///< Both buttons pressed
} oled_button_t;

/**
 * @brief Initialize OLED UI system
 *
 * Sets up u8g2 graphics library and initializes display.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_mini_init(void);

/**
 * @brief Update device status and refresh display
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_mini_init(void);

/**
 * @brief Switch to different screen
 *
 * @param screen Screen type to display
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_mini_set_screen(ui_mini_screen_t screen);

/**
 * @brief Get current screen
 *
 * @return Current screen type
 */
ui_mini_screen_t ui_mini_get_screen(void);

/**
 * @brief Show message on display
 *
 * @param title Message title
 * @param message Message text
 * @param timeout_ms Auto-clear timeout (0 = no timeout)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_mini_show_message(const char *title, const char *message, uint32_t timeout_ms);

/**
 * @brief Clear display
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_mini_clear(void);

/**
 * @brief Get u8g2 instance for custom drawing
 *
 * @return Pointer to u8g2 instance
 */
u8g2_t *ui_mini_get_u8g2(void);

/**
 * @brief Try to acquire draw lock (non-blocking)
 * @return true if lock acquired, false if busy
 */
bool ui_mini_try_lock_draw(void);

/**
 * @brief Release draw lock
 */
void ui_mini_unlock_draw(void);

/**
 * @brief Turn off display (power save mode)
 * @return ESP_OK on success
 */
esp_err_t ui_mini_display_off(void);

/**
 * @brief Turn on display (wake from power save)
 * @return ESP_OK on success
 */
esp_err_t ui_mini_display_on(void);

/**
 * @brief Show OTA update screen
 * @return ESP_OK on success
 */
esp_err_t ui_mini_show_ota_update(void);

/**
 * @brief Update OTA progress
 * @param progress Progress percentage (0-100)
 * @return ESP_OK on success
 */
esp_err_t ui_mini_update_ota_progress(uint8_t progress);

#ifdef __cplusplus
}
#endif
