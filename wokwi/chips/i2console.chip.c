// I2Console Wokwi Chip - I2C to USB-CDC Console Bridge Simulator
// Simulates RP2350-based I2Console device for testing I2C console communication

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEVICE_ID 0x12C0
#define FW_VERSION 0x01
#define I2C_ADDRESS 0x37
#define VERSION_STRING "v0.1.0-6-g7c168ca-dirty"

#define REG_DEVICE_ID       0x00
#define REG_FW_VERSION      0x01
#define REG_I2C_ADDRESS     0x02
#define REG_CLOCK_STRETCH   0x03
#define REG_VERSION_STRING  0x04
#define REG_TX_AVAIL_LOW    0x10
#define REG_TX_AVAIL_HIGH   0x11
#define REG_RX_AVAIL        0x12
#define REG_DATA_START      0x20

typedef struct {
  uint8_t current_register;
  bool register_set;
  uint8_t version_string_index;
} chip_state_t;

static bool on_i2c_connect(void *user_data, uint32_t address, bool connect) {
  return address == I2C_ADDRESS;
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t*)user_data;
  uint8_t data = 0;
  
  if (chip->current_register == REG_DEVICE_ID) {
    data = (DEVICE_ID >> 8) & 0xFF;
  } else if (chip->current_register == REG_DEVICE_ID + 1) {
    data = DEVICE_ID & 0xFF;
  } else if (chip->current_register == REG_FW_VERSION) {
    data = FW_VERSION;
  } else if (chip->current_register == REG_I2C_ADDRESS) {
    data = I2C_ADDRESS;
  } else if (chip->current_register == REG_CLOCK_STRETCH) {
    data = 0; // Clock stretch disabled
  } else if (chip->current_register == REG_VERSION_STRING) {
    if (chip->version_string_index < strlen(VERSION_STRING)) {
      data = VERSION_STRING[chip->version_string_index++];
    } else {
      data = 0; // Null terminator
    }
  } else if (chip->current_register == REG_TX_AVAIL_LOW) {
    data = 0xFF; // TX buffer available (low byte)
  } else if (chip->current_register == REG_TX_AVAIL_HIGH) {
    data = 0x03; // TX buffer available (high byte) - 1023 bytes
  } else if (chip->current_register == REG_RX_AVAIL) {
    data = 0; // No RX data available
  }
  
  return data;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = (chip_state_t*)user_data;
  
  if (!chip->register_set) {
    chip->current_register = data;
    chip->register_set = true;
    // Reset version string index when selecting version string register
    if (data == REG_VERSION_STRING) {
      chip->version_string_index = 0;
    }
    return true;
  }
  
  if (chip->current_register >= REG_DATA_START) {
    // Echo to console
    if (data >= 0x20 && data < 0x7F) {
      printf("%c", data);
    } else if (data == '\n') {
      printf("\n");
    }
    fflush(stdout);
  }
  
  return true;
}

static void on_i2c_disconnect(void *user_data) {
  chip_state_t *chip = (chip_state_t*)user_data;
  chip->register_set = false;
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  memset(chip, 0, sizeof(chip_state_t));
  
  const i2c_config_t i2c_config = {
    .user_data = chip,
    .address = I2C_ADDRESS,
    .scl = pin_init("SCL", INPUT_PULLUP),
    .sda = pin_init("SDA", INPUT_PULLUP),
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .disconnect = on_i2c_disconnect,
  };
  
  // i2c_init(&i2c_config);
  
  printf("[I2Console] Initialized at I2C address 0x%02X\n", I2C_ADDRESS);
}
