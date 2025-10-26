#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

// BQ27220 Fuel Gauge I2C chip
#define BQ27220_ADDR 0x55

// Register addresses
#define REG_VOLTAGE     0x04
#define REG_CURRENT     0x10
#define REG_CAPACITY    0x0C
#define REG_SOC         0x1C  // State of charge
#define REG_TEMP        0x06

typedef struct {
    uint16_t voltage;      // mV
    int16_t current;       // mA
    uint16_t capacity;     // mAh
    uint8_t soc;          // %
    uint16_t temperature;  // 0.1K
    uint8_t current_reg;
    uint8_t byte_count;
} chip_state_t;

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t chip_i2c_read(void *user_data);
static bool chip_i2c_write(void *user_data, uint8_t data);
static void chip_i2c_disconnect(void *user_data);

void chip_init(void) {
    chip_state_t *chip = malloc(sizeof(chip_state_t));
    if (!chip) return;
    
    // Initialize with typical values
    chip->voltage = 3700;      // 3.7V
    chip->current = -100;      // -100mA (discharging)
    chip->capacity = 2000;     // 2000mAh
    chip->soc = 75;           // 75%
    chip->temperature = 2980;  // 25°C (298K)
    chip->current_reg = 0;
    chip->byte_count = 0;

    const i2c_config_t i2c_config = {
        .user_data = chip,
        .address = BQ27220_ADDR,
        .scl = pin_init("SCL", INPUT_PULLUP),
        .sda = pin_init("SDA", INPUT_PULLUP),
        .connect = chip_i2c_connect,
        .read = chip_i2c_read,
        .write = chip_i2c_write,
        .disconnect = chip_i2c_disconnect,
    };
    
    i2c_init(&i2c_config);
    
    printf("BQ27220: Fuel gauge initialized at 0x%02X\n", BQ27220_ADDR);
}

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect) {
    return true;
}

static uint8_t chip_i2c_read(void *user_data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    uint8_t value = 0;
    
    switch (chip->current_reg) {
        case REG_VOLTAGE:
            value = (chip->byte_count == 0) ? (chip->voltage & 0xFF) : (chip->voltage >> 8);
            break;
        case REG_CURRENT:
            value = (chip->byte_count == 0) ? (chip->current & 0xFF) : (chip->current >> 8);
            break;
        case REG_CAPACITY:
            value = (chip->byte_count == 0) ? (chip->capacity & 0xFF) : (chip->capacity >> 8);
            break;
        case REG_SOC:
            value = chip->soc;
            break;
        case REG_TEMP:
            value = (chip->byte_count == 0) ? (chip->temperature & 0xFF) : (chip->temperature >> 8);
            break;
    }
    
    chip->byte_count++;
    return value;
}

static bool chip_i2c_write(void *user_data, uint8_t data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    chip->current_reg = data;
    chip->byte_count = 0;
    return true;
}

static void chip_i2c_disconnect(void *user_data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    chip->byte_count = 0;
}
