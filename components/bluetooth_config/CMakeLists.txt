idf_component_register(
    SRCS "bluetooth_config.c"
    INCLUDE_DIRS "include"
    REQUIRES bt commands device_config nvs_flash
)
