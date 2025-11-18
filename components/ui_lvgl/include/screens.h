#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Screen creation functions
void screen_boot_create(lv_obj_t *parent);
void screen_main_create(lv_obj_t *parent);
void screen_menu_create(lv_obj_t *parent);
void screen_settings_create(lv_obj_t *parent);
void screen_ota_create(lv_obj_t *parent);
void screen_pairing_create(lv_obj_t *parent);

// Screen update functions
void screen_main_update_mode(const char *mode);
void screen_main_update_status(const char *status);
void screen_ota_update_progress(uint8_t percent);

#ifdef __cplusplus
}
#endif
