/**
 * @file presenter_mode_manager.h
 * @brief Presenter Mode Manager - handles input events in presenter mode
 *
 * CONTEXT: Maps button/encoder events to LoRa commands for presentation control
 * PURPOSE: Separate presenter mode business logic from UI
 */

#pragma once

#include "common_types.h"
#include "esp_err.h"
#include "input_manager.h"

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
 * @brief Handle input event in presenter mode
 *
 * @param event Type of input event (button or encoder)
 * @return ESP_OK if handled
 */
esp_err_t presenter_mode_manager_handle_input(input_event_t event);

/**
 * @brief Deinitialize presenter mode manager
 */
void presenter_mode_manager_deinit(void);

#ifdef __cplusplus
}
#endif
