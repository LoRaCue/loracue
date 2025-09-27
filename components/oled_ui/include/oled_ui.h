/**
 * @file oled_ui.h
 * @brief OLED User Interface with state machine and menu system
 * 
 * CONTEXT: Ticket 4.1 - OLED State Machine
 * PURPOSE: Two-button navigation with hierarchical menu system
 * HARDWARE: SH1106 OLED display via BSP I2C interface
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UI states for state machine
 */
typedef enum {
    UI_STATE_MAIN,          ///< Main status screen
    UI_STATE_MENU,          ///< Main menu
    UI_STATE_WIRELESS,      ///< Wireless settings
    UI_STATE_PAIRING,       ///< Pairing menu
    UI_STATE_PAIRING_PIN,   ///< Pairing PIN display
    UI_STATE_SYSTEM,        ///< System settings
    UI_STATE_WEB_CONFIG,    ///< Web config display
    UI_STATE_NAME_EDIT,     ///< Device name editor
    UI_STATE_BOOT_LOGO,     ///< Boot logo display
    UI_STATE_SCREENSAVER,   ///< Screensaver mode
} ui_state_t;

/**
 * @brief Button events for UI navigation
 */
typedef enum {
    UI_EVENT_NONE,          ///< No event
    UI_EVENT_PREV_SHORT,    ///< PREV button short press
    UI_EVENT_PREV_LONG,     ///< PREV button long press (>1.5s)
    UI_EVENT_NEXT_SHORT,    ///< NEXT button short press  
    UI_EVENT_NEXT_LONG,     ///< NEXT button long press (>1.5s)
    UI_EVENT_BOTH_LONG,     ///< Both buttons long press (>3s)
    UI_EVENT_TIMEOUT,       ///< Inactivity timeout
} ui_event_t;

/**
 * @brief Device status information for display
 */
typedef struct {
    char device_name[32];   ///< Device name
    float battery_voltage;  ///< Battery voltage
    int signal_strength;    ///< Signal strength (0-100%)
    bool usb_connected;     ///< USB connection status
    bool is_charging;       ///< Battery charging status
    int paired_count;       ///< Number of paired devices
} device_status_t;

/**
 * @brief Initialize OLED UI system
 * 
 * @return ESP_OK on success
 */
esp_err_t oled_ui_init(void);

/**
 * @brief Update UI with button events
 * 
 * @param event Button event to process
 * @return ESP_OK on success
 */
esp_err_t oled_ui_handle_event(ui_event_t event);

/**
 * @brief Update device status display
 * 
 * @param status Current device status
 * @return ESP_OK on success
 */
esp_err_t oled_ui_update_status(const device_status_t *status);

/**
 * @brief Get current UI state
 * 
 * @return Current UI state
 */
ui_state_t oled_ui_get_state(void);

/**
 * @brief Force UI to specific state
 * 
 * @param state Target UI state
 * @return ESP_OK on success
 */
esp_err_t oled_ui_set_state(ui_state_t state);

/**
 * @brief Show boot logo
 * 
 * @return ESP_OK on success
 */
esp_err_t oled_ui_show_boot_logo(void);

/**
 * @brief Enter screensaver mode
 * 
 * @return ESP_OK on success
 */
esp_err_t oled_ui_enter_screensaver(void);

#ifdef __cplusplus
}
#endif
