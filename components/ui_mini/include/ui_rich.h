/**
 * @file ui_rich.h
 * @brief Stub header for UI_MINI builds (prevents compilation errors)
 */

#pragma once

#ifndef CONFIG_UI_RICH

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Stub functions (no-op for UI_MINI builds)
static inline esp_err_t ui_rich_init(void) { return ESP_OK; }

#endif // !CONFIG_UI_RICH
