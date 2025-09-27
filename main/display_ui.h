#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the display UI system
 * @return ESP_OK on success
 */
esp_err_t display_ui_init(void);

/**
 * @brief Show the boot logo
 */
void display_show_boot_logo(void);

/**
 * @brief Show the main screen with device status
 * @param device_name Name of the device
 * @param battery_voltage Current battery voltage
 * @param signal_strength Signal strength percentage (0-100)
 */
void display_show_main_screen(const char* device_name, float battery_voltage, int signal_strength);

/**
 * @brief Clear the display
 */
void display_clear(void);

/**
 * @brief Set display brightness
 * @param brightness Brightness level (0-255)
 */
void display_set_brightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif
