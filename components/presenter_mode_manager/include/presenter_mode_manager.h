/**
 * @file presenter_mode_manager.h
 * @brief Presenter Mode Manager - handles button events in presenter mode
 *
 * CONTEXT: Maps button presses to LoRa commands for presentation control
 * PURPOSE: Separate presenter mode business logic from UI
 */

#pragma once

#include "common_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize presenter mode manager
 *
 * @return ESP_OK on success
 */
esp_err_t presenter_mode_manager_init(void);

/**
 * @brief Handle button event in presenter mode
 *
 * @param button_type Type of button event
 * @return ESP_OK if handled
 */
esp_err_t presenter_mode_manager_handle_button(button_event_type_t button_type);

/**
 * @brief Deinitialize presenter mode manager
 */
void presenter_mode_manager_deinit(void);

#ifdef __cplusplus
}
#endif
