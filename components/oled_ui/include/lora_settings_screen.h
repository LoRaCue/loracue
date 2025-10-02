#pragma once

#include "ui_config.h"
#include "menu_screen.h"

/**
 * @brief Draw LoRa settings configuration screen
 */
void lora_settings_screen_draw(void);

/**
 * @brief Navigate in LoRa settings screen
 * @param direction Navigation direction
 */
void lora_settings_screen_navigate(menu_direction_t direction);

/**
 * @brief Select current LoRa preset
 */
void lora_settings_screen_select(void);

/**
 * @brief Reset LoRa settings screen state
 */
void lora_settings_screen_reset(void);
