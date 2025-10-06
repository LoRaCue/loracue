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
    OLED_SCREEN_BOOT,            ///< Boot/startup screen
    OLED_SCREEN_MAIN,            ///< Main status screen
    OLED_SCREEN_PC_MODE,         ///< PC mode receiver screen
    OLED_SCREEN_MENU,            ///< Settings menu
    OLED_SCREEN_DEVICE_MODE,     ///< Device mode selection
    OLED_SCREEN_BATTERY,         ///< Battery status
    OLED_SCREEN_LORA_SETTINGS,   ///< LoRa configuration
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
    OLED_SCREEN_CONNECTION_LOST  ///< Connection lost
} oled_screen_t;

/**
 * @brief Button types for navigation
 */
typedef enum {
    OLED_BUTTON_PREV = 0, ///< Previous/Back button
    OLED_BUTTON_NEXT,     ///< Next/Forward button
    OLED_BUTTON_BOTH      ///< Both buttons pressed
} oled_button_t;

/**
 * @brief Active presenter info for PC mode
 */
typedef struct {
    uint16_t device_id;
    int16_t rssi;
    uint32_t command_count;
} active_presenter_info_t;

/**
 * @brief Command history entry for PC mode
 */
typedef struct {
    uint32_t timestamp_ms;
    uint16_t device_id;
    char device_name[16];
    char command[8];
} command_history_entry_t;

/**
 * @brief Device status for display
 */
typedef struct {
    uint8_t battery_level; ///< Battery percentage (0-100)
    bool battery_charging; ///< Battery charging status
    bool lora_connected;   ///< LoRa connection status
    uint8_t lora_signal;   ///< LoRa signal strength (0-100)
    bool usb_connected;    ///< USB connection status
    uint16_t device_id;    ///< Device ID
    char device_name[32];  ///< Device name
    char last_command[16]; ///< Last received command (PC mode)
    uint8_t active_presenter_count;
    active_presenter_info_t active_presenters[4];
    command_history_entry_t command_history[4];
    uint8_t command_history_count;
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
 * @brief Handle button press events
 *
 * @param button Button that was pressed
 * @param long_press True if long press detected
 */
void oled_ui_handle_button(oled_button_t button, bool long_press);

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
 * @brief Get current screen
 *
 * @return Current screen type
 */
oled_screen_t oled_ui_get_screen(void);

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
u8g2_t *oled_ui_get_u8g2(void);

/**
 * @brief Try to acquire draw lock (non-blocking)
 * @return true if lock acquired, false if busy
 */
bool oled_ui_try_lock_draw(void);

/**
 * @brief Release draw lock
 */
void oled_ui_unlock_draw(void);

#ifdef __cplusplus
}
#endif
