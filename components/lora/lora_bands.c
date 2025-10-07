/**
 * @file lora_bands.c
 * @brief LoRa hardware band profiles from embedded JSON
 */

#include "lora_bands.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "LORA_BANDS";

// Embedded JSON file
extern const uint8_t lora_bands_json_start[] asm("_binary_lora_bands_json_start");
extern const uint8_t lora_bands_json_end[]   asm("_binary_lora_bands_json_end");

static lora_band_profile_t band_profiles[LORA_MAX_BANDS];
static int band_count = 0;
static bool bands_initialized = false;

esp_err_t lora_bands_init(void)
{
    if (bands_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Parsing embedded lora_bands.json");

    cJSON *root = cJSON_Parse((const char *)lora_bands_json_start);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    cJSON *profiles = cJSON_GetObjectItem(root, "HardwareProfiles");
    if (!cJSON_IsArray(profiles)) {
        ESP_LOGE(TAG, "HardwareProfiles not found");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    band_count = 0;
    cJSON *profile = NULL;
    cJSON_ArrayForEach(profile, profiles) {
        if (band_count >= LORA_MAX_BANDS) break;

        cJSON *id = cJSON_GetObjectItem(profile, "id");
        cJSON *name = cJSON_GetObjectItem(profile, "name");
        cJSON *optimal_center = cJSON_GetObjectItem(profile, "optimal_center_khz");
        cJSON *optimal_min = cJSON_GetObjectItem(profile, "optimal_freq_min_khz");
        cJSON *optimal_max = cJSON_GetObjectItem(profile, "optimal_freq_max_khz");

        if (cJSON_IsString(id) && cJSON_IsString(name) && 
            cJSON_IsNumber(optimal_center) && cJSON_IsNumber(optimal_min) && cJSON_IsNumber(optimal_max)) {
            
            strncpy(band_profiles[band_count].id, id->valuestring, sizeof(band_profiles[band_count].id) - 1);
            strncpy(band_profiles[band_count].name, name->valuestring, sizeof(band_profiles[band_count].name) - 1);
            band_profiles[band_count].optimal_center_khz = optimal_center->valueint;
            band_profiles[band_count].optimal_freq_min_khz = optimal_min->valueint;
            band_profiles[band_count].optimal_freq_max_khz = optimal_max->valueint;

            ESP_LOGI(TAG, "Loaded band: %s (%s) - %d-%d kHz", 
                     band_profiles[band_count].id,
                     band_profiles[band_count].name,
                     band_profiles[band_count].optimal_freq_min_khz,
                     band_profiles[band_count].optimal_freq_max_khz);

            band_count++;
        }
    }

    cJSON_Delete(root);
    bands_initialized = true;

    ESP_LOGI(TAG, "Loaded %d band profiles", band_count);
    return ESP_OK;
}

int lora_bands_get_count(void)
{
    return band_count;
}

const lora_band_profile_t* lora_bands_get_profile(int index)
{
    if (index < 0 || index >= band_count) {
        return NULL;
    }
    return &band_profiles[index];
}

const lora_band_profile_t* lora_bands_get_profile_by_id(const char *id)
{
    for (int i = 0; i < band_count; i++) {
        if (strcmp(band_profiles[i].id, id) == 0) {
            return &band_profiles[i];
        }
    }
    return NULL;
}

int lora_bands_get_index_by_frequency(uint32_t frequency_hz)
{
    uint32_t freq_khz = frequency_hz / 1000;
    
    for (int i = 0; i < band_count; i++) {
        if (freq_khz >= band_profiles[i].optimal_freq_min_khz && 
            freq_khz <= band_profiles[i].optimal_freq_max_khz) {
            return i;
        }
    }
    
    return -1; // Not in any band
}
