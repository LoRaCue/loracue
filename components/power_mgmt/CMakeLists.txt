idf_component_register(
    SRCS "power_mgmt.c" "power_mgmt_config.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_pm driver esp_timer freertos nvs_flash
    PRIV_REQUIRES bsp oled_ui
)
