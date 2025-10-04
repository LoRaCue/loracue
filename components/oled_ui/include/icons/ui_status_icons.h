#pragma once

#include "ui_config.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Draw USB icon at specified position
 * @param x X coordinate
 * @param y Y coordinate
 */
void ui_usb_draw_at(int x, int y);

/**
 * @brief Draw Bluetooth icon at specified position
 * @param x X coordinate
 * @param y Y coordinate
 * @param connected Connection state (currently unused, same icon for both states)
 */
void ui_bluetooth_draw_at(int x, int y, bool connected);

/**
 * @brief Draw battery icon with charge level
 * @param battery_level Battery charge level (0-100%)
 */
void ui_battery_draw(uint8_t battery_level);

/**
 * @brief Draw RF signal strength icon
 * @param strength Signal strength level
 */
void ui_rf_draw(signal_strength_t strength);
