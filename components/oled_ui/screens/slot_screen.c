#include "slot_screen.h"
#include "general_config.h"
#include "oled_ui.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "ui_helpers.h"

extern u8g2_t u8g2;

static int selected_slot = 0;
static bool edit_mode = false;

#define SLOT_COUNT 16

void slot_screen_init(void) {
    general_config_t config;
    general_config_get(&config);
    selected_slot = config.slot_id - 1; // Convert 1-16 to 0-15
    edit_mode = false;
}

void slot_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    general_config_t config;
    general_config_get(&config);
    
    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "SLOT SELECTION");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);
    
    // Display current slot value
    u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
    char slot_str[20];
    snprintf(slot_str, sizeof(slot_str), "Slot %d", selected_slot + 1);
    int text_width = u8g2_GetStrWidth(&u8g2, slot_str);
    int text_x = (DISPLAY_WIDTH - text_width) / 2;
    int text_y = (SEPARATOR_Y_TOP + SEPARATOR_Y_BOTTOM) / 2 + 5;
    u8g2_DrawStr(&u8g2, text_x, text_y, slot_str);

    // Footer with one-button UI icons
    if (edit_mode) {
        ui_draw_footer(FOOTER_CONTEXT_EDIT, NULL);
    } else {
        const char *labels[] = {NULL, "Back", "Edit"};
        ui_draw_footer(FOOTER_CONTEXT_CUSTOM, labels);
    }
    
    u8g2_SendBuffer(&u8g2);
}

void slot_screen_navigate(menu_direction_t direction) {
    if (!edit_mode) return;
    
    if (direction == MENU_DOWN) {
        selected_slot = (selected_slot + 1) % SLOT_COUNT;
    } else if (direction == MENU_UP) {
        selected_slot = (selected_slot - 1 + SLOT_COUNT) % SLOT_COUNT;
    }
}

void slot_screen_select(void) {
    if (edit_mode) {
        // Save
        general_config_t config;
        general_config_get(&config);
        config.slot_id = selected_slot + 1; // Convert 0-15 to 1-16
        general_config_set(&config);
        oled_ui_set_screen(OLED_SCREEN_MENU);
    } else {
        // Enter edit mode
        edit_mode = true;
    }
}

bool slot_screen_is_edit_mode(void) {
    return edit_mode;
}
