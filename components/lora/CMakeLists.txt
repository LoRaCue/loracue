idf_component_register(
    SRCS "lora_driver.c" "lora_protocol.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp mbedtls device_registry
)
