idf_component_register(
    SRCS "usb_hid.c" "usb_descriptors.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer freertos espressif__esp_tinyusb espressif__tinyusb general_config input_manager json device_registry bsp usb_cdc
)
