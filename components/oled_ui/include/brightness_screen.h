#pragma once

#include "menu_screen.h"
#include <stdbool.h>

void brightness_screen_init(void);
void brightness_screen_draw(void);
void brightness_screen_navigate(menu_direction_t direction);
void brightness_screen_select(void);
bool brightness_screen_is_edit_mode(void);
