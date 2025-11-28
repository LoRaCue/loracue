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
    BSP_BUTTON_BOTH = 2, ///< Both buttons (Wokwi simulation only, ignored on hardware)
} bsp_button_t;

/**
 * @brief Display type enumeration
 */
typedef enum {
    BSP_DISPLAY_TYPE_OLED_SSD1306,  ///< OLED display (SSD1306)
    BSP_DISPLAY_TYPE_EPAPER_SSD1681 ///< E-Paper display (SSD1681)
} bsp_display_type_t;

/**
 * @brief Initialize the board support package
 *
 * Initializes all hardware peripherals including GPIO, ADC, and power management.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_init(void);

/**
 * @brief Get the board name
 *
 * @return Pointer to board name string (e.g., "Heltec V3", "LilyGO T3")
 */
const char* bsp_get_board_name(void);

/**
 * @brief Deinitialize BSP and free resources
 *
 * Cleans up LVGL, mutexes, and other allocated resources.
 * Should be called before system shutdown or reset.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bsp_deinit(void);

/**
 * 
 * 
 * @param timeout_ms Timeout in milliseconds (portMAX_DELAY for infinite)
 * @return true if lock acquired, false on timeout
 */

/**
 */

/**
 * @brief Get UART pins for specified port
 * 
 * @param uart_num UART port number (0, 1, or 2)
 * @param tx_pin Output parameter for TX pin
 * @param rx_pin Output parameter for RX pin
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if port not supported
 */
esp_err_t bsp_get_uart_pins(int uart_num, int *tx_pin, int *rx_pin);

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
 *
 * @return ESP_OK on success, error code otherwise
 */

/**
 * @brief Get board identifier string
 *
 * Returns a unique identifier for the hardware board type.
 * Used for firmware compatibility checking during OTA updates.
 *
 * @return Constant string with board ID (e.g., "heltec_v3", "wokwi_sim")
 */
const char *bsp_get_board_id(void);

/**
 * @brief USB descriptor configuration
 */
typedef struct {
    uint16_t usb_pid;        ///< USB Product ID
    const char *usb_product; ///< USB Product string
} bsp_usb_config_t;          // cppcheck-suppress unusedStructMember

/**
 * @brief LoRa SX126X pin configuration (all fields used in sx126x.c)
 */
typedef struct {
    int miso;
    int mosi;
    int sclk;
    int cs;
    int rst;
    int busy;
    int dio1;
} bsp_lora_pins_t; // cppcheck-suppress unusedStructMember

/**
 * @brief Get USB configuration for this board
 *
 * @return Pointer to USB configuration structure
 */
const bsp_usb_config_t *bsp_get_usb_config(void);

/**
 * @brief Check if USB is connected
 *
 * @return true if USB is connected, false otherwise
 */
bool bsp_is_usb_connected(void);

/**
 * @brief Get device serial number (derived from MAC address)
 * @param serial_number Buffer to store serial number (min 13 bytes)
 * @param max_len Maximum length of buffer
 * @return ESP_OK on success
 */
esp_err_t bsp_get_serial_number(char *serial_number, size_t max_len);

/**
 * @brief Get LoRa pin configuration
 * @return Pointer to LoRa pin configuration structure
 */
const bsp_lora_pins_t *bsp_get_lora_pins(void);

/**
 * @brief Set display brightness/contrast
 * @param brightness Brightness level (0-255)
 * @return ESP_OK on success
 */
esp_err_t bsp_set_display_brightness(uint8_t brightness);

/**
 * @brief Put display into power save mode
 * @return ESP_OK on success
 */
esp_err_t bsp_display_sleep(void);

/**
 * @brief Wake display from power save mode
 * @return ESP_OK on success
 */
esp_err_t bsp_display_wake(void);

// I2C Bus Management
#include "driver/i2c_master.h"

/**
 * @brief Initialize shared I2C bus with board-specific pins
 *
 * @return ESP_OK on success
 */
esp_err_t bsp_i2c_init_default(void);

/**
 * @brief Initialize shared I2C bus
 *
 * @param sda SDA GPIO pin
 * @param scl SCL GPIO pin
 * @param freq_hz I2C frequency in Hz
 * @return ESP_OK on success
 */
esp_err_t bsp_i2c_init(gpio_num_t sda, gpio_num_t scl, uint32_t freq_hz);

/**
 * @brief Deinitialize I2C bus
 * 
 * @return ESP_OK on success
 */
esp_err_t bsp_i2c_deinit(void);

/**
 * @brief Get I2C bus handle for adding devices
 *
 * @return I2C bus handle or NULL if not initialized
 */
i2c_master_bus_handle_t bsp_i2c_get_bus(void);

/**
 * @brief Add I2C device to bus
 *
 * @param addr 7-bit I2C address
 * @param freq_hz Device-specific frequency
 * @param dev_handle Output device handle
 * @return ESP_OK on success
 */
esp_err_t bsp_i2c_add_device(uint8_t addr, uint32_t freq_hz, i2c_master_dev_handle_t *dev_handle);


/**
 */

/**
 */

/**
 * @brief Set OLED reset pin
 *
 * @param pin Reset GPIO pin
 */
void bsp_oled_set_reset_pin(gpio_num_t pin);

/**
 * @brief Power on E-Paper display (UI_RICH only)
 *
 * @return ESP_OK on success
 */
esp_err_t bsp_epaper_power_on(void);

/**
 * @brief Power off E-Paper display (UI_RICH only)
 *
 * @return ESP_OK on success
 */
esp_err_t bsp_epaper_power_off(void);

/**
 * @brief Get display type for this board
 *
 * @return Display type enumeration
 */
bsp_display_type_t bsp_get_display_type(void);

/**
 * @brief Get I2C bus handle for display
 *
 * @return I2C bus handle or NULL if not available
 */
i2c_master_bus_handle_t bsp_get_i2c_bus(void);

/**
 * @brief Get SPI device handle for display
 *
 * @return SPI device handle or NULL if not available
 */
void *bsp_get_spi_device(void);

/**
 * @brief Get E-Paper DC pin
 *
 * @return GPIO pin number or -1 if not available
 */
int bsp_get_epaper_dc_pin(void);

/**
 * @brief Get E-Paper CS pin
 *
 * @return GPIO pin number or -1 if not available
 */
int bsp_get_epaper_cs_pin(void);

/**
 * @brief Get E-Paper RST pin
 *
 * @return GPIO pin number or -1 if not available
 */
int bsp_get_epaper_rst_pin(void);

/**
 * @brief Get E-Paper BUSY pin
 *
 * @return GPIO pin number or -1 if not available
 */
int bsp_get_epaper_busy_pin(void);

#ifdef __cplusplus
}
#endif
