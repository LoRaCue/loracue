#pragma once

typedef enum { MENU_UP, MENU_DOWN } menu_direction_t;

typedef enum {
    MENU_DEVICE_MODE = 0,
    MENU_BATTERY_STATUS,
    MENU_LORA_SETTINGS,
    MENU_DEVICE_PAIRING,
    MENU_DEVICE_REGISTRY,
    MENU_BRIGHTNESS,
    MENU_CONFIG_MODE,
    MENU_DEVICE_INFO,
    MENU_SYSTEM_INFO,
    MENU_FACTORY_RESET
} menu_item_t;

/**
 * @brief Draw menu screen
 */
void menu_screen_draw(void);

/**
 * @brief Navigate menu selection
 * @param direction Direction to navigate
 */
void menu_screen_navigate(menu_direction_t direction);

/**
 * @brief Get currently selected menu item
 * @return Selected menu item index
 */
int menu_screen_get_selected(void);

/**
 * @brief Reset menu to first item
 */
void menu_screen_reset(void);
