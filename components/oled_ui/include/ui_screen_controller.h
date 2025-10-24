#pragma once

#include "button_manager.h"
#include "oled_ui.h"
#include "ui_config.h"

/**
 * @brief Initialize screen controller
 */
void ui_screen_controller_init(void);

/**
 * @brief Set current screen
 * @param screen Screen to display
 * @param status Current UI status
 */
void ui_screen_controller_set(oled_screen_t screen, const ui_status_t *status);

/**
 * @brief Get current screen
 * @return Current screen type
 */
oled_screen_t ui_screen_controller_get_current(void);

/**
 * @brief Update current screen with new status
 * @param status Updated UI status
 */
void ui_screen_controller_update(const ui_status_t *status);

/**
 * @brief Handle button input for current screen
 * @param event Button event (short/double/long)
 */
void ui_screen_controller_handle_button(button_event_type_t event);
