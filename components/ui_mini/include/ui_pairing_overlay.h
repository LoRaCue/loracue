#pragma once

#include <stdint.h>

/**
 * @brief Draw Bluetooth pairing overlay with PIN
 *
 * Draws a centered overlay box with Bluetooth icon and 6-digit PIN.
 * Should be called at the end of screen draw functions (before u8g2_SendBuffer).
 *
 * @param passkey 6-digit pairing PIN to display
 */
void ui_pairing_overlay_draw(uint32_t passkey);
