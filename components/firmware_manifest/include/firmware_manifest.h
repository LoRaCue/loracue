#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FIRMWARE_MAGIC 0x4C524355  // "LRCU"

/**
 * @brief Firmware manifest structure
 * 
 * Embedded in firmware binary for compatibility checking during OTA updates.
 * Total size: 60 bytes
 */
typedef struct {
    uint32_t magic;                 ///< Magic number (0x4C524355 = "LRCU")
    uint8_t  manifest_version;      ///< Manifest format version (1)
    uint8_t  reserved[3];           ///< Reserved for alignment
    char     board_id[16];          ///< Board identifier (e.g., "heltec_v3")
    char     firmware_version[32];  ///< Version string from GitVersion
    uint32_t build_timestamp;       ///< Unix timestamp of build
    uint32_t checksum;              ///< CRC32 of manifest (excluding this field)
} __attribute__((packed)) firmware_manifest_t;

/**
 * @brief Get pointer to firmware manifest
 * 
 * @return Pointer to firmware manifest structure
 */
const firmware_manifest_t* firmware_manifest_get(void);

/**
 * @brief Initialize firmware manifest
 * 
 * Must be called during system initialization to populate runtime fields.
 */
void firmware_manifest_init(void);

#ifdef __cplusplus
}
#endif
