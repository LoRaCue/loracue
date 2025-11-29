/**
 * @file led_manager.h
 * @brief LED Manager with solid, fading and blinking patterns
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED pattern types
 */
typedef enum {
    LED_PATTERN_OFF,   ///< LED off
    LED_PATTERN_SOLID, ///< LED solid on
    LED_PATTERN_BLINK, ///< LED blinking
    LED_PATTERN_FADE   ///< LED fading (pulsing)
} led_pattern_t;

/**
 * @brief Initialize LED manager
 * @return ESP_OK on success
 */
esp_err_t led_manager_init(void);

/**
 * @brief Set LED to solid state
 * @param on true = solid on, false = off
 * @return ESP_OK on success
 */
esp_err_t led_manager_solid(bool on);

/**
 * @brief Start LED blinking
 * @param period_ms Blink period in milliseconds (on + off time)
 * @param duty_percent Duty cycle percentage (0-100)
 * @return ESP_OK on success
 */
esp_err_t led_manager_blink(uint32_t period_ms, uint8_t duty_percent);

/**
 * @brief Start LED fading (pulsing)
 * @param period_ms Fade period in milliseconds (full cycle)
 * @return ESP_OK on success
 */
esp_err_t led_manager_fade(uint32_t period_ms);

/**
 * @brief Stop all LED patterns
 * @return ESP_OK on success
 */
esp_err_t led_manager_stop(void);

/**
 * @brief Get current LED pattern
 * @return Current pattern type
 */
led_pattern_t led_manager_get_pattern(void);

/**
 * @brief Set button feedback override (turns LED off while button pressed)
 * @param active true = button pressed (LED off), false = button released (restore pattern)
 * @return ESP_OK on success
 */
esp_err_t led_manager_button_feedback(bool active);

#ifdef __cplusplus
}
#endif
