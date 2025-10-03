#pragma once

#include <stdbool.h>

/**
 * @brief Draw Bluetooth icon at specified position
 * @param x X position
 * @param y Y position
 * @param connected Whether Bluetooth is connected
 */
void ui_bluetooth_draw_at(int x, int y, bool connected);
