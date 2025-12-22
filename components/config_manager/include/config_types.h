#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Device mode enumeration
typedef enum { 
    DEVICE_MODE_PRESENTER = 0, 
    DEVICE_MODE_PC = 1 
} device_mode_t;

// General device configuration
typedef struct {
    char device_name[32];
    device_mode_t device_mode;
    uint8_t display_contrast;
    bool bluetooth_enabled;
    bool bluetooth_pairing_enabled;
    uint8_t slot_id;
} general_config_t;

// Power management configuration
typedef struct {
    uint32_t display_sleep_timeout_ms;
    uint32_t light_sleep_timeout_ms;
    uint32_t deep_sleep_timeout_ms;
    bool enable_auto_display_sleep;
    bool enable_auto_light_sleep;
    bool enable_auto_deep_sleep;
    uint8_t cpu_freq_mhz;
} power_config_t;

// LoRa radio configuration
typedef struct {
    uint32_t frequency;
    uint8_t spreading_factor;
    uint16_t bandwidth;
    uint8_t coding_rate;
    int8_t tx_power;
    char band_id[16];
    char regulatory_domain[3];
    uint8_t aes_key[32];
} lora_config_t;

// Device registry entry
typedef struct {
    uint16_t device_id;
    char device_name[32];
    uint8_t mac_address[6];
    uint8_t aes_key[32];
    uint16_t highest_sequence;
    uint64_t recent_bitmap;
} paired_device_t;

// Device registry configuration
typedef struct {
    paired_device_t devices[4];
    size_t device_count;
} device_registry_config_t;

#ifdef __cplusplus
}
#endif
