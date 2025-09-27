#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BSP interface for hardware abstraction
 */
typedef struct {
    esp_err_t (*init)(void);
    esp_err_t (*lora_init)(void);
    esp_err_t (*display_init)(void);
    esp_err_t (*buttons_init)(void);
    float (*read_battery)(void);
    void (*enter_sleep)(void);
} bsp_interface_t;

/**
 * @brief Get the BSP interface for the current board
 * @return Pointer to BSP interface
 */
const bsp_interface_t* bsp_get_interface(void);

/**
 * @brief Initialize the board support package
 * @return ESP_OK on success
 */
esp_err_t bsp_init(void);

#ifdef __cplusplus
}
#endif
