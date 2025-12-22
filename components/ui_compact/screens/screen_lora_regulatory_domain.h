/**
 * @file screen_lora_regulatory_domain.h
 * @brief Regulatory domain selection screen
 */

#pragma once

#include "button_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize regulatory domain screen
 */
void screen_lora_regulatory_domain_on_enter(void);

/**
 * @brief Cleanup regulatory domain screen
 */
void screen_lora_regulatory_domain_on_exit(void);

/**
 * @brief Handle button press events
 * @param event Button event
 */
void screen_lora_regulatory_domain_on_button_press(button_event_t event);

/**
 * @brief Render regulatory domain screen
 */
void screen_lora_regulatory_domain_render(void);

#ifdef __cplusplus
}
#endif
