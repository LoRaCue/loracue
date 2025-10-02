#pragma once

#include "ui_config.h"

/**
 * @brief Draw RF signal strength bars
 * @param strength Signal strength level (SIGNAL_NONE to SIGNAL_STRONG)
 */
void ui_rf_draw(signal_strength_t strength);
