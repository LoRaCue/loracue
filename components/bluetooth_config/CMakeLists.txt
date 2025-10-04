# Only build if Bluetooth is enabled
if(CONFIG_BT_ENABLED)
    idf_component_register(
        SRCS "bluetooth_config.c"
        INCLUDE_DIRS "include"
        REQUIRES bt commands device_config nvs_flash
    )
else()
    # Register empty component to avoid build errors
    idf_component_register()
endif()
