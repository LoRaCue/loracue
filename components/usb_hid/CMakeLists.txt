idf_component_register(
    SRCS "usb_hid.c" "usb_descriptors.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer freertos espressif__esp_tinyusb device_config button_manager json device_registry usb_cdc bsp
)
