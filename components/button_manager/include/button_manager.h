/**
 * @file button_manager.h
 * @brief Button manager for UI event generation
 *
 * CONTEXT: Ticket 4.1 - Button event processing for OLED UI
 * PURPOSE: Convert button presses to UI events with timing
 */

#pragma once

#include "esp_err.h"
#include "oled_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Button event types for one-button UI
 */
typedef enum {
    BUTTON_EVENT_SHORT = 0,  // <500ms
    BUTTON_EVENT_DOUBLE,     // 2 clicks <500ms apart
    BUTTON_EVENT_LONG        // >3s
} button_event_type_t;

/**
 * @brief Button event callback function type
 */
typedef void (*button_event_callback_t)(button_event_type_t event, void *arg);

/**
 * @brief Initialize button manager
 *
 * @return ESP_OK on success
 */
esp_err_t button_manager_init(void);

/**
 * @brief Start button manager task
 *
 * @return ESP_OK on success
 */
esp_err_t button_manager_start(void);

/**
 * @brief Stop button manager task
 *
 * @return ESP_OK on success
 */
esp_err_t button_manager_stop(void);

/**
 * @brief Register callback for button events
 *
 * @param callback Callback function
 * @param arg User argument passed to callback
 * @return ESP_OK on success
 */
esp_err_t button_manager_register_callback(button_event_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif
