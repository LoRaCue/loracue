if(CONFIG_BT_ENABLED)
    if(CONFIG_BT_NIMBLE_ENABLED)
        # NimBLE implementation
        set(SRCS "bluetooth_config.c" "ble_ota.c")
        set(REQUIRES commands general_config nvs_flash ota_engine bsp bt nimble)
    else()
        # Bluedroid implementation (legacy, kept for reference)
        set(SRCS "bluetooth_config_bluedroid.c.bak" "ble_ota.c")
        set(REQUIRES commands general_config nvs_flash ota_engine bsp bt)
    endif()
else()
    # Bluetooth disabled - use stub
    set(SRCS "bluetooth_config_stub.c")
    set(REQUIRES)
endif()

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "include"
    REQUIRES ${REQUIRES}
)

# Link BLE libraries with proper symbol resolution
if(CONFIG_BT_ENABLED)
    if(CONFIG_BT_NIMBLE_ENABLED)
        # NimBLE requires these components
        target_link_libraries(${COMPONENT_LIB} INTERFACE 
            "-Wl,--whole-archive" 
            idf::bt 
            idf::nimble
            "-Wl,--no-whole-archive"
        )
    else()
        # Bluedroid requires whole-archive linking
        target_link_libraries(${COMPONENT_LIB} INTERFACE 
            "-Wl,--whole-archive" 
            idf::bt 
            "-Wl,--no-whole-archive"
        )
    endif()
endif()
