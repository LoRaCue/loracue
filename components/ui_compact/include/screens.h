#pragma once

#include "lvgl.h"
#include "ui_compact_statusbar.h"
#include "ui_screen_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create boot screen
 * @param parent Parent object to create screen in
 */
void screen_boot_create(lv_obj_t *parent);
ui_screen_t *screen_boot_get_interface(void);

/**
 * @brief Create main screen
 * @param parent Parent object to create screen in
 * @param initial_status Initial status data (NULL for defaults)
 */
void screen_main_create(lv_obj_t *parent, const statusbar_data_t *initial_status);

/**
 * @brief Update device name on main screen
 * @param device_name Device name to display
 */
void screen_main_update_device_name(const char *device_name);
ui_screen_t *screen_main_get_interface(void);

/**
 * @brief Create PC mode screen
 * @param parent Parent object to create screen in
 * @param initial_status Initial status data (NULL for defaults)
 */
void screen_pc_mode_create(lv_obj_t *parent, const statusbar_data_t *initial_status);
ui_screen_t *screen_pc_mode_get_interface(void);

/**
 * @brief Main menu items
 */
typedef enum {
    MAIN_MENU_DEVICE_MODE = 0,
    MAIN_MENU_SLOT,
    MAIN_MENU_LORA,
    MAIN_MENU_PAIRING,
    MAIN_MENU_REGISTRY,
    MAIN_MENU_BRIGHTNESS,
    MAIN_MENU_BLUETOOTH,
    MAIN_MENU_CONFIG,
    MAIN_MENU_DEVICE_INFO,
    MAIN_MENU_SYSTEM_INFO,
    MAIN_MENU_FACTORY_RESET,
    MAIN_MENU_COUNT
} main_menu_item_t;

/**
 * @brief Main menu item labels
 */
extern const char *MAIN_MENU_ITEMS[MAIN_MENU_COUNT];

/**
 * @brief Create menu screen
 * @param parent Parent object to create screen in
 */
void screen_menu_create(lv_obj_t *parent);

/**
 * @brief Navigate down in menu
 */
void screen_menu_navigate_down(void);

/**
 * @brief Get selected menu item index
 * @return Selected item index
 */
int screen_menu_get_selected(void);

/**
 * @brief Reset menu to first item
 */
void screen_menu_reset(void);

/**
 * @brief Get selected menu item name
 * @return Selected item name or NULL
 */
const char *screen_menu_get_selected_name(void);
ui_screen_t *screen_menu_get_interface(void);

/**
 * @brief Create system info screen
 * @param parent Parent object to create screen in
 */
void screen_system_info_create(lv_obj_t *parent);

/**
 * @brief Create device info screen
 * @param parent Parent object to create screen in
 */
void screen_device_info_create(lv_obj_t *parent);
ui_screen_t *screen_device_info_get_interface(void);
ui_screen_t *screen_system_info_get_interface(void);

/**
 * @brief Create battery status screen
 * @param parent Parent object to create screen in
 */
void screen_battery_status_create(lv_obj_t *parent);
ui_screen_t *screen_battery_status_get_interface(void);

/**
 * @brief Create factory reset screen
 * @param parent Parent object to create screen in
 */
void screen_factory_reset_create(lv_obj_t *parent);

/**
 * @brief Check hold button for factory reset
 */
void screen_factory_reset_check_hold(bool button_pressed);

/**
 * @brief Reset factory reset screen
 */
void screen_factory_reset_reset(void);
ui_screen_t *screen_factory_reset_get_interface(void);

// LoRa Frequency screen
void screen_lora_frequency_init(void);
void screen_lora_frequency_create(lv_obj_t *parent);
void screen_lora_frequency_select(void);
bool screen_lora_frequency_is_edit_mode(void);
void screen_lora_frequency_reset(void);

// LoRa Spreading Factor screen
void screen_lora_sf_init(void);
void screen_lora_sf_create(lv_obj_t *parent);
void screen_lora_sf_navigate_down(void);
void screen_lora_sf_navigate_up(void);
void screen_lora_sf_select(void);
bool screen_lora_sf_is_edit_mode(void);
void screen_lora_sf_reset(void);

// LoRa Bandwidth screen
void screen_lora_bw_init(void);
void screen_lora_bw_create(lv_obj_t *parent);
void screen_lora_bw_navigate_down(void);
void screen_lora_bw_navigate_up(void);
void screen_lora_bw_select(void);
bool screen_lora_bw_is_edit_mode(void);

// LoRa Coding Rate screen
void screen_lora_cr_init(void);
void screen_lora_cr_create(lv_obj_t *parent);
void screen_lora_cr_navigate_down(void);
void screen_lora_cr_navigate_up(void);
void screen_lora_cr_select(void);
bool screen_lora_cr_is_edit_mode(void);

// LoRa TX Power screen
void screen_lora_txpower_init(void);
void screen_lora_txpower_create(lv_obj_t *parent);
void screen_lora_txpower_increment(void);
void screen_lora_txpower_select(void);
bool screen_lora_txpower_is_edit_mode(void);

// LoRa Band screen
void screen_lora_band_init(void);
void screen_lora_band_create(lv_obj_t *parent);
void screen_lora_band_navigate_down(void);
void screen_lora_band_navigate_up(void);
void screen_lora_band_select(void);
bool screen_lora_band_is_edit_mode(void);

/**
 * @brief Create device mode selection screen
 * @param parent Parent object to create screen in
 */
void screen_device_mode_create(lv_obj_t *parent);

/**
 * @brief Navigate down in device mode screen
 */
void screen_device_mode_navigate_down(void);

/**
 * @brief Select device mode
 */
void screen_device_mode_select(void);

/**
 * @brief Reset device mode screen
 */
void screen_device_mode_reset(void);
ui_screen_t *screen_device_mode_get_interface(void);

/**
 * @brief Create slot screen
 * @param parent Parent object to create screen in
 */
void screen_slot_create(lv_obj_t *parent);

/**
 * @brief Initialize slot screen
 */
void screen_slot_init(void);
ui_screen_t *screen_slot_get_interface(void);

/**
 * @brief Navigate down in slot screen
 */
void screen_slot_navigate_down(void);

/**
 * @brief Navigate up in slot screen
 */
void screen_slot_navigate_up(void);

/**
 * @brief Select slot
 */
void screen_slot_select(void);

/**
 * @brief Check if slot screen is in edit mode
 * @return true if in edit mode
 */
bool screen_slot_is_edit_mode(void);

/**
 * @brief LoRa submenu items
 */
typedef enum {
    LORA_MENU_PRESETS = 0,
    LORA_MENU_FREQUENCY,
    LORA_MENU_SF,
    LORA_MENU_BW,
    LORA_MENU_CR,
    LORA_MENU_TXPOWER,
    LORA_MENU_BAND,
    LORA_MENU_COUNT
} lora_menu_item_t;

/**
 * @brief LoRa submenu item labels
 */
extern const char *LORA_MENU_ITEMS[LORA_MENU_COUNT];

/**
 * @brief Create LoRa submenu screen
 * @param parent Parent object to create screen in
 */
void screen_lora_submenu_create(lv_obj_t *parent);

/**
 * @brief Navigate down in LoRa submenu
 */
void screen_lora_submenu_navigate_down(void);

/**
 * @brief Select LoRa submenu item
 */
void screen_lora_submenu_select(void);

/**
 * @brief Get selected LoRa submenu item index
 */
int screen_lora_submenu_get_selected(void);

/**
 * @brief Reset LoRa submenu
 */
void screen_lora_submenu_reset(void);
ui_screen_t *screen_lora_submenu_get_interface(void);

// LoRa Presets screen
void screen_lora_presets_create(lv_obj_t *parent);
void screen_lora_presets_navigate_down(void);
void screen_lora_presets_select(void);
void screen_lora_presets_reset(void);
ui_screen_t *screen_lora_presets_get_interface(void);
void screen_lora_presets_init(void);

// LoRa Frequency Screen
void screen_lora_frequency_init(void);
void screen_lora_frequency_create(lv_obj_t *parent);
void screen_lora_frequency_navigate_down(void);
void screen_lora_frequency_navigate_up(void);
void screen_lora_frequency_select(void);
bool screen_lora_frequency_is_edit_mode(void);
void screen_lora_frequency_reset(void);
ui_screen_t *screen_lora_frequency_get_interface(void);

// LoRa Spreading Factor Screen
void screen_lora_sf_init(void);
void screen_lora_sf_create(lv_obj_t *parent);
void screen_lora_sf_navigate_down(void);
void screen_lora_sf_select(void);
bool screen_lora_sf_is_edit_mode(void);
void screen_lora_sf_reset(void);
ui_screen_t *screen_lora_sf_get_interface(void);

// LoRa Bandwidth Screen
void screen_lora_bw_init(void);
void screen_lora_bw_create(lv_obj_t *parent);
void screen_lora_bw_navigate_down(void);
void screen_lora_bw_select(void);
bool screen_lora_bw_is_edit_mode(void);
void screen_lora_bw_reset(void);
ui_screen_t *screen_lora_bw_get_interface(void);

void screen_lora_cr_init(void);
void screen_lora_cr_create(lv_obj_t *parent);
void screen_lora_cr_navigate_down(void);
void screen_lora_cr_select(void);
bool screen_lora_cr_is_edit_mode(void);
void screen_lora_cr_reset(void);
ui_screen_t *screen_lora_cr_get_interface(void);

void screen_lora_txpower_init(void);
void screen_lora_txpower_create(lv_obj_t *parent);
void screen_lora_txpower_navigate_down(void);
void screen_lora_txpower_navigate_up(void);
void screen_lora_txpower_select(void);
bool screen_lora_txpower_is_edit_mode(void);
void screen_lora_txpower_reset(void);
ui_screen_t *screen_lora_txpower_get_interface(void);

void screen_lora_band_init(void);
void screen_lora_band_create(lv_obj_t *parent);
void screen_lora_band_navigate_down(void);
void screen_lora_band_select(void);
void screen_lora_band_reset(void);
ui_screen_t *screen_lora_band_get_interface(void);

/**
 * @brief Create pairing screen
 * @param parent Parent object to create screen in
 */
void screen_pairing_create(lv_obj_t *parent);

/**
 * @brief Reset pairing screen
 */
void screen_pairing_reset(void);
ui_screen_t *screen_pairing_get_interface(void);

/**
 * @brief Create device registry screen
 * @param parent Parent object to create screen in
 */
void screen_device_registry_create(lv_obj_t *parent);

/**
 * @brief Reset device registry screen
 */
void screen_device_registry_reset(void);
ui_screen_t *screen_device_registry_get_interface(void);

/**
 * @brief Create brightness screen
 * @param parent Parent object to create screen in
 */
void screen_brightness_create(lv_obj_t *parent);

/**
 * @brief Initialize brightness screen
 */
void screen_brightness_init(void);
ui_screen_t *screen_brightness_get_interface(void);

/**
 * @brief Navigate down in brightness screen
 */
void screen_brightness_navigate_down(void);

/**
 * @brief Navigate up in brightness screen
 */
void screen_brightness_navigate_up(void);

/**
 * @brief Select brightness
 */
void screen_brightness_select(void);

/**
 * @brief Check if brightness screen is in edit mode
 * @return true if in edit mode
 */
bool screen_brightness_is_edit_mode(void);

/**
 * @brief Create bluetooth screen
 * @param parent Parent object to create screen in
 */
void screen_bluetooth_create(lv_obj_t *parent);

/**
 * @brief Navigate down in bluetooth screen
 */
void screen_bluetooth_navigate_down(void);

/**
 * @brief Select bluetooth option
 */
void screen_bluetooth_select(void);

/**
 * @brief Reset bluetooth screen
 */
void screen_bluetooth_reset(void);
ui_screen_t *screen_bluetooth_get_interface(void);

/**
 * @brief Create config mode screen
 * @param parent Parent object to create screen in
 */
void screen_config_mode_create(lv_obj_t *parent);

/**
 * @brief Reset config mode screen
 */
void screen_config_mode_reset(void);
ui_screen_t *screen_config_mode_get_interface(void);

#ifdef __cplusplus
}
#endif
