idf_component_register(
    SRCS "firmware_manifest.c" "ota_compatibility.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp
)
