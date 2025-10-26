#pragma once

#include "esp_err.h"

/**
 * @brief Start data provider update task
 * Polls sensors (battery, USB, LoRa) every 5 seconds
 * Does NOT draw to screen
 */
esp_err_t ui_data_update_task_start(void);

/**
 * @brief Stop data provider update task
 */
esp_err_t ui_data_update_task_stop(void);
