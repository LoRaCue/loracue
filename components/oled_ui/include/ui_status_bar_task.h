#pragma once

#include "esp_err.h"

/**
 * @brief Start status bar update task
 * Redraws status bar (USB, RF, battery icons) every 5 seconds
 * Only updates screens with status bar (MAIN, PC_MODE)
 */
esp_err_t ui_status_bar_task_start(void);

/**
 * @brief Stop status bar update task
 */
esp_err_t ui_status_bar_task_stop(void);
