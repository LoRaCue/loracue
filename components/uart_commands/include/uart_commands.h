/**
 * @file uart_commands.h
 * @brief UART command interface for LoRaCue
 *
 * Provides command interface over hardware UART with dynamic configuration:
 * - Default: UART0=commands, UART1=console
 * - Swapped: UART0=console, UART1=commands (hold button at boot)
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
 * Checks button state at boot:
 * - Not pressed: UART0 (TX=GPIO43, RX=GPIO44) for commands
 * - Pressed: UART1 (TX=GPIO2, RX=GPIO3) for commands
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
