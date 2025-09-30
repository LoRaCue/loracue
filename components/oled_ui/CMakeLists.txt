idf_component_register(
    SRCS "oled_ui.c"
    INCLUDE_DIRS "include"
    REQUIRES power_mgmt freertos u8g2 u8g2-hal-esp-idf driver esp_driver_i2c
)
