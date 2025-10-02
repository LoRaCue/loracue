#pragma once

#include "ui_config.h"

/**
 * @brief Draw system information screen
 */
void system_info_screen_draw(void);

/**
 * @brief Draw device information screen
 * @param status Current UI status data
 */
void device_info_screen_draw(const ui_status_t* status);

/**
 * @brief Draw battery status screen
 * @param status Current UI status data
 */
void battery_status_screen_draw(const ui_status_t* status);

/**
 * @brief Draw LoRa statistics screen
 */
void lora_stats_screen_draw(void);
