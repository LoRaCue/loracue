idf_component_register(
    SRCS "ota_engine.c"
    INCLUDE_DIRS "include"
    REQUIRES app_update mbedtls
)
