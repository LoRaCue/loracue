#include "lora_settings_screen.h"
#include "esp_log.h"
#include "lora_bands.h"
#include "lora_driver.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"

extern u8g2_t u8g2;

static const char *TAG     = "LORA_SETTINGS";
static int selected_preset = 0;
static int scroll_offset   = 0;

#define VIEWPORT_SIZE 2 // Show 2 presets at once

static const lora_config_t presets[] = {
    // Conference (100m)
    {
        .frequency        = 0,   // Set separately based on region
        .spreading_factor = 7,   // SF7
        .bandwidth        = 500, // 500kHz
        .coding_rate      = 5,   // 4/5
        .tx_power         = 0    // Set based on frequency
    },
    // Auditorium (250m)
    {
        .frequency        = 0,   // Set separately based on region
        .spreading_factor = 9,   // SF9
        .bandwidth        = 125, // 125kHz
        .coding_rate      = 7,   // 4/7
        .tx_power         = 0    // Set based on frequency
    },
    // Stadium (500m)
    {
        .frequency        = 0,   // Set separately based on region
        .spreading_factor = 10,  // SF10
        .bandwidth        = 125, // 125kHz
        .coding_rate      = 8,   // 4/8
        .tx_power         = 0    // Set based on frequency
    }};
static const char *preset_names[] = {"Conference (100m)", "Auditorium (250m)", "Stadium (500m)"};

static const char *preset_details[] = {"SF7, 500kHz, CR4/5", "SF9, 125kHz, CR4/7", "SF10, 125kHz, CR4/8"};

static const int preset_count = sizeof(preset_names) / sizeof(preset_names[0]);

// Get TX power from band profile
static int8_t get_tx_power_for_band(const char *band_id)
{
    extern const lora_band_profile_t *lora_bands_get_profile_by_id(const char *id);
    const lora_band_profile_t *band = lora_bands_get_profile_by_id(band_id);
    if (band) {
        return band->max_power_dbm;
    }
    return 14; // Default to 14dBm if band not found
}

static int get_current_preset(void)
{
    lora_config_t current_config;
    if (lora_get_config(&current_config) != ESP_OK) {
        return 0; // Default to first preset
    }

    // Match current config to presets
    for (int i = 0; i < preset_count; i++) {
        if (presets[i].spreading_factor == current_config.spreading_factor &&
            presets[i].bandwidth == current_config.bandwidth && presets[i].tx_power == current_config.tx_power) {
            return i;
        }
    }
    return 0; // Default to first preset if no match
}

void lora_settings_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "LORA SETTINGS");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);

    // Get current preset for highlighting
    int current_preset = get_current_preset();

    // Update scroll offset to keep selected item visible
    if (selected_preset < scroll_offset) {
        scroll_offset = selected_preset;
    } else if (selected_preset >= scroll_offset + VIEWPORT_SIZE) {
        scroll_offset = selected_preset - VIEWPORT_SIZE + 1;
    }

    // Draw visible presets (2x2 layout)
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    for (int i = 0; i < VIEWPORT_SIZE && (scroll_offset + i) < preset_count; i++) {
        int preset_idx = scroll_offset + i;
        int y_base     = 21 + (i * 20); // 20px spacing for 2-line items, moved 1px down

        if (preset_idx == selected_preset) {
            // Highlight selected preset with full box (exactly half viewport height)
            u8g2_DrawBox(&u8g2, 0, y_base - 10, DISPLAY_WIDTH, 20);
            u8g2_SetDrawColor(&u8g2, 0); // Invert color

            // Show checkmark for currently active preset (centered in lightbar)
            if (preset_idx == current_preset) {
                u8g2_DrawXBM(&u8g2, 4, y_base - 3, checkmark_width, checkmark_height, checkmark_bits);
                u8g2_DrawStr(&u8g2, 16, y_base - 1, preset_names[preset_idx]);
            } else {
                u8g2_DrawStr(&u8g2, 16, y_base - 1, preset_names[preset_idx]);
            }
            u8g2_DrawStr(&u8g2, 16, y_base + 8, preset_details[preset_idx]);
            u8g2_SetDrawColor(&u8g2, 1); // Reset color
        } else {
            // Show checkmark for currently active preset (centered in lightbar)
            if (preset_idx == current_preset) {
                u8g2_DrawXBM(&u8g2, 4, y_base - 3, checkmark_width, checkmark_height, checkmark_bits);
                u8g2_DrawStr(&u8g2, 16, y_base - 1, preset_names[preset_idx]);
            } else {
                u8g2_DrawStr(&u8g2, 16, y_base - 1, preset_names[preset_idx]);
            }
            u8g2_DrawStr(&u8g2, 16, y_base + 8, preset_details[preset_idx]);
        }
    }

    // Draw scroll indicators (positioned within viewport area)
    if (preset_count > VIEWPORT_SIZE) {
        // Up arrow if not at top (positioned in first item area)
        if (scroll_offset > 0) {
            // Check if scroll up arrow overlaps with lightbar
            int selected_y_base = 21 + ((selected_preset - scroll_offset) * 20);
            int lightbar_top    = selected_y_base - 10;
            int lightbar_bottom = selected_y_base + 10;

            if (15 >= lightbar_top && 15 + scroll_up_height <= lightbar_bottom) {
                u8g2_SetDrawColor(&u8g2, 0); // Invert for lightbar
            }
            u8g2_DrawXBM(&u8g2, 119, 15, scroll_up_width, scroll_up_height, scroll_up_bits);
            u8g2_SetDrawColor(&u8g2, 1); // Reset color
        }

        // Down arrow if not at bottom (positioned in second item area)
        if (scroll_offset + VIEWPORT_SIZE < preset_count) {
            // Check if scroll down arrow overlaps with lightbar
            int selected_y_base = 21 + ((selected_preset - scroll_offset) * 20);
            int lightbar_top    = selected_y_base - 10;
            int lightbar_bottom = selected_y_base + 10;

            if (35 >= lightbar_top && 35 + scroll_down_height <= lightbar_bottom) {
                u8g2_SetDrawColor(&u8g2, 0); // Invert for lightbar
            }
            u8g2_DrawXBM(&u8g2, 119, 35, scroll_down_width, scroll_down_height, scroll_down_bits);
            u8g2_SetDrawColor(&u8g2, 1); // Reset color
        }
    }

    // Footer with navigation (same as device mode screen)
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    // Left: Back arrow
    u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, 8, 64, "Back");

    // Middle: Next arrow
    u8g2_DrawXBM(&u8g2, 40, 56, track_next_width, track_next_height, track_next_bits);
    u8g2_DrawStr(&u8g2, 46, 64, "Next");

    // Right: Select with both buttons icon
    const char *select_text = "Select";
    int select_text_width   = u8g2_GetStrWidth(&u8g2, select_text);
    int select_x            = DISPLAY_WIDTH - both_buttons_width - select_text_width - 2;
    u8g2_DrawXBM(&u8g2, select_x, 56, both_buttons_width, both_buttons_height, both_buttons_bits);
    u8g2_DrawStr(&u8g2, select_x + both_buttons_width + 2, 64, select_text);

    u8g2_SendBuffer(&u8g2);
}

void lora_settings_screen_navigate(menu_direction_t direction)
{
    switch (direction) {
        case MENU_UP:
            selected_preset = (selected_preset - 1 + preset_count) % preset_count;
            break;
        case MENU_DOWN:
            selected_preset = (selected_preset + 1) % preset_count;
            break;
    }
}

void lora_settings_screen_select(void)
{
    // Get current config to preserve frequency
    lora_config_t current_config;
    if (lora_get_config(&current_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get current config");
        return;
    }

    // Apply preset but keep current frequency/band and set appropriate power
    lora_config_t new_config = presets[selected_preset];
    new_config.frequency = current_config.frequency;
    strncpy(new_config.band_id, current_config.band_id, sizeof(new_config.band_id) - 1);
    new_config.tx_power = get_tx_power_for_band(current_config.band_id);

    esp_err_t ret = lora_set_config(&new_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Applied LoRa preset: %s (freq: %lu Hz, power: %d dBm)", 
                 preset_names[selected_preset], new_config.frequency, new_config.tx_power);
    } else {
        ESP_LOGE(TAG, "Failed to apply LoRa preset: %s", esp_err_to_name(ret));
    }
}

void lora_settings_screen_reset(void)
{
    selected_preset = get_current_preset(); // Start with current preset selected
    scroll_offset   = 0;                    // Reset scroll position
}
