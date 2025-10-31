#pragma once

#include "menu_screen.h"
#include <stdbool.h>

void slot_screen_init(void);
void slot_screen_draw(void);
void slot_screen_navigate(menu_direction_t direction);
void slot_screen_select(void);
bool slot_screen_is_edit_mode(void);
