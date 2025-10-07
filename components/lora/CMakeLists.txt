idf_component_register(
    SRCS "lora_driver.c" "lora_protocol.c" "lora_bands.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash mbedtls driver json bsp device_registry sx126x_lib
    EMBED_FILES "lora_bands.json"
)
