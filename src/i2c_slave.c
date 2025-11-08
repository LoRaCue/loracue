#include "i2c_slave.h"
#include "flash_config.h"
#include "log.h"
#include "version.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/regs/i2c.h"
#include <string.h>
#include <stdio.h>

static uint8_t fw_version_byte = 0x01;
static char version_short[16] = {0};

static void parse_version(void) {
    const char *v = FW_VERSION;
    // Skip 'v' prefix if present
    if (v[0] == 'v') v++;
    
    // Copy until we hit '-g' (git hash marker) or end
    int i = 0;
    while (v[i] && i < 15) {
        if (v[i] == '-' && v[i+1] == 'g') break;
        version_short[i] = v[i];
        i++;
    }
    version_short[i] = '\0';
}

static circular_buffer_t *tx_buffer;
static circular_buffer_t *rx_buffer;
static i2c_stats_t stats = {0};
static uint8_t current_register = 0;
static bool register_set = false;
static bool is_read_mode = false;

static void i2c0_irq_handler(void) {
    i2c_hw_t *hw = i2c_get_hw(i2c0);
    uint32_t intr_stat = hw->intr_stat;
    
    if (intr_stat & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        uint8_t data = (uint8_t)hw->data_cmd;
        
        if (!register_set) {
            current_register = data;
            register_set = true;
        } else {
            if (current_register == REG_I2C_ADDRESS) {
                flash_config_set_i2c_address(data);
            } else if (current_register == REG_CLOCK_STRETCH) {
                flash_config_set_clock_stretch(data & 0x01);
            } else if (current_register >= REG_DATA_START) {
                if (!circular_buffer_push(tx_buffer, data)) {
                    stats.tx_overflow++;
                } else {
                    stats.tx_bytes++;
                }
            }
        }
    }
    
    if (intr_stat & I2C_IC_INTR_STAT_R_RD_REQ_BITS) {
        hw->clr_rd_req;
        uint8_t data = 0;
        
        if (current_register == REG_DEVICE_ID) {
            data = (DEVICE_ID >> 8) & 0xFF;
        } else if (current_register == REG_DEVICE_ID + 1) {
            data = DEVICE_ID & 0xFF;
        } else if (current_register == REG_FW_VERSION) {
            data = fw_version_byte;
        } else if (current_register >= 0x04 && current_register < 0x04 + sizeof(version_short)) {
            // Version string registers 0x04-0x13
            data = version_short[current_register - 0x04];
        } else if (current_register == REG_I2C_ADDRESS) {
            data = flash_config_get_i2c_address();
        } else if (current_register == REG_CLOCK_STRETCH) {
            data = flash_config_get_clock_stretch() ? 1 : 0;
        } else if (current_register == REG_TX_AVAIL_LOW) {
            uint16_t avail = circular_buffer_available(tx_buffer);
            data = avail & 0xFF;
        } else if (current_register == REG_TX_AVAIL_HIGH) {
            uint16_t avail = circular_buffer_available(tx_buffer);
            data = (avail >> 8) & 0xFF;
        } else if (current_register == REG_RX_AVAIL) {
            data = circular_buffer_available(rx_buffer) & 0xFF;
        } else if (current_register >= REG_DATA_START) {
            if (!circular_buffer_pop(rx_buffer, &data)) {
                data = 0x00;
            } else {
                stats.rx_bytes++;
            }
        }
        
        hw->data_cmd = data;
    }
    
    if (intr_stat & I2C_IC_INTR_STAT_R_STOP_DET_BITS) {
        hw->clr_stop_det;
        register_set = false;
    }
}

void i2c_slave_init(circular_buffer_t *tx_buf, circular_buffer_t *rx_buf) {
    tx_buffer = tx_buf;
    rx_buffer = rx_buf;
    
    parse_version();
    
    gpio_init(I2C_SLAVE_SDA_PIN);
    gpio_set_function(I2C_SLAVE_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SDA_PIN);
    
    gpio_init(I2C_SLAVE_SCL_PIN);
    gpio_set_function(I2C_SLAVE_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SCL_PIN);
    
    i2c_init(i2c0, 100000);
    
    i2c_hw_t *hw = i2c_get_hw(i2c0);
    hw->enable = 0;
    hw->con = I2C_IC_CON_IC_SLAVE_DISABLE_BITS | I2C_IC_CON_IC_RESTART_EN_BITS;
    hw->sar = flash_config_get_i2c_address();
    hw->con &= ~I2C_IC_CON_IC_SLAVE_DISABLE_BITS;
    hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | 
                    I2C_IC_INTR_MASK_M_RD_REQ_BITS |
                    I2C_IC_INTR_MASK_M_STOP_DET_BITS;
    hw->enable = 1;
    
    irq_set_exclusive_handler(I2C0_IRQ, i2c0_irq_handler);
    irq_set_enabled(I2C0_IRQ, true);
}

void i2c_slave_task(void) {
    // Handler runs in interrupt context
}

i2c_stats_t i2c_slave_get_stats(void) {
    return stats;
}
