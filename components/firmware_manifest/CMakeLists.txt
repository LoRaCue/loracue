idf_component_register(
    SRCS "firmware_manifest.c" "ota_compatibility.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp
)

# Place manifest at fixed offset 0x100 in app binary
target_linker_script(${COMPONENT_LIB} INTERFACE "manifest.ld")


# Force manifest symbol to be included for predictable placement
target_link_options(${COMPONENT_LIB} INTERFACE
    "-Wl,--undefined=firmware_manifest"
)
