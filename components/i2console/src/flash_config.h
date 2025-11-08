#ifndef FLASH_CONFIG_H
#define FLASH_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_MAGIC 0x12C0CAFE

typedef struct {
    uint32_t magic;
    uint8_t i2c_address;
    uint8_t clock_stretch_enable;
    uint8_t reserved[2];
} config_t;

void flash_config_init(void);
void flash_config_load(config_t *config);
void flash_config_save(const config_t *config);
uint8_t flash_config_get_i2c_address(void);
void flash_config_set_i2c_address(uint8_t address);
bool flash_config_get_clock_stretch(void);
void flash_config_set_clock_stretch(bool enable);

#endif
