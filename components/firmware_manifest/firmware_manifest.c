#include "firmware_manifest.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "FW_MANIFEST";

const char* firmware_manifest_get_board_id(void)
{
    const esp_app_desc_t *desc = esp_app_get_description();
    return desc->project_name;
}

const char* firmware_manifest_get_version(void)
{
    const esp_app_desc_t *desc = esp_app_get_description();
    return desc->version;
}

bool firmware_manifest_is_compatible(const esp_app_desc_t *new_app_info)
{
    const esp_app_desc_t *running = esp_app_get_description();
    
    // Check board compatibility via project_name
    if (strcmp(running->project_name, new_app_info->project_name) != 0) {
        ESP_LOGW(TAG, "Board mismatch: running=%s, new=%s", 
                 running->project_name, new_app_info->project_name);
        return false;
    }
    
    return true;
}

void firmware_manifest_init(void)
{
    const esp_app_desc_t *desc = esp_app_get_description();
    
    ESP_LOGI(TAG, "Firmware manifest:");
    ESP_LOGI(TAG, "  Board ID: %s", desc->project_name);
    ESP_LOGI(TAG, "  Version: %s", desc->version);
    ESP_LOGI(TAG, "  Build date: %s %s", desc->date, desc->time);
    ESP_LOGI(TAG, "  IDF version: %s", desc->idf_ver);
}
