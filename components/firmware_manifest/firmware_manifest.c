#include "firmware_manifest.h"
#include "bsp.h"
#include "version.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>

static const char *TAG = "FW_MANIFEST";

// Firmware manifest embedded in .rodata section
static firmware_manifest_t __attribute__((section(".rodata.manifest"))) 
    firmware_manifest = {
    .magic = FIRMWARE_MAGIC,
    .manifest_version = 1,
    .reserved = {0, 0, 0},
    .board_id = "",  // Filled at runtime
    .firmware_version = LORACUE_VERSION_FULL,
    .build_timestamp = 0,  // Could be filled at build time
    .checksum = 0  // Could be calculated post-build
};

void firmware_manifest_init(void)
{
    // Get board ID from BSP
    const char *board_id = bsp_get_board_id();
    strncpy((char*)firmware_manifest.board_id, board_id, sizeof(firmware_manifest.board_id) - 1);
    firmware_manifest.board_id[sizeof(firmware_manifest.board_id) - 1] = '\0';
    
    ESP_LOGI(TAG, "Firmware manifest initialized:");
    ESP_LOGI(TAG, "  Board ID: %s", firmware_manifest.board_id);
    ESP_LOGI(TAG, "  Version:  %s", firmware_manifest.firmware_version);
    ESP_LOGI(TAG, "  Magic:    0x%08" PRIX32, firmware_manifest.magic);
}

const firmware_manifest_t* firmware_manifest_get(void)
{
    return &firmware_manifest;
}
