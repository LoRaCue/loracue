idf_component_register(
    SRCS "power_mgmt.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_pm driver esp_timer freertos config_manager
    PRIV_REQUIRES bsp usb_hid ui_lvgl
)
