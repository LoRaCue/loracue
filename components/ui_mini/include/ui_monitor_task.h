#pragma once

#include "esp_err.h"

/**
 * @brief Start UI monitoring task
 * @return ESP_OK on success
 */
esp_err_t ui_monitor_task_start(void);

/**
 * @brief Stop UI monitoring task
 * @return ESP_OK on success
 */
esp_err_t ui_monitor_task_stop(void);
