#pragma once

#include "menu_screen.h"
#include <stdbool.h>

void lora_txpower_screen_init(void);
void lora_txpower_screen_draw(void);
void lora_txpower_screen_navigate(menu_direction_t direction);
void lora_txpower_screen_select(void);
bool lora_txpower_screen_is_edit_mode(void);
