/**
 * @file uart_commands.h
 * @brief UART command interface for LoRaCue
 *
 * Provides command interface over hardware UART0 (USB-to-Serial)
 * Baudrate: 460800, 8N1
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize UART command interface
 * 
 * Configures UART0 (TX=GPIO43, RX=GPIO44) at 460800 baud
 * and starts command processing task
 * 
 * @return ESP_OK on success
 */
esp_err_t uart_commands_init(void);

/**
 * @brief Start UART command processing task
 * 
 * @return ESP_OK on success
 */
esp_err_t uart_commands_start(void);

/**
 * @brief Stop UART command processing task
 * 
 * @return ESP_OK on success
 */
esp_err_t uart_commands_stop(void);

#ifdef __cplusplus
}
#endif
