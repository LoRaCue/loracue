#pragma once

#include "esp_err.h"
#include "ui_compact_statusbar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_SCREEN_BOOT,
    UI_SCREEN_MAIN,
    UI_SCREEN_PC_MODE,
    UI_SCREEN_MENU,
    UI_SCREEN_SETTINGS,
    UI_SCREEN_OTA,
    UI_SCREEN_PAIRING,
    UI_SCREEN_BATTERY,
    UI_SCREEN_DEVICE_INFO,
    UI_SCREEN_SYSTEM_INFO,
    UI_SCREEN_FACTORY_RESET,
    UI_SCREEN_DEVICE_MODE,
    UI_SCREEN_SLOT,
    UI_SCREEN_LORA_SUBMENU,
    UI_SCREEN_LORA_PRESETS,
    UI_SCREEN_LORA_FREQUENCY,
    UI_SCREEN_LORA_SF,
    UI_SCREEN_LORA_BW,
    UI_SCREEN_LORA_CR,
    UI_SCREEN_LORA_TXPOWER,
    UI_SCREEN_LORA_BAND,
    UI_SCREEN_DEVICE_REGISTRY,
    UI_SCREEN_CONTRAST,
    UI_SCREEN_BLUETOOTH,
    UI_SCREEN_CONFIG_MODE
} ui_screen_type_t;

/**
 * @brief Initialize compact UI for small displays
 *
 * Initializes LVGL core and compact UI components optimized
 * for small displays (128x64, 240x135, etc.)
 *
 * @return ESP_OK on success
 */
esp_err_t ui_compact_init(void);

/**
 * @brief Show boot screen
 *
 * @return ESP_OK on success
 */
esp_err_t ui_compact_show_boot_screen(void);

/**
 * @brief Show main screen
 *
 * @return ESP_OK on success
 */
esp_err_t ui_compact_show_main_screen(void);

/**
 * @brief Register button event callback
 *
 * Must be called AFTER all other components that register button callbacks
 *
 * @return ESP_OK on success
 */
esp_err_t ui_compact_register_button_callback(void);

/**
 * @brief Get current status bar data
 *
 * @param status Pointer to status structure to fill
 */
void ui_compact_get_status(statusbar_data_t *status);

#ifdef __cplusplus
}
#endif
