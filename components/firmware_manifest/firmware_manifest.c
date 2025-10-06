#include "firmware_manifest.h"
#include "bsp.h"
#include "version.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>

static const char *TAG = "FW_MANIFEST";

// Determine board ID at compile time based on BSP selection
#if defined(SIMULATOR_BUILD)
    #define BOARD_ID "wokwi_sim"
#else
    #define BOARD_ID "heltec_v3"
#endif

// Build timestamp as string for logging
static const char BUILD_TIME[] = __DATE__ " " __TIME__;

// Firmware manifest embedded in .rodata section (~180KB into binary)
// Placed naturally by linker within first 256KB - suitable for OTA validation
const firmware_manifest_t __attribute__((section(".rodata.manifest"), used, aligned(4))) 
    firmware_manifest = {
    .magic = FIRMWARE_MAGIC,
    .manifest_version = 1,
    .reserved = {0, 0, 0},
    .board_id = BOARD_ID,
    .firmware_version = LORACUE_VERSION_FULL,
    .build_timestamp = 0,  // Set to 0, actual timestamp in BUILD_TIME string
    .checksum = 0
};

void firmware_manifest_init(void)
{
    ESP_LOGI(TAG, "Firmware manifest:");
    ESP_LOGI(TAG, "  Board ID: %s", firmware_manifest.board_id);
    ESP_LOGI(TAG, "  Version:  %s", firmware_manifest.firmware_version);
    ESP_LOGI(TAG, "  Built:    %s", BUILD_TIME);
    ESP_LOGI(TAG, "  Magic:    0x%08" PRIX32, firmware_manifest.magic);
}

const firmware_manifest_t* firmware_manifest_get(void)
{
    return &firmware_manifest;
}
