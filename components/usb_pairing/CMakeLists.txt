idf_component_register(
    SRCS "usb_pairing.c"
    INCLUDE_DIRS "include"
    REQUIRES usb_cdc device_config device_registry json esp_timer freertos usb espressif__usb_host_cdc_acm espressif__esp_tinyusb
)
