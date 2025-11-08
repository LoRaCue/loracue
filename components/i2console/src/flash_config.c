#include "flash_config.h"
#include "log.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

static config_t current_config;

void flash_config_init(void) {
    flash_config_load(&current_config);
    if (current_config.magic != CONFIG_MAGIC) {
        current_config.magic = CONFIG_MAGIC;
        current_config.i2c_address = 0x37;
        current_config.clock_stretch_enable = 0;
        flash_config_save(&current_config);
        LOG_INFO("Flash config initialized with defaults");
    } else {
        LOG_DEBUG("Flash config loaded: addr=0x%02X", current_config.i2c_address);
    }
}

void flash_config_load(config_t *config) {
    const uint8_t *flash_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy(config, flash_ptr, sizeof(config_t));
}

void flash_config_save(const config_t *config) {
    uint8_t buffer[FLASH_PAGE_SIZE];
    memcpy(buffer, config, sizeof(config_t));
    
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buffer, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
    
    memcpy(&current_config, config, sizeof(config_t));
}

uint8_t flash_config_get_i2c_address(void) {
    return current_config.i2c_address;
}

void flash_config_set_i2c_address(uint8_t address) {
    LOG_INFO("I2C address changed: 0x%02X -> 0x%02X", current_config.i2c_address, address);
    current_config.i2c_address = address;
    flash_config_save(&current_config);
}

bool flash_config_get_clock_stretch(void) {
    return current_config.clock_stretch_enable != 0;
}

void flash_config_set_clock_stretch(bool enable) {
    current_config.clock_stretch_enable = enable ? 1 : 0;
    flash_config_save(&current_config);
}
