/**
 * @file lora_bands.c
 * @brief LoRa regulatory compliance and hardware profiles
 */

#include "lora_bands.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "LORA_REGULATORY";

// Embedded JSON file
extern const uint8_t lora_regulatory_json_start[] asm("_binary_lora_regulatory_json_start");
extern const uint8_t lora_regulatory_json_end[] asm("_binary_lora_regulatory_json_end");

static lora_hardware_t hardware_profiles[LORA_MAX_HARDWARE];
static lora_region_t regions[LORA_MAX_REGIONS];
static lora_compliance_t compliance_rules[LORA_MAX_COMPLIANCE];
static int hardware_count = 0;
static int region_count = 0;
static int compliance_count = 0;
static bool regulatory_initialized = false;

esp_err_t lora_regulatory_init(void)
{
    if (regulatory_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Parsing embedded lora_regulatory.json");

    cJSON *root = cJSON_Parse((const char *)lora_regulatory_json_start);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    // Parse hardware profiles
    cJSON *hardware_array = cJSON_GetObjectItem(root, "hardware");
    if (cJSON_IsArray(hardware_array)) {
        hardware_count = 0;
        cJSON *hw = NULL;
        cJSON_ArrayForEach(hw, hardware_array) {
            if (hardware_count >= LORA_MAX_HARDWARE) break;
            
            cJSON *id = cJSON_GetObjectItem(hw, "id");
            cJSON *name = cJSON_GetObjectItem(hw, "name");
            cJSON *freq_min = cJSON_GetObjectItem(hw, "freq_min_khz");
            cJSON *freq_max = cJSON_GetObjectItem(hw, "freq_max_khz");
            cJSON *optimal = cJSON_GetObjectItem(hw, "optimal_khz");
            
            if (cJSON_IsString(id) && cJSON_IsString(name) && 
                cJSON_IsNumber(freq_min) && cJSON_IsNumber(freq_max) && cJSON_IsNumber(optimal)) {
                
                strncpy(hardware_profiles[hardware_count].id, id->valuestring, sizeof(hardware_profiles[hardware_count].id) - 1);
                strncpy(hardware_profiles[hardware_count].name, name->valuestring, sizeof(hardware_profiles[hardware_count].name) - 1);
                hardware_profiles[hardware_count].freq_min_khz = freq_min->valueint;
                hardware_profiles[hardware_count].freq_max_khz = freq_max->valueint;
                hardware_profiles[hardware_count].optimal_khz = optimal->valueint;
                hardware_count++;
            }
        }
    }

    // Parse regions
    cJSON *regions_array = cJSON_GetObjectItem(root, "regions");
    if (cJSON_IsArray(regions_array)) {
        region_count = 0;
        cJSON *region = NULL;
        cJSON_ArrayForEach(region, regions_array) {
            if (region_count >= LORA_MAX_REGIONS) break;
            
            cJSON *id = cJSON_GetObjectItem(region, "id");
            cJSON *name = cJSON_GetObjectItem(region, "name");
            
            if (cJSON_IsString(id) && cJSON_IsString(name)) {
                strncpy(regions[region_count].id, id->valuestring, sizeof(regions[region_count].id) - 1);
                strncpy(regions[region_count].name, name->valuestring, sizeof(regions[region_count].name) - 1);
                region_count++;
            }
        }
    }

    // Parse compliance rules
    cJSON *compliance_array = cJSON_GetObjectItem(root, "compliance");
    if (cJSON_IsArray(compliance_array)) {
        compliance_count = 0;
        cJSON *rule = NULL;
        cJSON_ArrayForEach(rule, compliance_array) {
            if (compliance_count >= LORA_MAX_COMPLIANCE) break;
            
            cJSON *region = cJSON_GetObjectItem(rule, "region");
            cJSON *hardware = cJSON_GetObjectItem(rule, "hardware");
            cJSON *freq_min = cJSON_GetObjectItem(rule, "freq_min_khz");
            cJSON *freq_max = cJSON_GetObjectItem(rule, "freq_max_khz");
            cJSON *max_power = cJSON_GetObjectItem(rule, "max_power_dbm");
            
            if (cJSON_IsString(region) && cJSON_IsString(hardware) && 
                cJSON_IsNumber(freq_min) && cJSON_IsNumber(freq_max) && cJSON_IsNumber(max_power)) {
                
                strncpy(compliance_rules[compliance_count].region_id, region->valuestring, sizeof(compliance_rules[compliance_count].region_id) - 1);
                strncpy(compliance_rules[compliance_count].hardware_id, hardware->valuestring, sizeof(compliance_rules[compliance_count].hardware_id) - 1);
                compliance_rules[compliance_count].freq_min_khz = freq_min->valueint;
                compliance_rules[compliance_count].freq_max_khz = freq_max->valueint;
                compliance_rules[compliance_count].max_power_dbm = max_power->valueint;
                
                // Optional fields
                cJSON *duty_cycle = cJSON_GetObjectItem(rule, "duty_cycle_percent");
                compliance_rules[compliance_count].duty_cycle_percent = cJSON_IsNumber(duty_cycle) ? duty_cycle->valueint : 0;
                
                cJSON *fhss = cJSON_GetObjectItem(rule, "fhss_required");
                compliance_rules[compliance_count].fhss_required = cJSON_IsTrue(fhss);
                
                cJSON *lbt = cJSON_GetObjectItem(rule, "lbt_required");
                compliance_rules[compliance_count].lbt_required = cJSON_IsTrue(lbt);
                
                compliance_count++;
            }
        }
    }

    cJSON_Delete(root);
    regulatory_initialized = true;

    ESP_LOGI(TAG, "Loaded %d hardware profiles, %d regions, %d compliance rules", 
             hardware_count, region_count, compliance_count);
    return ESP_OK;
}

int lora_hardware_get_count(void)
{
    return hardware_count;
}

const lora_hardware_t *lora_hardware_get_profile(int index)
{
    if (index < 0 || index >= hardware_count) {
        return NULL;
    }
    return &hardware_profiles[index];
}

const lora_hardware_t *lora_hardware_get_profile_by_id(const char *id)
{
    for (int i = 0; i < hardware_count; i++) {
        if (strcmp(hardware_profiles[i].id, id) == 0) {
            return &hardware_profiles[i];
        }
    }
    return NULL;
}

int lora_hardware_get_index_by_frequency(uint32_t frequency_hz)
{
    uint32_t freq_khz = frequency_hz / 1000;

    for (int i = 0; i < hardware_count; i++) {
        if (freq_khz >= hardware_profiles[i].freq_min_khz && freq_khz <= hardware_profiles[i].freq_max_khz) {
            return i;
        }
    }

    return -1;
}

int lora_regulatory_get_region_count(void)
{
    return region_count;
}

const lora_region_t *lora_regulatory_get_region(int index)
{
    if (index < 0 || index >= region_count) {
        return NULL;
    }
    return &regions[index];
}

const lora_region_t *lora_regulatory_get_region_by_id(const char *region_id)
{
    for (int i = 0; i < region_count; i++) {
        if (strcmp(regions[i].id, region_id) == 0) {
            return &regions[i];
        }
    }
    return NULL;
}

const lora_compliance_t *lora_regulatory_get_compliance(const char *region_id, const char *hardware_id)
{
    for (int i = 0; i < compliance_count; i++) {
        if (strcmp(compliance_rules[i].region_id, region_id) == 0 && 
            strcmp(compliance_rules[i].hardware_id, hardware_id) == 0) {
            return &compliance_rules[i];
        }
    }
    return NULL;
}

int lora_regulatory_get_available_hardware(const char *region_id, const char **hardware_ids, int max_count)
{
    int count = 0;
    for (int i = 0; i < compliance_count && count < max_count; i++) {
        if (strcmp(compliance_rules[i].region_id, region_id) == 0) {
            // Check if already added
            bool already_added = false;
            for (int j = 0; j < count; j++) {
                if (strcmp(hardware_ids[j], compliance_rules[i].hardware_id) == 0) {
                    already_added = true;
                    break;
                }
            }
            if (!already_added) {
                hardware_ids[count++] = compliance_rules[i].hardware_id;
            }
        }
    }
    return count;
}

bool lora_regulatory_validate_domain(const char *domain)
{
    if (!domain || strlen(domain) == 0) return true; // Empty = "Unknown" is valid
    if (strlen(domain) != 2) return false; // Must be 2 chars
    return lora_regulatory_get_region_by_id(domain) != NULL;
}

const lora_compliance_t *lora_regulatory_get_limits(const char *domain, const char *hardware_id)
{
    if (!domain || strlen(domain) == 0) return NULL; // No limits for "Unknown"
    return lora_regulatory_get_compliance(domain, hardware_id);
}

int lora_regulatory_get_region_count(void)
{
    return region_count;
}

const lora_region_t *lora_regulatory_get_region(int index)
{
    if (index < 0 || index >= region_count) return NULL;
    return &regions[index];
}
