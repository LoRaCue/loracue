/**
 * @file input_manager.h
 * @brief Unified input management for Alpha and Alpha+ models
 *
 * Handles button and rotary encoder inputs with hardware abstraction.
 * - Alpha: Single button (GPIO0) with short/double/long press
 * - Alpha+: Dual buttons (GPIO0, GPIO46) + rotary encoder (GPIO4-6)
 *
 * @copyright Copyright (c) 2025 LoRaCue Project
 * @license GPL-3.0
 */

#pragma once

#include "common_types.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Input event types (Alpha+ specific)
 */
typedef enum {
    INPUT_EVENT_PREV_SHORT,             ///< PREV button short press (Alpha+)
    INPUT_EVENT_PREV_LONG,              ///< PREV button long press (Alpha+)
    INPUT_EVENT_NEXT_SHORT,             ///< NEXT button short press (Alpha/Alpha+)
    INPUT_EVENT_NEXT_LONG,              ///< NEXT button long press (Alpha/Alpha+)
    INPUT_EVENT_NEXT_DOUBLE,            ///< NEXT button double press (Alpha only)
    INPUT_EVENT_ENCODER_CW,             ///< Encoder clockwise rotation (Alpha+)
    INPUT_EVENT_ENCODER_CCW,            ///< Encoder counter-clockwise rotation (Alpha+)
    INPUT_EVENT_ENCODER_BUTTON_SHORT,   ///< Encoder button short press (Alpha+)
    INPUT_EVENT_ENCODER_BUTTON_LONG,    ///< Encoder button long press (Alpha+)
} input_event_t;

/**
 * @brief Input event callback function type
 */
typedef void (*input_callback_t)(input_event_t event);

/**
 * @brief Initialize input manager
 *
 * Configures GPIOs and initializes hardware based on model:
 * - Alpha: GPIO0 with debouncing and multi-press detection
 * - Alpha+: GPIO0, GPIO46, and rotary encoder on GPIO4-6
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t input_manager_init(void);

/**
 * @brief Register callback for input events
 *
 * @param callback Function to call when input events occur
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if callback is NULL
 */
esp_err_t input_manager_register_callback(input_callback_t callback);

/**
 * @brief Start input manager task
 *
 * Begins monitoring inputs and generating events.
 * Must be called after input_manager_init().
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t input_manager_start(void);

#ifdef __cplusplus
}
#endif
