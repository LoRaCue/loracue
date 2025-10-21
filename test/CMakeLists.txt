idf_component_register(
    SRCS "test_lora_integration.c"
    INCLUDE_DIRS "."
    REQUIRES unity lora device_registry general_config power_mgmt
)
