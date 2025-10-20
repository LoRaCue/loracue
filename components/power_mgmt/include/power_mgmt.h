/**
 * @file power_mgmt.h
 * @brief Power management for LoRaCue with sleep modes and battery optimization
 *
 * CONTEXT: Ticket 4.2 - Power Management
 * PURPOSE: Light sleep, deep sleep, wake sources, battery life >24h
 * TARGET: <10mA active, <1mA light sleep, <10µA deep sleep
 */

#pragma once

#include "esp_err.h"
#include "esp_sleep.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power management modes
 */
typedef enum {
    POWER_MODE_ACTIVE,        ///< Full power, all peripherals active
    POWER_MODE_DISPLAY_SLEEP, ///< Display off, CPU active
    POWER_MODE_LIGHT_SLEEP,   ///< Light sleep, wake on button/timer
    POWER_MODE_DEEP_SLEEP,    ///< Deep sleep, wake on button only
} power_mode_t;

/**
 * @brief Power management configuration
 */
typedef struct {
    uint32_t display_sleep_timeout_ms; ///< Timeout for display sleep (default: 10s)
    uint32_t light_sleep_timeout_ms;   ///< Timeout for light sleep (default: 30s)
    uint32_t deep_sleep_timeout_ms;    ///< Timeout for deep sleep (default: 5min)
    bool enable_auto_display_sleep;    ///< Enable automatic display sleep
    bool enable_auto_light_sleep;      ///< Enable automatic light sleep
    bool enable_auto_deep_sleep;       ///< Enable automatic deep sleep
    uint8_t cpu_freq_mhz;              ///< CPU frequency in MHz (80/160/240)
} power_config_t;

/**
 * @brief Power statistics
 */
typedef struct {
    uint32_t active_time_ms;        ///< Total active time
    uint32_t display_sleep_time_ms; ///< Total display sleep time
    uint32_t light_sleep_time_ms;   ///< Total light sleep time
    uint32_t deep_sleep_time_ms;    ///< Total deep sleep time
    uint32_t wake_count_button;     ///< Wake count from buttons
    uint32_t wake_count_timer;      ///< Wake count from timer
    float estimated_battery_hours;  ///< Estimated battery life remaining
} power_stats_t;

/**
 * @brief Initialize power management
 *
 * @param config Power management configuration
 * @return ESP_OK on success
 */
esp_err_t power_mgmt_init(const power_config_t *config);

/**
 * @brief Enter display sleep mode (display off, CPU active)
 *
 * @return ESP_OK on success
 */
esp_err_t power_mgmt_display_sleep(void);

/**
 * @brief Enter light sleep mode
 *
 * @param timeout_ms Wake timeout in milliseconds (0 = indefinite)
 * @return ESP_OK on success
 */
esp_err_t power_mgmt_light_sleep(uint32_t timeout_ms);

/**
 * @brief Enter deep sleep mode
 *
 * @param timeout_ms Wake timeout in milliseconds (0 = indefinite)
 * @return ESP_OK on success (function doesn't return on deep sleep)
 */
esp_err_t power_mgmt_deep_sleep(uint32_t timeout_ms);

/**
 * @brief Update activity timestamp (prevents auto-sleep)
 *
 * @return ESP_OK on success
 */
esp_err_t power_mgmt_update_activity(void);

/**
 * @brief Check if system should enter sleep mode
 *
 * @return Recommended power mode based on inactivity
 */
power_mode_t power_mgmt_get_recommended_mode(void);

/**
 * @brief Get power management statistics
 *
 * @param stats Output power statistics
 * @return ESP_OK on success
 */
esp_err_t power_mgmt_get_stats(power_stats_t *stats);

/**
 * @brief Set CPU frequency for power optimization
 *
 * @param freq_mhz CPU frequency in MHz (80/160/240)
 * @return ESP_OK on success
 */
esp_err_t power_mgmt_set_cpu_freq(uint8_t freq_mhz);

/**
 * @brief Get wake reason from last sleep
 *
 * @return ESP sleep wake cause
 */
esp_sleep_wakeup_cause_t power_mgmt_get_wake_cause(void);

/**
 * @brief Prepare system for sleep (disable peripherals)
 *
 * @return ESP_OK on success
 */
esp_err_t power_mgmt_prepare_sleep(void);

/**
 * @brief Restore system after wake (re-enable peripherals)
 *
 * @return ESP_OK on success
 */
esp_err_t power_mgmt_restore_wake(void);

#ifdef __cplusplus
}
#endif
