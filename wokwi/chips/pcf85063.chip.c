#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

// PCF85063 RTC I2C chip
#define PCF85063_ADDR 0x51

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t days;
    uint8_t weekdays;
    uint8_t months;
    uint8_t years;
    uint8_t current_reg;
} chip_state_t;

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t chip_i2c_read(void *user_data);
static bool chip_i2c_write(void *user_data, uint8_t data);
static void chip_i2c_disconnect(void *user_data);

static uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

void chip_init(void) {
    chip_state_t *chip = malloc(sizeof(chip_state_t));
    if (!chip) return;
    
    // Initialize with default time (2025-01-01 00:00:00)
    chip->seconds = 0x00;
    chip->minutes = 0x00;
    chip->hours = 0x00;
    chip->days = dec_to_bcd(1);
    chip->weekdays = 3;  // Wednesday
    chip->months = dec_to_bcd(1);
    chip->years = dec_to_bcd(25);  // 2025
    chip->current_reg = 0x04;

    const i2c_config_t i2c_config = {
        .user_data = chip,
        .address = PCF85063_ADDR,
        .scl = pin_init("SCL", INPUT_PULLUP),
        .sda = pin_init("SDA", INPUT_PULLUP),
        .connect = chip_i2c_connect,
        .read = chip_i2c_read,
        .write = chip_i2c_write,
        .disconnect = chip_i2c_disconnect,
    };
    
    i2c_init(&i2c_config);
    
    printf("PCF85063: RTC initialized at 0x%02X\n", PCF85063_ADDR);
}

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect) {
    return true;
}

static uint8_t chip_i2c_read(void *user_data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    uint8_t value = 0;
    
    switch (chip->current_reg) {
        case 0x04: value = chip->seconds; break;
        case 0x05: value = chip->minutes; break;
        case 0x06: value = chip->hours; break;
        case 0x07: value = chip->days; break;
        case 0x08: value = chip->weekdays; break;
        case 0x09: value = chip->months; break;
        case 0x0A: value = chip->years; break;
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
        switch (chip->current_reg) {
            case 0x04: chip->seconds = data; break;
            case 0x05: chip->minutes = data; break;
            case 0x06: chip->hours = data; break;
            case 0x07: chip->days = data; break;
            case 0x08: chip->weekdays = data; break;
            case 0x09: chip->months = data; break;
            case 0x0A: chip->years = data; break;
        }
        chip->current_reg++;
    }
    
    return true;
}

static void chip_i2c_disconnect(void *user_data) {
}
