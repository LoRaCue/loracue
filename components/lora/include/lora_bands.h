/**
 * @file lora_bands.h
 * @brief LoRa regulatory compliance and hardware profiles
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LORA_MAX_HARDWARE 8     // Max hardware profiles
#define LORA_MAX_REGIONS 16     // Max regulatory regions
#define LORA_MAX_COMPLIANCE 64  // Max compliance rules
#define LORA_BAND_ID_LEN 16
#define LORA_BAND_NAME_LEN 64

/**
 * @brief Hardware profile structure
 */
typedef struct {
    char id[LORA_BAND_ID_LEN];     ///< Hardware ID (e.g., "HW_433")
    char name[LORA_BAND_NAME_LEN]; ///< Human-readable name
    uint32_t freq_min_khz;         ///< Hardware minimum frequency in kHz
    uint32_t freq_max_khz;         ///< Hardware maximum frequency in kHz
    uint32_t optimal_khz;          ///< Optimal frequency in kHz
} lora_hardware_t;

/**
 * @brief Regulatory region structure
 */
typedef struct {
    char id[LORA_BAND_ID_LEN];     ///< Region ID (e.g., "EU", "US")
    char name[LORA_BAND_NAME_LEN]; ///< Human-readable name
} lora_region_t;

/**
 * @brief Compliance rule structure
 */
typedef struct {
    char region_id[LORA_BAND_ID_LEN];   ///< Region ID
    char hardware_id[LORA_BAND_ID_LEN]; ///< Hardware ID
    uint32_t freq_min_khz;              ///< Legal minimum frequency in kHz
    uint32_t freq_max_khz;              ///< Legal maximum frequency in kHz
    int8_t max_power_dbm;               ///< Maximum legal TX power
    uint8_t duty_cycle_percent;         ///< Duty cycle limit (0 = no limit)
    bool fhss_required;                 ///< FHSS required
    bool lbt_required;                  ///< Listen-before-talk required
} lora_compliance_t;

/**
 * @brief Initialize regulatory system from embedded JSON
 * @return ESP_OK on success
 */
esp_err_t lora_regulatory_init(void);

/**
 * @brief Get number of hardware profiles
 * @return Number of hardware profiles
 */
int lora_hardware_get_count(void);

/**
 * @brief Get hardware profile by index
 * @param index Hardware index (0 to count-1)
 * @return Pointer to hardware profile or NULL
 */
const lora_hardware_t *lora_hardware_get_profile(int index);

/**
 * @brief Get hardware profile by ID
 * @param id Hardware ID string
 * @return Pointer to hardware profile or NULL
 */
const lora_hardware_t *lora_hardware_get_profile_by_id(const char *id);

/**
 * @brief Regulatory domain validation and limits
 */
bool lora_regulatory_validate_domain(const char *domain);
const lora_compliance_t *lora_regulatory_get_limits(const char *domain, const char *hardware_id);
int lora_regulatory_get_region_count(void);
const lora_region_t *lora_regulatory_get_region(int index);

/**
 * @brief Get hardware index for a given frequency
 * @param frequency_hz Frequency in Hz
 * @return Hardware index or -1 if not supported
 */
int lora_hardware_get_index_by_frequency(uint32_t frequency_hz);

/**
 * @brief Get number of regulatory regions
 * @return Number of regions
 */
int lora_regulatory_get_region_count(void);

/**
 * @brief Get region by index
 * @param index Region index (0 to count-1)
 * @return Pointer to region or NULL
 */
const lora_region_t *lora_regulatory_get_region(int index);

/**
 * @brief Get region by ID
 * @param region_id Region ID string
 * @return Pointer to region or NULL
 */
const lora_region_t *lora_regulatory_get_region_by_id(const char *region_id);

/**
 * @brief Get compliance rules for region+hardware combination
 * @param region_id Region ID
 * @param hardware_id Hardware ID
 * @return Pointer to compliance rules or NULL
 */
const lora_compliance_t *lora_regulatory_get_compliance(const char *region_id, const char *hardware_id);

/**
 * @brief Get available hardware for a region
 * @param region_id Region ID
 * @param hardware_ids Array to store hardware IDs
 * @param max_count Maximum number of hardware IDs to return
 * @return Number of available hardware profiles
 */
int lora_regulatory_get_available_hardware(const char *region_id, const char **hardware_ids, int max_count);

#ifdef __cplusplus
}
#endif
