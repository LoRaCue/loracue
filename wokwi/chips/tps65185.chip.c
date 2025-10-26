#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

// TPS65185 E-Paper PMIC I2C chip
#define TPS65185_ADDR 0x68

typedef struct {
    uint8_t regs[0x10];
    uint8_t current_reg;
} chip_state_t;

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t chip_i2c_read(void *user_data);
static bool chip_i2c_write(void *user_data, uint8_t data);
static void chip_i2c_disconnect(void *user_data);

void chip_init(void) {
    chip_state_t *chip = malloc(sizeof(chip_state_t));
    if (!chip) return;
    
    // Initialize registers
    for (int i = 0; i < 0x10; i++) {
        chip->regs[i] = 0;
    }
    
    chip->regs[0x00] = 0x00;  // TMST_VALUE
    chip->regs[0x01] = 0x3F;  // Enable all rails
    chip->regs[0x0A] = 0x00;  // INT1
    chip->regs[0x0B] = 0x00;  // INT2
    chip->current_reg = 0;

    const i2c_config_t i2c_config = {
        .user_data = chip,
        .address = TPS65185_ADDR,
        .scl = pin_init("SCL", INPUT_PULLUP),
        .sda = pin_init("SDA", INPUT_PULLUP),
        .connect = chip_i2c_connect,
        .read = chip_i2c_read,
        .write = chip_i2c_write,
        .disconnect = chip_i2c_disconnect,
    };
    
    i2c_init(&i2c_config);
    
    printf("TPS65185: E-Paper PMIC initialized at 0x%02X\n", TPS65185_ADDR);
}

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect) {
    return true;
}

static uint8_t chip_i2c_read(void *user_data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    uint8_t value = 0;
    
    if (chip->current_reg < 0x10) {
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
        if (chip->current_reg < 0x10) {
            chip->regs[chip->current_reg] = data;
        }
        chip->current_reg++;
    }
    
    return true;
}

static void chip_i2c_disconnect(void *user_data) {
}
