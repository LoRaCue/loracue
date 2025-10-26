#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

// PCA9535 I2C GPIO Expander
// 16-bit I/O expander with interrupt

#define PCA9535_INPUT_PORT0     0x00
#define PCA9535_INPUT_PORT1     0x01
#define PCA9535_OUTPUT_PORT0    0x02
#define PCA9535_OUTPUT_PORT1    0x03
#define PCA9535_POLARITY_PORT0  0x04
#define PCA9535_POLARITY_PORT1  0x05
#define PCA9535_CONFIG_PORT0    0x06
#define PCA9535_CONFIG_PORT1    0x07

typedef struct {
    uint8_t input_port0;
    uint8_t input_port1;
    uint8_t output_port0;
    uint8_t output_port1;
    uint8_t polarity_port0;
    uint8_t polarity_port1;
    uint8_t config_port0;
    uint8_t config_port1;
    uint8_t current_reg;
} chip_state_t;

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t chip_i2c_read(void *user_data);
static bool chip_i2c_write(void *user_data, uint8_t data);
static void chip_i2c_disconnect(void *user_data);

void chip_init(void) {
    chip_state_t *chip = malloc(sizeof(chip_state_t));
    if (!chip) return;
    
    // Initialize with default values
    chip->input_port0 = 0xFF;      // All inputs high
    chip->input_port1 = 0xFF;
    chip->output_port0 = 0xFF;
    chip->output_port1 = 0xFF;
    chip->polarity_port0 = 0x00;
    chip->polarity_port1 = 0x00;
    chip->config_port0 = 0xFF;     // All pins as inputs
    chip->config_port1 = 0xFF;
    chip->current_reg = 0x00;

    const i2c_config_t i2c_config = {
        .user_data = chip,
        .address = 0x20,
        .scl = pin_init("SCL", INPUT_PULLUP),
        .sda = pin_init("SDA", INPUT_PULLUP),
        .connect = chip_i2c_connect,
        .read = chip_i2c_read,
        .write = chip_i2c_write,
        .disconnect = chip_i2c_disconnect,
    };
    
    i2c_init(&i2c_config);
    
    printf("PCA9535: Initialized at address 0x20\n");
}

static bool chip_i2c_connect(void *user_data, uint32_t address, bool connect) {
    if (connect) {
        printf("PCA9535: Connected\n");
    }
    return true;
}

static uint8_t chip_i2c_read(void *user_data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    uint8_t value = 0xFF;
    
    switch (chip->current_reg) {
        case PCA9535_INPUT_PORT0:
            value = chip->input_port0;
            break;
        case PCA9535_INPUT_PORT1:
            value = chip->input_port1;
            break;
        case PCA9535_OUTPUT_PORT0:
            value = chip->output_port0;
            break;
        case PCA9535_OUTPUT_PORT1:
            value = chip->output_port1;
            break;
        case PCA9535_POLARITY_PORT0:
            value = chip->polarity_port0;
            break;
        case PCA9535_POLARITY_PORT1:
            value = chip->polarity_port1;
            break;
        case PCA9535_CONFIG_PORT0:
            value = chip->config_port0;
            break;
        case PCA9535_CONFIG_PORT1:
            value = chip->config_port1;
            break;
    }
    
    return value;
}

static bool chip_i2c_write(void *user_data, uint8_t data) {
    chip_state_t *chip = (chip_state_t*)user_data;
    static bool first_byte = true;
    
    if (first_byte) {
        // First byte is register address
        chip->current_reg = data;
        first_byte = false;
    } else {
        // Subsequent bytes are data
        switch (chip->current_reg) {
            case PCA9535_OUTPUT_PORT0:
                chip->output_port0 = data;
                break;
            case PCA9535_OUTPUT_PORT1:
                chip->output_port1 = data;
                break;
            case PCA9535_POLARITY_PORT0:
                chip->polarity_port0 = data;
                break;
            case PCA9535_POLARITY_PORT1:
                chip->polarity_port1 = data;
                break;
            case PCA9535_CONFIG_PORT0:
                chip->config_port0 = data;
                break;
            case PCA9535_CONFIG_PORT1:
                chip->config_port1 = data;
                break;
        }
        chip->current_reg++;
    }
    
    return true;
}

static void chip_i2c_disconnect(void *user_data) {
    // Reset for next transaction
}
