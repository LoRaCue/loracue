#pragma once

#include "esp_err.h"

/**
 * @brief Start PC mode command history update task
 * Updates elapsed times in command history every 1 second
 * Only updates when current_screen == OLED_SCREEN_PC_MODE
 */
esp_err_t ui_pc_history_task_start(void);

/**
 * @brief Stop PC mode history update task
 */
esp_err_t ui_pc_history_task_stop(void);

/**
 * @brief Notify task to update immediately (called when new command received)
 */
void ui_pc_history_notify_update(void);
