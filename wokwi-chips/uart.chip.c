// Wokwi Custom UART Bridge Chip
// Bridges ESP32 UART to visible terminal output in Wokwi CLI
// SPDX-License-Identifier: GPL-3.0

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 256

typedef struct {
  uart_dev_t uart0;
  char buffer[BUFFER_SIZE];
  int buffer_pos;
} chip_state_t;

static void on_uart_rx_data(void *user_data, uint8_t byte);

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->buffer_pos = 0;
  memset(chip->buffer, 0, BUFFER_SIZE);

  const uart_config_t uart_config = {
    .tx = pin_init("TX", INPUT_PULLUP),
    .rx = pin_init("RX", INPUT),
    .baud_rate = 115200,
    .rx_data = on_uart_rx_data,
    .user_data = chip,
  };
  chip->uart0 = uart_init(&uart_config);
}

static void on_uart_rx_data(void *user_data, uint8_t byte) {
  chip_state_t *chip = (chip_state_t*)user_data;
  
  if (byte == '\n' || chip->buffer_pos >= BUFFER_SIZE - 1) {
    chip->buffer[chip->buffer_pos] = '\0';
    printf("%s\n", chip->buffer);
    fflush(stdout);
    chip->buffer_pos = 0;
  } else if (byte != '\r') {
    chip->buffer[chip->buffer_pos++] = byte;
  }
}
