idf_component_register(
    SRCS "usb_pairing.c"
    INCLUDE_DIRS "include"
    REQUIRES device_registry esp_timer freertos mbedtls
)
