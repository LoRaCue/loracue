idf_component_register(
    SRCS "firmware_manifest.c" "ota_compatibility.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp
    LDFRAGMENTS "linker.lf"
)

# Force manifest symbol to be included for predictable placement
target_link_options(${COMPONENT_LIB} INTERFACE
    "-Wl,--undefined=firmware_manifest"
)
