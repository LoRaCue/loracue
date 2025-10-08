/**
 * @file lora_bands.h
 * @brief LoRa hardware band profiles
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LORA_MAX_BANDS 8
#define LORA_BAND_ID_LEN 16
#define LORA_BAND_NAME_LEN 64

/**
 * @brief LoRa hardware band profile
 */
typedef struct {
    char id[LORA_BAND_ID_LEN];           ///< Band ID (e.g., "HW_433")
    char name[LORA_BAND_NAME_LEN];       ///< Human-readable name
    uint32_t optimal_center_khz;         ///< Optimal center frequency in kHz
    uint32_t optimal_freq_min_khz;       ///< Optimal minimum frequency in kHz
    uint32_t optimal_freq_max_khz;       ///< Optimal maximum frequency in kHz
    int8_t max_power_dbm;                ///< Maximum regulatory TX power (from first public_bands entry)
} lora_band_profile_t;

/**
 * @brief Initialize band profiles from embedded JSON
 * @return ESP_OK on success
 */
esp_err_t lora_bands_init(void);

/**
 * @brief Get number of available band profiles
 * @return Number of bands
 */
int lora_bands_get_count(void);

/**
 * @brief Get band profile by index
 * @param index Band index (0 to count-1)
 * @return Pointer to band profile or NULL
 */
const lora_band_profile_t* lora_bands_get_profile(int index);

/**
 * @brief Get band profile by ID
 * @param id Band ID string
 * @return Pointer to band profile or NULL
 */
const lora_band_profile_t* lora_bands_get_profile_by_id(const char *id);

/**
 * @brief Get band index for a given frequency
 * @param frequency_hz Frequency in Hz
 * @return Band index or -1 if not in any band
 */
int lora_bands_get_index_by_frequency(uint32_t frequency_hz);

#ifdef __cplusplus
}
#endif
