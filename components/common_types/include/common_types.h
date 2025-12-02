#pragma once

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create mutex or fail with error
 *
 * Usage: CREATE_MUTEX_OR_FAIL(state_mutex);
 */
#define CREATE_MUTEX_OR_FAIL(mutex_var)                                                                                \
    do {                                                                                                               \
        (mutex_var) = xSemaphoreCreateMutex();                                                                         \
        if (!(mutex_var)) {                                                                                            \
            ESP_LOGE(TAG, "Failed to create mutex: " #mutex_var);                                                      \
            return ESP_ERR_NO_MEM;                                                                                     \
        }                                                                                                              \
    } while (0)

/**
 * @brief Check if component is initialized
 *
 * Usage: CHECK_INITIALIZED(protocol_initialized, "LoRa protocol");
 */
#define CHECK_INITIALIZED(flag, component_name)                                                                        \
    do {                                                                                                               \
        if (!(flag)) {                                                                                                 \
            ESP_LOGE(TAG, component_name " not initialized");                                                          \
            return ESP_ERR_INVALID_STATE;                                                                              \
        }                                                                                                              \
    } while (0)

/**
 * @brief Log error and return error code
 *
 * Usage: LOG_ERROR_RETURN(ret, "initialize SPI bus");
 */
#define LOG_ERROR_RETURN(ret, action)                                                                                  \
    do {                                                                                                               \
        ESP_LOGE(TAG, "Failed to " action ": %s", esp_err_to_name(ret));                                               \
        return ret;                                                                                                    \
    } while (0)

/**
 * @brief Validate argument and return error if invalid
 *
 * Usage: VALIDATE_ARG(config);
 */
#define VALIDATE_ARG(arg)                                                                                              \
    do {                                                                                                               \
        if (!(arg)) {                                                                                                  \
            ESP_LOGE(TAG, "Invalid argument: " #arg);                                                                  \
            return ESP_ERR_INVALID_ARG;                                                                                \
        }                                                                                                              \
    } while (0)

/**
 * @brief Validate pointer and length
 *
 * Usage: VALIDATE_PTR_AND_LEN(data, length);
 */
#define VALIDATE_PTR_AND_LEN(ptr, len)                                                                                 \
    do {                                                                                                               \
        VALIDATE_ARG(ptr);                                                                                             \
        if ((len) == 0) {                                                                                              \
            ESP_LOGE(TAG, "Invalid length: " #len);                                                                    \
            return ESP_ERR_INVALID_ARG;                                                                                \
        }                                                                                                              \
    } while (0)

/**
 * @brief Validate buffer with bounds checking
 *
 * Usage: VALIDATE_BUFFER(data, length, max_length);
 */
#define VALIDATE_BUFFER(ptr, len, max_len)                                                                             \
    do {                                                                                                               \
        VALIDATE_ARG(ptr);                                                                                             \
        if ((len) == 0 || (len) > (max_len)) {                                                                         \
            ESP_LOGE(TAG, "Invalid buffer length: " #len);                                                             \
            return ESP_ERR_INVALID_ARG;                                                                                \
        }                                                                                                              \
    } while (0)

/**
 * @brief Get current time in milliseconds
 *
 * @return Current time in milliseconds
 */
static inline uint32_t get_time_ms(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/**
 * @brief Pack two bytes into uint16_t (little-endian)
 *
 * @param low Low byte
 * @param high High byte
 * @return Packed uint16_t value
 */
static inline uint16_t pack_u16_le(uint8_t low, uint8_t high)
{
    return (uint16_t)low | ((uint16_t)high << 8);
}

/**
 * @brief Pack four bytes into uint32_t (little-endian)
 *
 * @param b0 Byte 0 (LSB)
 * @param b1 Byte 1
 * @param b2 Byte 2
 * @param b3 Byte 3 (MSB)
 * @return Packed uint32_t value
 */
static inline uint32_t pack_u32_le(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    return (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
}

/**
 * @brief Unpack uint16_t into two bytes (little-endian)
 *
 * @param val Value to unpack
 * @param low Output low byte
 * @param high Output high byte
 */
static inline void unpack_u16_le(uint16_t val, uint8_t *low, uint8_t *high)
{
    *low  = val & 0xFF;
    *high = (val >> 8) & 0xFF;
}

/**
 * @brief Command history entry for PC mode
 */
typedef struct {
    uint32_t timestamp_ms;
    uint16_t device_id;
    char device_name[16];
    char command[8];
    uint8_t keycode;
    uint8_t modifiers;
} command_history_entry_t;

#ifdef __cplusplus
}
#endif
