idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES bsp lora usb_hid device_registry oled_ui button_manager led_manager power_mgmt usb_pairing web_config nvs_flash
)
