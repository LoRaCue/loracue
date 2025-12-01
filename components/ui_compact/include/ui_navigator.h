#pragma once

#include "esp_err.h"
#include "ui_screen_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the UI navigator
 *
 * @return ESP_OK on success
 */
esp_err_t ui_navigator_init(void);

/**
 * @brief Switch to a specific screen
 *
 * Destroys the current screen and creates the new one.
 *
 * @param screen_type The type of screen to switch to
 * @return ESP_OK on success
 */
esp_err_t ui_navigator_switch_to(ui_screen_type_t screen_type);

/**
 * @brief Get the currently active screen type
 *
 * @return ui_screen_type_t Current screen type
 */
ui_screen_type_t ui_navigator_get_current_screen_type(void);

/**
 * @brief Handle input event for current screen
 *
 * Dispatches the event to the current screen's handle_input_event callback.
 *
 * @param event Input event
 */
void ui_navigator_handle_input_event(input_event_t event);

/**
 * @brief Register a screen implementation
 *
 * @param screen_impl Pointer to the screen implementation structure
 * @return ESP_OK on success
 */
esp_err_t ui_navigator_register_screen(ui_screen_type_t type, const ui_screen_t *screen_impl);

/**
 * @brief Callback for screen change events
 * @param type New screen type
 */
typedef void (*ui_navigator_screen_change_cb_t)(ui_screen_type_t type);

/**
 * @brief Set callback for screen change events
 * @param cb Callback function
 */
void ui_navigator_set_screen_change_callback(ui_navigator_screen_change_cb_t cb);

#ifdef __cplusplus
}
#endif
