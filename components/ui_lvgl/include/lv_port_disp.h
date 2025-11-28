#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL display port
 * 
 * @return lv_display_t* Display object or NULL on error
 */
lv_display_t *lv_port_disp_init(void);

/**
 * @brief Deinitialize LVGL display port
 */
void lv_port_disp_deinit(void);

#ifdef __cplusplus
}
#endif
