/**
 * @file screen_lora_regulatory_domain.c
 * @brief Regulatory domain selection screen
 */

#include "screen_lora_regulatory_domain.h"
#include "ui_compact.h"
#include "config_manager.h"
#include "lora_bands.h"
#include "system_events.h"
#include <string.h>

static int current_selection = 0;
static int total_options = 0;
static char current_domain[3] = "";

void screen_lora_regulatory_domain_on_enter(void)
{
    // Get current regulatory domain
    config_manager_get_regulatory_domain(current_domain, sizeof(current_domain));
    
    // Count total options (regions + "Unknown" if domain is empty)
    total_options = lora_regulatory_get_region_count();
    if (strlen(current_domain) == 0) {
        total_options++; // Add "Unknown" option
    }
    
    // Find current selection
    current_selection = 0;
    bool domain_is_empty = (strlen(current_domain) == 0);
    if (domain_is_empty) {
        current_selection = 0; // "Unknown" is first option
    } else {
        int offset = domain_is_empty ? 1 : 0;
        for (int i = 0; i < lora_regulatory_get_region_count(); i++) {
            const lora_region_t *region = lora_regulatory_get_region(i);
            if (region && strcmp(region->id, current_domain) == 0) {
                current_selection = i + offset;
                break;
            }
        }
    }
}

void screen_lora_regulatory_domain_on_exit(void)
{
    // Nothing to cleanup
}

void screen_lora_regulatory_domain_on_button_press(button_event_t event)
{
    switch (event) {
        case BUTTON_EVENT_NEXT:
            current_selection = (current_selection + 1) % total_options;
            break;
            
        case BUTTON_EVENT_PREV:
            current_selection = (current_selection - 1 + total_options) % total_options;
            break;
            
        case BUTTON_EVENT_SELECT:
            // Apply selection
            {
                bool domain_is_empty = (strlen(current_domain) == 0);
                if (domain_is_empty && current_selection == 0) {
                    // Keep "Unknown" (empty string)
                    config_manager_set_regulatory_domain("");
                } else {
                    int offset = domain_is_empty ? 1 : 0;
                    int region_index = current_selection - offset;
                    const lora_region_t *region = lora_regulatory_get_region(region_index);
                    if (region) {
                    if (region) {
                        config_manager_set_regulatory_domain(region->id);
                    }
                }
            }
            
            // Post configuration change event
            esp_event_post(SYSTEM_EVENTS, DEVICE_CONFIG_CHANGED, NULL, 0, portMAX_DELAY);
            
            // Return to parent menu
            ui_compact_navigate_back();
            break;
            
        case BUTTON_EVENT_BACK:
            ui_compact_navigate_back();
            break;
            
        default:
            break;
    }
}

void screen_lora_regulatory_domain_render(void)
{
    ui_compact_clear();
    ui_compact_draw_title("Regulatory Domain");
    
    // Show current selection
    bool domain_is_empty = (strlen(current_domain) == 0);
    if (domain_is_empty && current_selection == 0) {
        ui_compact_draw_menu_item(0, "Unknown", true);
    } else {
        int offset = domain_is_empty ? 1 : 0;
        
        // Show "Unknown" option if domain is empty
        if (domain_is_empty) {
            ui_compact_draw_menu_item(0, "Unknown", current_selection == 0);
        }
        
        // Show region options
        for (int i = 0; i < lora_regulatory_get_region_count(); i++) {
            const lora_region_t *region = lora_regulatory_get_region(i);
            if (region) {
                bool selected = (current_selection == i + offset);
                ui_compact_draw_menu_item(i + offset, region->name, selected);
            }
        }
    }
    
    ui_compact_draw_navigation_hint("Select/Back");
    ui_compact_display();
}
