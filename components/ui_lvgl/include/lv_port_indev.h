#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL input device port (buttons)
 *
 * @return lv_indev_t* Input device object or NULL on error
 */
lv_indev_t *lv_port_indev_init(void);

/**
 * @brief Deinitialize LVGL input device port
 *
 * @param indev Input device to deinitialize
 */
void lv_port_indev_deinit(lv_indev_t *indev);

#ifdef __cplusplus
}
#endif
