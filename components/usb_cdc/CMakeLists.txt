idf_component_register(
    SRCS "usb_cdc.c" "usb_console.c"
    INCLUDE_DIRS "include"
    REQUIRES "commands" "espressif__esp_tinyusb"
)
