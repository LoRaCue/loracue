#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

// GT911 Touch Controller I2C chip
#define GT911_ADDR 0x5D

typedef struct {
    uint8_t regs[0x8150];
    uint16_t current_reg;
    uint8_t byte_count;
} chip_state_t;

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t chip_i2c_read(void *user_data);
static bool chip_i2c_write(void *user_data, uint8_t data);
static void chip_i2c_disconnect(void *user_data);

void chip_init(void) {
    chip_state_t *chip = malloc(sizeof(chip_state_t));
    if (!chip) return;
    
    // Initialize registers
    for (int i = 0; i < 0x8150; i++) {
        chip->regs[i] = 0;
    }
    
    // Product ID at 0x8140-0x8143: "911"
    chip->regs[0x8140] = '9';
    chip->regs[0x8141] = '1';
    chip->regs[0x8142] = '1';
    chip->regs[0x8143] = 0;
    
    // Config version at 0x8047
    chip->regs[0x8047] = 0x01;
    
    // Touch status at 0x814E (no touch)
    chip->regs[0x814E] = 0x00;
    
    chip->current_reg = 0;
    chip->byte_count = 0;

    const i2c_config_t i2c_config = {
        .user_data = chip,
        .address = GT911_ADDR,
        .scl = pin_init("SCL", INPUT_PULLUP),
        .sda = pin_init("SDA", INPUT_PULLUP),
        .connect = chip_i2c_connect,
        .read = chip_i2c_read,
        .write = chip_i2c_write,
        .disconnect = chip_i2c_disconnect,
    };
    
    i2c_init(&i2c_config);
    
    printf("GT911: Touch controller initialized at 0x%02X\n", GT911_ADDR);
}

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect) {
    return true;
}

static uint8_t chip_i2c_read(void *user_data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    uint8_t value = 0;
    
    if (chip->current_reg < 0x8150) {
        value = chip->regs[chip->current_reg];
    }
    
    chip->current_reg++;
    return value;
}

static bool chip_i2c_write(void *user_data, uint8_t data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    
    if (chip->byte_count == 0) {
        // First byte: register address high
        chip->current_reg = (data << 8);
        chip->byte_count = 1;
    } else if (chip->byte_count == 1) {
        // Second byte: register address low
        chip->current_reg |= data;
        chip->byte_count = 2;
    } else {
        // Data bytes
        if (chip->current_reg < 0x8150) {
            chip->regs[chip->current_reg] = data;
        }
        chip->current_reg++;
    }
    
    return true;
}

static void chip_i2c_disconnect(void *user_data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    chip->byte_count = 0;
}
