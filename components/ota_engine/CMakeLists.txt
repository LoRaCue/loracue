idf_component_register(
    SRCS "ota_engine.c"
    INCLUDE_DIRS "include"
    REQUIRES app_update mbedtls
    EMBED_FILES "${CMAKE_SOURCE_DIR}/keys/firmware_public_ed25519.bin"
)
