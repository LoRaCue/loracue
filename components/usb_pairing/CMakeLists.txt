idf_component_register(
    SRCS "usb_pairing.c"
    INCLUDE_DIRS "include"
    REQUIRES usb_cdc device_config json esp_timer freertos usb
)
