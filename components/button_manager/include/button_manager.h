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

#ifdef __cplusplus
}
#endif
