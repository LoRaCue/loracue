if(CONFIG_BT_ENABLED)
    set(SRCS "bluetooth_config.c" "ble_ota.c")
else()
    set(SRCS "bluetooth_config_stub.c")
endif()

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "include"
    REQUIRES commands general_config nvs_flash ota_engine bsp bt
)

# Explicitly link BLE libraries with whole-archive for proper symbol resolution
if(CONFIG_BT_ENABLED)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--whole-archive" idf::bt "-Wl,--no-whole-archive")
endif()
