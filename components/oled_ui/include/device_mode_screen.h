#pragma once

#include "general_config.h"
#include "menu_screen.h"

void device_mode_screen_draw(void);
void device_mode_screen_navigate(menu_direction_t direction);
void device_mode_screen_select(void);
device_mode_t device_mode_get_current(void);
void device_mode_screen_reset(void);
