file(GLOB GENERATED_SOURCES "generated/*.c")

set(INCLUDE_DIRS "include")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated")
    list(APPEND INCLUDE_DIRS "generated")
endif()

idf_component_register(
    SRCS 
        "ui_compact.c"
        "ui_compact_statusbar.c"
        "ui_components.c"
        "ui_navigator.c"
        "screens/screen_boot.c"
        "screens/screen_main.c"
        "screens/screen_pc_mode.c"
        "screens/screen_menu.c"
        "screens/screen_info.c"
        "screens/screen_factory_reset.c"
        "screens/screen_device_mode.c"
        "screens/screen_slot.c"
        "screens/screen_contrast.c"
        "screens/screen_pairing.c"
        "screens/screen_device_registry.c"
        "screens/screen_config_mode.c"
        "screens/screen_bluetooth.c"
        "screens/screen_lora_submenu.c"
        "screens/screen_lora_presets.c"
        "screens/screen_lora_frequency.c"
        "screens/screen_lora_sf.c"
        "screens/screen_lora_bw.c"
        "screens/screen_lora_cr.c"
        "screens/screen_lora_txpower.c"
        "screens/screen_lora_regulatory_domain.c"
        "screens/screen_lora_band.c"
        ${GENERATED_SOURCES}
    INCLUDE_DIRS 
        ${INCLUDE_DIRS}
    REQUIRES 
        ui_lvgl
        input_manager
        config_manager
        power_mgmt
        ble
        bsp
        lora
        presenter_mode_manager
        usb_pairing
)
