#pragma once

#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_RICH_SCREEN_HOME,
    UI_RICH_SCREEN_SETTINGS,
    UI_RICH_SCREEN_PRESENTER,
    UI_RICH_SCREEN_PC_MODE
} ui_rich_screen_t;

/**
 * @brief Initialize the rich UI system
 * @return ESP_OK on success
 */
esp_err_t ui_rich_init(void);

/**
 * @brief Navigate to a specific screen
 * @param screen Screen to navigate to
 */
void ui_rich_navigate(ui_rich_screen_t screen);

/**
 * @brief Get current active screen
 * @return Current screen
 */
ui_rich_screen_t ui_rich_get_current_screen(void);

#ifdef __cplusplus
}
#endif
