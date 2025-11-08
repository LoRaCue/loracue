#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <stdint.h>
#include "circular_buffer.h"

#define I2C_SLAVE_SDA_PIN 28
#define I2C_SLAVE_SCL_PIN 29

#define REG_DEVICE_ID 0x00
#define REG_FW_VERSION 0x01
#define REG_I2C_ADDRESS 0x02
#define REG_CLOCK_STRETCH 0x03
#define REG_TX_AVAIL_LOW 0x10
#define REG_TX_AVAIL_HIGH 0x11
#define REG_RX_AVAIL 0x12
#define REG_DATA_START 0x20

#define DEVICE_ID 0x12C0

typedef struct {
    uint32_t tx_bytes;
    uint32_t rx_bytes;
    uint32_t tx_overflow;
    uint32_t rx_overflow;
    uint32_t i2c_errors;
} i2c_stats_t;

void i2c_slave_init(circular_buffer_t *tx_buf, circular_buffer_t *rx_buf);
void i2c_slave_task(void);
i2c_stats_t i2c_slave_get_stats(void);

#endif
