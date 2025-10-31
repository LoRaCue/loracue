#pragma once

#include "menu_screen.h"
#include "ui_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Draw the device registry screen
 */
void device_registry_screen_draw(void);

/**
 * @brief Navigate in device registry screen
 * @param direction Navigation direction
 */
void device_registry_screen_navigate(menu_direction_t direction);

/**
 * @brief Get selected device index
 * @return Selected device index
 */
int device_registry_screen_get_selected(void);

/**
 * @brief Reset device registry screen state
 */
void device_registry_screen_reset(void);

#ifdef __cplusplus
}
#endif
