/**
 * @file bsp.h
 * @brief Board Support Package interface for LoRaCue hardware abstraction
 *
 * CONTEXT: LoRaCue enterprise presentation clicker BSP layer
 * PURPOSE: Hardware abstraction for multiple board support
 * CURRENT: Heltec LoRa V3 implementation
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Button identifiers
 */
typedef enum {
    BSP_BUTTON_PREV = 0, ///< Previous/Back button
    BSP_BUTTON_NEXT = 1, ///< Next/Forward button
#ifdef SIMULATOR_BUILD
    BSP_BUTTON_BOTH = 2, ///< Both buttons (Wokwi simulation only)
#endif
} bsp_button_t;

/**
 * @brief Initialize the board support package
 *
 * Initializes all hardware peripherals including GPIO, ADC, and power management.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_init(void);

/**
 * @brief Initialize button GPIO pins
 *
 * Configures button pins as inputs with internal pull-ups enabled.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_init_buttons(void);

/**
 * @brief Initialize battery monitoring system
 *
 * Configures ADC and control GPIO for battery voltage measurement.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_init_battery(void);

/**
 * @brief Set status LED state
 * @param state true = LED on, false = LED off
 */
void bsp_set_led(bool state);

/**
 * @brief Toggle status LED state
 */
void bsp_toggle_led(void);

/**
 * @brief Read button state
 *
 * @param button Button to read (BSP_BUTTON_PREV or BSP_BUTTON_NEXT)
 * @return true if button is pressed, false otherwise
 */
bool bsp_read_button(bsp_button_t button);

/**
 * @brief Read battery voltage
 *
 * Enables voltage divider, takes multiple ADC readings, and calculates battery voltage.
 * Automatically disables voltage divider after measurement to save power.
 *
 * @return Battery voltage in volts, or -1.0 on error
 */
float bsp_read_battery(void);

/**
 * @brief Enter deep sleep mode with button wake-up
 *
 * Configures both buttons as wake sources and enters deep sleep.
 * Device will wake up when either button is pressed.
 *
 * @return ESP_OK (should not return in normal operation)
 */
esp_err_t bsp_enter_sleep(void);

/**
 * @brief Initialize I2C bus for OLED communication
 *
 * Configures I2C0 for SH1106 OLED display with proper timing.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_init_i2c(void);

/**
 * @brief Initialize SH1106 OLED display
 *
 * Sends initialization sequence to configure SH1106 display.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_oled_init(void);

/**
 * @brief Clear OLED display
 *
 * Clears all pixels on the SH1106 display.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_oled_clear(void);

/**
 * @brief Write command to OLED
 *
 * @param cmd Command byte to send
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_oled_write_command(uint8_t cmd);

/**
 * @brief Initialize SPI bus for LoRa communication
 *
 * Configures SPI2_HOST for SX1262 LoRa transceiver with proper timing and control pins.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_init_spi(void);

/**
 * @brief Read SX1262 register via SPI
 *
 * @param reg Register address to read
 * @return Register value, or 0 on error
 */
uint8_t bsp_sx1262_read_register(uint16_t reg);

/**
 * @brief Reset SX1262 chip
 *
 * Performs hardware reset sequence for SX1262.
 *
 * @return ESP_OK on success
 */
esp_err_t bsp_sx1262_reset(void);

/**
 * @brief Validate hardware functionality
 *
 * Tests buttons and battery monitoring to ensure hardware is working correctly.
 *
 * @return ESP_OK if hardware validation passes, ESP_FAIL otherwise
 */
esp_err_t bsp_validate_hardware(void);

/**
 * @brief Initialize u8g2 graphics library with BSP HAL callbacks
 *
 * @param u8g2 Pointer to u8g2 structure to initialize
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_u8g2_init(void *u8g2);

/**
 * @brief Get board identifier string
 *
 * Returns a unique identifier for the hardware board type.
 * Used for firmware compatibility checking during OTA updates.
 *
 * @return Constant string with board ID (e.g., "heltec_v3", "wokwi_sim")
 */
const char* bsp_get_board_id(void);

/**
 * @brief USB descriptor configuration
 */
typedef struct {
    uint16_t usb_pid;           ///< USB Product ID
    const char *usb_product;    ///< USB Product string
} bsp_usb_config_t; // cppcheck-suppress unusedStructMember

/**
 * @brief Get USB configuration for this board
 *
 * @return Pointer to USB configuration structure
 */
const bsp_usb_config_t* bsp_get_usb_config(void);

#ifdef __cplusplus
}
#endif
