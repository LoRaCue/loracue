idf_component_register(
    SRCS "oled_ui.c" "icons/ui_battery.c" "icons/ui_usb.c" "icons/ui_rf.c" "ui_status_bar.c" "ui_screen_controller.c" "ui_data_provider.c" "ui_monitor_task.c" "screens/boot_screen.c" "screens/main_screen.c" "screens/presenter_main_screen.c" "screens/pc_main_screen.c" "screens/info_screens.c" "screens/menu_screen.c" "screens/device_mode_screen.c" "screens/lora_settings_screen.c" "screens/config_mode_screen.c"
    INCLUDE_DIRS "include"
    REQUIRES power_mgmt freertos u8g2 u8g2-hal-esp-idf driver esp_driver_i2c bsp lora device_config config_wifi_server
)
