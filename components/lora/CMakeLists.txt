idf_component_register(
    SRCS "lora_driver.c" "lora_protocol.c" "lora_bands.c"
    INCLUDE_DIRS "include"
    REQUIRES mbedtls driver json bsp device_registry sx126x config_manager power_mgmt common_types
    EMBED_FILES "lora_regulatory.json" "lora_presets.json"
)
