#pragma once

#include "ui_config.h"

/**
 * @brief Draw top status bar with brand and status icons
 * @param status Current UI status data
 */
void ui_status_bar_draw(const ui_status_t *status);

/**
 * @brief Draw bottom bar with device name and menu hint
 * @param status Current UI status data
 */
void ui_bottom_bar_draw(const ui_status_t *status);
