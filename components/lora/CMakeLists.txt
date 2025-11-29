idf_component_register(
    SRCS "lora_driver.c" "lora_protocol.c" "lora_bands.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash mbedtls driver json bsp device_registry sx126x general_config power_mgmt
    EMBED_FILES "lora_bands.json" "lora_presets.json"
)
