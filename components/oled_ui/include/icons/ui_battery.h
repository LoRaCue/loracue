#pragma once

#include <stdint.h>

/**
 * @brief Draw battery icon with charge level
 * @param battery_level Battery charge level (0-100%)
 */
void ui_battery_draw(uint8_t battery_level);
