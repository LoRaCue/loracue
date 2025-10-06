idf_component_register(
    SRCS "bluetooth_config.c"
    INCLUDE_DIRS "include"
    REQUIRES bt commands device_config nvs_flash
)

# Explicitly link BLE libraries for CI compatibility
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--whole-archive" idf::bt "-Wl,--no-whole-archive")
