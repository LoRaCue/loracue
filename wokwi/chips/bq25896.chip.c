#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

// BQ25896 Battery Charger I2C chip
#define BQ25896_ADDR 0x6B

typedef struct {
    uint8_t regs[0x15];
    uint8_t current_reg;
} chip_state_t;

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t chip_i2c_read(void *user_data);
static bool chip_i2c_write(void *user_data, uint8_t data);
static void chip_i2c_disconnect(void *user_data);

void chip_init(void) {
    chip_state_t *chip = malloc(sizeof(chip_state_t));
    if (!chip) return;
    
    // Initialize registers with default values
    for (int i = 0; i < 0x15; i++) {
        chip->regs[i] = 0;
    }
    
    // Set some typical status values
    chip->regs[0x0B] = 0x00;  // VBUS status: no input
    chip->regs[0x0E] = 0x23;  // Part info
    chip->current_reg = 0;

    const i2c_config_t i2c_config = {
        .user_data = chip,
        .address = BQ25896_ADDR,
        .scl = pin_init("SCL", INPUT_PULLUP),
        .sda = pin_init("SDA", INPUT_PULLUP),
        .connect = chip_i2c_connect,
        .read = chip_i2c_read,
        .write = chip_i2c_write,
        .disconnect = chip_i2c_disconnect,
    };
    
    i2c_init(&i2c_config);
    
    printf("BQ25896: Charger initialized at 0x%02X\n", BQ25896_ADDR);
}

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect) {
    return true;
}

static uint8_t chip_i2c_read(void *user_data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    uint8_t value = 0;
    
    if (chip->current_reg < 0x15) {
        value = chip->regs[chip->current_reg];
    }
    
    chip->current_reg++;
    return value;
}

static bool chip_i2c_write(void *user_data, uint8_t data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    static bool first_byte = true;
    
    if (first_byte) {
        chip->current_reg = data;
        first_byte = false;
    } else {
        if (chip->current_reg < 0x15) {
            chip->regs[chip->current_reg] = data;
        }
        chip->current_reg++;
    }
    
    return true;
}

static void chip_i2c_disconnect(void *user_data) {
}
