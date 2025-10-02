#pragma once

#include "ui_config.h"
#include "menu_screen.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Draw the pairing screen
 */
void pairing_screen_draw(void);

/**
 * @brief Navigate in pairing screen
 * @param direction Navigation direction
 */
void pairing_screen_navigate(menu_direction_t direction);

/**
 * @brief Get selected pairing option
 * @return Selected option index
 */
int pairing_screen_get_selected(void);

/**
 * @brief Reset pairing screen state
 */
void pairing_screen_reset(void);

/**
 * @brief Select current pairing option
 */
void pairing_screen_select(void);

#ifdef __cplusplus
}
#endif
