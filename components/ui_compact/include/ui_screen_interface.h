#pragma once

#include "input_manager.h"
#include "lvgl.h"
#include "ui_compact.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interface for a UI screen (State Pattern)
 */
typedef struct ui_screen_t ui_screen_t;

struct ui_screen_t {
    /**
     * @brief Unique screen type identifier
     */
    ui_screen_type_t type;

    /**
     * @brief Create the screen's UI elements
     *
     * @param parent The parent LVGL object (usually the screen object)
     */
    void (*create)(lv_obj_t *parent);

    /**
     * @brief Destroy the screen and free resources
     *
     * Optional: If NULL, standard lv_obj_del of the parent is sufficient.
     * Use this if you allocated additional dynamic memory.
     */
    void (*destroy)(void);

    /**
     * @brief Handle input events
     *
     * @param event The input event type
     */
    void (*handle_input_event)(input_event_t event);

    /**
     * @brief Called when the screen becomes active (after creation/transition)
     *
     * Optional.
     */
    void (*on_enter)(void);

    /**
     * @brief Called before the screen is destroyed/left
     *
     * Optional.
     */
    void (*on_exit)(void);
};

#ifdef __cplusplus
}
#endif
