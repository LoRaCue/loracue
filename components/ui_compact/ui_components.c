#include "ui_components.h"
#include "ui_compact_fonts.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// External image declarations
LV_IMG_DECLARE(nav_up);
LV_IMG_DECLARE(nav_down);
LV_IMG_DECLARE(button_short_press);
LV_IMG_DECLARE(button_long_press);
LV_IMG_DECLARE(button_double_press);

// LVGL styles
lv_style_t style_title;
lv_style_t style_text;
lv_style_t style_small;

void ui_components_init(void) {
    // Title style (bold, larger)
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &lv_font_pixolletta_10);
    lv_style_set_text_color(&style_title, lv_color_white());

    // Normal text style
    lv_style_init(&style_text);
    lv_style_set_text_font(&style_text, &lv_font_pixolletta_10);
    lv_style_set_text_color(&style_text, lv_color_white());

    // Small text style
    lv_style_init(&style_small);
    lv_style_set_text_font(&style_small, &lv_font_pixolletta_10);
    lv_style_set_text_color(&style_small, lv_color_make(0xA0, 0xA0, 0xA0));
}

int ui_draw_icon_text(lv_obj_t *parent, const void *icon_src, const char *text, int x, int y, ui_align_t align) {
    const lv_img_dsc_t *img_dsc = icon_src;
    int icon_width = img_dsc->header.w;
    
    // Create label first to measure text width
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_add_style(label, &style_text, 0);
    lv_label_set_text(label, text);
    lv_obj_update_layout(label);
    int text_width = lv_obj_get_width(label);
    
    lv_obj_t *icon = lv_img_create(parent);
    lv_img_set_src(icon, icon_src);
    
    int total_width = icon_width + 2 + text_width;
    int icon_x, text_x;
    
    if (align == UI_ALIGN_LEFT) {
        icon_x = x;
        text_x = x + icon_width + 2;
    } else if (align == UI_ALIGN_RIGHT) {
        // x is rightmost pixel (inclusive), so subtract total_width - 1
        icon_x = x - total_width + 1;
        text_x = icon_x + icon_width + 2;
    } else { // UI_ALIGN_CENTER
        icon_x = x - total_width / 2;
        text_x = icon_x + icon_width + 2;
    }
    
    lv_obj_set_pos(icon, icon_x, y);
    lv_obj_set_pos(label, text_x, y - 1);
    
    return total_width;
}

// 3-item scrollable menu
ui_menu_t *ui_menu_create(lv_obj_t *parent, const char **item_names, int count) {
    ui_menu_t *menu = malloc(sizeof(ui_menu_t));
    if (!menu) return NULL;
    menu->screen = parent;
    menu->total_items = count;
    menu->selected_index = 0;
    menu->scroll_offset = 0;
    
    // Initialize pointers
    for (int i = 0; i < UI_MENU_VISIBLE_ITEMS; i++) {
        menu->items[i] = NULL;
    }
    menu->nav_up = NULL;
    menu->nav_down = NULL;
    menu->back_icon = NULL;

    // Don't auto-update - let caller control initial render
    return menu;
}

void ui_menu_update(ui_menu_t *menu, const char **item_names) {
    if (!menu || !menu->screen) return;
    
    ESP_LOGD("ui_menu", "Updating menu: selected=%d, scroll_offset=%d", menu->selected_index, menu->scroll_offset);
    
    // Delete old menu items if they exist
    for (int i = 0; i < UI_MENU_VISIBLE_ITEMS; i++) {
        if (menu->items[i]) {
            lv_obj_del(menu->items[i]);
            menu->items[i] = NULL;
        }
    }
    if (menu->nav_up) { lv_obj_del(menu->nav_up); menu->nav_up = NULL; }
    if (menu->nav_down) { lv_obj_del(menu->nav_down); menu->nav_down = NULL; }
    if (menu->back_icon) { lv_obj_del(menu->back_icon); menu->back_icon = NULL; }
    
    // Delete all children (no title to preserve)
    uint32_t child_count = lv_obj_get_child_cnt(menu->screen);
    for (int i = child_count - 1; i >= 0; i--) {
        lv_obj_t *child = lv_obj_get_child(menu->screen, i);
        if (child) lv_obj_del(child);
    }
    
    // Update scroll offset
    if (menu->selected_index < menu->scroll_offset) {
        menu->scroll_offset = menu->selected_index;
    } else if (menu->selected_index >= menu->scroll_offset + UI_MENU_VISIBLE_ITEMS) {
        menu->scroll_offset = menu->selected_index - UI_MENU_VISIBLE_ITEMS + 1;
    }

    // Render visible items with highlights
    for (int i = 0; i < UI_MENU_VISIBLE_ITEMS; i++) {
        int item_index = menu->scroll_offset + i;
        if (item_index < menu->total_items) {
            int y = 0 + (i * 13);  // Start at Y=0 for 4 items
            
            // Calculate lightbar width based on nav arrows
            bool has_nav_arrows = (menu->scroll_offset > 0) || 
                                 (menu->scroll_offset + UI_MENU_VISIBLE_ITEMS < menu->total_items);
            int lightbar_width = has_nav_arrows ? (DISPLAY_WIDTH - 9) : DISPLAY_WIDTH;
            
            // Create rounded highlight box for selected item
            if (item_index == menu->selected_index) {
                lv_obj_t *highlight = lv_obj_create(menu->screen);
                lv_obj_set_size(highlight, lightbar_width, 11);
                lv_obj_set_pos(highlight, 0, y + 1);
                lv_obj_set_style_bg_color(highlight, lv_color_white(), 0);
                lv_obj_set_style_border_width(highlight, 0, 0);
                lv_obj_set_style_radius(highlight, 3, 0);
                lv_obj_set_style_pad_all(highlight, 0, 0);
            }
            
            // Create label
            menu->items[i] = lv_label_create(menu->screen);
            lv_obj_add_style(menu->items[i], &style_text, 0);
            lv_label_set_text(menu->items[i], item_names[item_index]);
            lv_obj_set_pos(menu->items[i], UI_MARGIN_LEFT + 2, y + 1);
            
            // Invert text color for selected item
            if (item_index == menu->selected_index) {
                lv_obj_set_style_text_color(menu->items[i], lv_color_black(), 0);
            }
        }
    }

    // Navigation arrows
    if (menu->scroll_offset > 0) {
        menu->nav_up = lv_img_create(menu->screen);
        lv_img_set_src(menu->nav_up, &nav_up);
        lv_obj_set_pos(menu->nav_up, UI_NAV_ARROW_X, UI_MENU_ARROW_Y_UP);
    }
    
    if (menu->scroll_offset + UI_MENU_VISIBLE_ITEMS < menu->total_items) {
        menu->nav_down = lv_img_create(menu->screen);
        lv_img_set_src(menu->nav_down, &nav_down);
        lv_obj_set_pos(menu->nav_down, UI_NAV_ARROW_X, UI_MENU_ARROW_Y_DOWN);
    }

    // Bottom separator line
    lv_obj_t *bottom_line = lv_line_create(menu->screen);
    static lv_point_precise_t bottom_points[] = {{0, UI_BOTTOM_BAR_LINE_Y}, {DISPLAY_WIDTH, UI_BOTTOM_BAR_LINE_Y}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);
    
    // Bottom bar button hints
    ui_draw_icon_text(menu->screen, &button_short_press, "Next", 2, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
    ui_draw_icon_text(menu->screen, &button_long_press, "Select", 41, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
    ui_draw_icon_text(menu->screen, &button_double_press, "Back", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
}

void ui_menu_set_selected(ui_menu_t *menu, int index) {
    menu->selected_index = index;
}

void ui_menu_set_checkmark(ui_menu_t *menu, int index, bool checked) {
    int visible_index = index - menu->scroll_offset;
    if (visible_index >= 0 && visible_index < UI_MENU_VISIBLE_ITEMS) {
        // Add checkmark to label text
        const char *text = lv_label_get_text(menu->items[visible_index]);
        if (checked && text[0] != '[') {
            char buf[64];
            snprintf(buf, sizeof(buf), "[✓] %s", text);
            lv_label_set_text(menu->items[visible_index], buf);
        }
    }
}

// Edit mode screen
ui_edit_screen_t *ui_edit_screen_create(const char *title) {
    ui_edit_screen_t *screen = malloc(sizeof(ui_edit_screen_t));
    if (!screen) return NULL;
    screen->screen = NULL;  // Parent will be provided
    screen->edit_mode = false;

    return screen;
}

void ui_edit_screen_render(const ui_edit_screen_t *screen, lv_obj_t *parent, const char *title, const char *value, int current, int max) {
    // Title
    lv_obj_t *title_label = lv_label_create(parent);
    lv_obj_add_style(title_label, &style_title, 0);
    lv_label_set_text(title_label, title);
    lv_obj_set_pos(title_label, UI_MARGIN_LEFT, 0);

    // Top separator
    lv_obj_t *top_line = lv_line_create(parent);
    static lv_point_precise_t top_points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(top_line, top_points, 2);
    lv_obj_set_style_line_color(top_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(top_line, 1, 0);

    // Calculate content area
    int content_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    int value_height = 20;  // Font height
    
    // Center value in content area (always same position)
    lv_obj_t *value_label = lv_label_create(parent);
    lv_obj_set_style_text_font(value_label, &lv_font_pixolletta_20, 0);
    lv_obj_set_style_text_color(value_label, lv_color_white(), 0);
    lv_label_set_text(value_label, value);
    lv_obj_update_layout(value_label);
    int text_width = lv_obj_get_width(value_label);
    int center_x = (DISPLAY_WIDTH - text_width) / 2;
    int center_y = SEPARATOR_Y_TOP + (content_height - value_height) / 2;
    lv_obj_set_pos(value_label, center_x, center_y);

    // Progress bar (below value text)
    if (screen->edit_mode) {
        int bar_width = 100;
        int bar_height = 5;
        int bar_margin = 2;
        int bar_y = center_y + value_height + bar_margin;
        int fill_width = (current * bar_width) / max;
        
        lv_obj_t *bar_frame = lv_obj_create(parent);
        lv_obj_set_size(bar_frame, bar_width, bar_height);
        lv_obj_set_pos(bar_frame, (DISPLAY_WIDTH - bar_width) / 2, bar_y);
        lv_obj_set_style_bg_color(bar_frame, lv_color_black(), 0);
        lv_obj_set_style_border_color(bar_frame, lv_color_white(), 0);
        lv_obj_set_style_border_width(bar_frame, 1, 0);
        lv_obj_set_style_radius(bar_frame, 0, 0);
        
        if (fill_width > 2) {
            lv_obj_t *bar_fill = lv_obj_create(parent);
            lv_obj_set_size(bar_fill, fill_width - 2, bar_height - 2);
            lv_obj_set_pos(bar_fill, (DISPLAY_WIDTH - bar_width) / 2 + 1, bar_y + 1);
            lv_obj_set_style_bg_color(bar_fill, lv_color_white(), 0);
            lv_obj_set_style_border_width(bar_fill, 0, 0);
            lv_obj_set_style_radius(bar_fill, 0, 0);
        }
    }

    // Bottom separator
    lv_obj_t *bottom_line = lv_line_create(parent);
    static lv_point_precise_t bottom_points[] = {{0, SEPARATOR_Y_BOTTOM}, {DISPLAY_WIDTH, SEPARATOR_Y_BOTTOM}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);

    // Bottom bar
    if (screen->edit_mode) {
        ui_draw_icon_text(parent, &button_short_press, "+", 2, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
        ui_draw_icon_text(parent, &button_double_press, "-", 30, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
        ui_draw_icon_text(parent, &button_long_press, "Save", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
    } else {
        ui_draw_icon_text(parent, &button_long_press, "Edit", 2, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
    }
}

// Info screen
ui_info_screen_t *ui_info_screen_create(const char *title) {
    ui_info_screen_t *screen = malloc(sizeof(ui_info_screen_t));
    if (!screen) return NULL;
    screen->screen = NULL;
    return screen;
}

void ui_info_screen_render(ui_info_screen_t *screen, lv_obj_t *parent, const char *title, const char *lines[3]) {
    // Title
    lv_obj_t *title_label = lv_label_create(parent);
    lv_obj_add_style(title_label, &style_title, 0);
    lv_label_set_text(title_label, title);
    lv_obj_set_pos(title_label, UI_MARGIN_LEFT, 0);

    // Top separator
    lv_obj_t *line = lv_line_create(parent);
    static lv_point_precise_t points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(line, points, 2);
    lv_obj_set_style_line_color(line, lv_color_white(), 0);
    lv_obj_set_style_line_width(line, 1, 0);
    
    // Bottom separator
    lv_obj_t *bottom_line = lv_line_create(parent);
    static lv_point_precise_t bottom_points[] = {{0, SEPARATOR_Y_BOTTOM}, {DISPLAY_WIDTH, SEPARATOR_Y_BOTTOM}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);

    // Info lines
    for (int i = 0; i < 3; i++) {
        lv_obj_t *line_label = lv_label_create(parent);
        lv_obj_add_style(line_label, &style_text, 0);
        lv_label_set_text(line_label, lines[i]);
        lv_obj_set_pos(line_label, UI_MARGIN_LEFT, 15 + i * UI_LINE_HEIGHT);
    }

    // Back icon
    ui_draw_icon_text(parent, &button_double_press, "Back", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
}

// Text viewer implementation
#define TEXT_VIEWER_MAX_VISIBLE_LINES 4

ui_text_viewer_t *ui_text_viewer_create(const char **lines, uint8_t line_count) {
    ui_text_viewer_t *viewer = malloc(sizeof(ui_text_viewer_t));
    viewer->lines = lines;
    viewer->line_count = line_count;
    viewer->scroll_pos = 0;
    return viewer;
}

void ui_text_viewer_render(ui_text_viewer_t *viewer, lv_obj_t *parent, const char *title) {
    // Title at Y=0
    lv_obj_t *title_label = lv_label_create(parent);
    lv_obj_add_style(title_label, &style_title, 0);
    lv_label_set_text(title_label, title);
    lv_obj_set_pos(title_label, UI_MARGIN_LEFT, 0);

    // Top separator
    lv_obj_t *line = lv_line_create(parent);
    static lv_point_precise_t points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(line, points, 2);
    lv_obj_set_style_line_color(line, lv_color_white(), 0);
    lv_obj_set_style_line_width(line, 1, 0);
    
    // Bottom separator
    lv_obj_t *bottom_line = lv_line_create(parent);
    static lv_point_precise_t bottom_points[] = {{0, SEPARATOR_Y_BOTTOM}, {DISPLAY_WIDTH, SEPARATOR_Y_BOTTOM}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);

    // Display visible lines
    uint8_t visible_lines = (viewer->line_count - viewer->scroll_pos) > TEXT_VIEWER_MAX_VISIBLE_LINES 
                            ? TEXT_VIEWER_MAX_VISIBLE_LINES 
                            : (viewer->line_count - viewer->scroll_pos);
    
    for (uint8_t i = 0; i < visible_lines; i++) {
        lv_obj_t *line_label = lv_label_create(parent);
        lv_obj_add_style(line_label, &style_text, 0);
        lv_label_set_text(line_label, viewer->lines[viewer->scroll_pos + i]);
        lv_obj_set_pos(line_label, 0, 12 + i * UI_LINE_HEIGHT);
    }

    // Scroll icon (left)
    ui_draw_icon_text(parent, &button_double_press, "Scroll", 0, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
    
    // Back icon (right)
    ui_draw_icon_text(parent, &button_double_press, "Back", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
}

void ui_text_viewer_scroll(ui_text_viewer_t *viewer) {
    if (viewer->line_count <= TEXT_VIEWER_MAX_VISIBLE_LINES) {
        return; // No scrolling needed
    }
    
    viewer->scroll_pos++;
    if (viewer->scroll_pos > viewer->line_count - TEXT_VIEWER_MAX_VISIBLE_LINES) {
        viewer->scroll_pos = 0; // Jump back to start
    }
}

void ui_text_viewer_destroy(ui_text_viewer_t *viewer) {
    if (viewer) {
        free(viewer);
    }
}

void ui_info_screen_set_line(const ui_info_screen_t *screen, int line, const char *text) {
    // Not used in new implementation
}

// Numeric input screen
ui_numeric_input_t *ui_numeric_input_create(float initial, float min, float max, float step) {
    ui_numeric_input_t *input = malloc(sizeof(ui_numeric_input_t));
    if (!input) return NULL;
    input->screen = NULL;
    input->value = initial;
    input->min = min;
    input->max = max;
    input->step = step;
    input->edit_mode = false;
    return input;
}

void ui_numeric_input_render(const ui_numeric_input_t *input, lv_obj_t *parent, const char *title, const char *unit) {
    // Title above separator
    lv_obj_t *title_label = lv_label_create(parent);
    lv_obj_add_style(title_label, &style_title, 0);
    lv_label_set_text(title_label, title);
    lv_obj_set_pos(title_label, UI_MARGIN_LEFT, 0);
    
    // Top separator line (same Y as main screen)
    lv_obj_t *top_line = lv_line_create(parent);
    static lv_point_precise_t top_points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(top_line, top_points, 2);
    lv_obj_set_style_line_color(top_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(top_line, 1, 0);
    
    // Value with unit (large font, centered)
    int content_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    int value_height = 20;  // Font height
    
    lv_obj_t *value_label = lv_label_create(parent);
    lv_obj_set_style_text_font(value_label, &lv_font_pixolletta_20, 0);
    lv_obj_set_style_text_color(value_label, lv_color_white(), 0);
    char value_text[32];
    snprintf(value_text, sizeof(value_text), "%.1f %s", input->value, unit);
    lv_label_set_text(value_label, value_text);
    lv_obj_update_layout(value_label);
    int text_width = lv_obj_get_width(value_label);
    int center_x = (DISPLAY_WIDTH - text_width) / 2;
    int center_y = SEPARATOR_Y_TOP + (content_height - value_height) / 2;
    lv_obj_set_pos(value_label, center_x, center_y);
    
    // Bottom separator line
    lv_obj_t *bottom_line = lv_line_create(parent);
    static lv_point_precise_t bottom_points[] = {{0, SEPARATOR_Y_BOTTOM}, {DISPLAY_WIDTH, SEPARATOR_Y_BOTTOM}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);
    
    // Bottom bar hints
    if (input->edit_mode) {
        int plus_width = ui_draw_icon_text(parent, &button_short_press, "+", 2, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
        ui_draw_icon_text(parent, &button_double_press, "-", 2 + plus_width + 4, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
        ui_draw_icon_text(parent, &button_long_press, "Save", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
    } else {
        ui_draw_icon_text(parent, &button_long_press, "Edit", 2, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
        ui_draw_icon_text(parent, &button_double_press, "Back", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
    }
}

void ui_numeric_input_increment(ui_numeric_input_t *input) {
    input->value += input->step;
    if (input->value > input->max) input->value = input->max;
}

void ui_numeric_input_decrement(ui_numeric_input_t *input) {
    input->value -= input->step;
    if (input->value < input->min) input->value = input->min;
}

// Dropdown selection screen
ui_dropdown_t *ui_dropdown_create(int initial_index, int option_count) {
    ui_dropdown_t *dropdown = malloc(sizeof(ui_dropdown_t));
    if (!dropdown) return NULL;
    dropdown->screen = NULL;
    dropdown->selected_index = initial_index;
    dropdown->option_count = option_count;
    dropdown->edit_mode = false;
    return dropdown;
}

void ui_dropdown_render(ui_dropdown_t *dropdown, lv_obj_t *parent, const char *title, const char **options) {
    // Title above separator
    lv_obj_t *title_label = lv_label_create(parent);
    lv_obj_add_style(title_label, &style_title, 0);
    lv_label_set_text(title_label, title);
    lv_obj_set_pos(title_label, UI_MARGIN_LEFT, 0);
    
    // Top separator line (same Y as main screen)
    lv_obj_t *top_line = lv_line_create(parent);
    static lv_point_precise_t top_points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(top_line, top_points, 2);
    lv_obj_set_style_line_color(top_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(top_line, 1, 0);
    
    // Current value (large font, centered)
    int content_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    int value_height = 20;  // Font height
    
    lv_obj_t *value_label = lv_label_create(parent);
    lv_obj_set_style_text_font(value_label, &lv_font_pixolletta_20, 0);
    lv_obj_set_style_text_color(value_label, lv_color_white(), 0);
    lv_label_set_text(value_label, options[dropdown->selected_index]);
    lv_obj_update_layout(value_label);
    int text_width = lv_obj_get_width(value_label);
    int center_x = (DISPLAY_WIDTH - text_width) / 2;
    int center_y = SEPARATOR_Y_TOP + (content_height - value_height) / 2;
    lv_obj_set_pos(value_label, center_x, center_y);
    
    // Bottom separator line
    lv_obj_t *bottom_line = lv_line_create(parent);
    static lv_point_precise_t bottom_points[] = {{0, SEPARATOR_Y_BOTTOM}, {DISPLAY_WIDTH, SEPARATOR_Y_BOTTOM}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);
    
    // Bottom bar hints
    if (dropdown->edit_mode) {
        int next_width = ui_draw_icon_text(parent, &button_short_press, "Next", 2, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
        ui_draw_icon_text(parent, &button_double_press, "Prev", 2 + next_width + 4, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
        ui_draw_icon_text(parent, &button_long_press, "Save", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
    } else {
        ui_draw_icon_text(parent, &button_long_press, "Edit", 2, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
        ui_draw_icon_text(parent, &button_double_press, "Back", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
    }
}

void ui_dropdown_next(ui_dropdown_t *dropdown) {
    dropdown->selected_index = (dropdown->selected_index + 1) % dropdown->option_count;
}

void ui_dropdown_prev(ui_dropdown_t *dropdown) {
    dropdown->selected_index = (dropdown->selected_index - 1 + dropdown->option_count) % dropdown->option_count;
}

// Radio select screen
ui_radio_select_t *ui_radio_select_create(int item_count, ui_radio_mode_t mode) {
    ui_radio_select_t *radio = malloc(sizeof(ui_radio_select_t));
    if (!radio) return NULL;
    radio->screen = NULL;
    radio->selected_index = 0;
    radio->item_count = item_count;
    radio->scroll_offset = 0;
    radio->mode = mode;
    radio->nav_up = NULL;
    radio->nav_down = NULL;
    
    // Allocate selected_items for both single and multi-select
    // For single-select: selected_items[0] stores the saved/committed value index
    // For multi-select: each element stores the checked state
    if (mode == UI_RADIO_MULTI) {
        radio->selected_items = calloc(item_count, sizeof(bool));
    } else {
        // Single-select: use selected_items[0] to store saved value index
        radio->selected_items = calloc(1, sizeof(int));
        if (radio->selected_items) {
            ((int*)radio->selected_items)[0] = -1;  // No saved value initially
        }
    }
    
    return radio;
}

void ui_radio_select_render(ui_radio_select_t *radio, lv_obj_t *parent, const char *title, const char **items) {
    radio->screen = parent;
    
    // Title
    lv_obj_t *title_label = lv_label_create(parent);
    lv_obj_add_style(title_label, &style_title, 0);
    lv_label_set_text(title_label, title);
    lv_obj_set_pos(title_label, UI_MARGIN_LEFT, 0);
    
    // Top separator
    lv_obj_t *top_line = lv_line_create(parent);
    static lv_point_precise_t top_points[] = {{0, SEPARATOR_Y_TOP}, {DISPLAY_WIDTH, SEPARATOR_Y_TOP}};
    lv_line_set_points(top_line, top_points, 2);
    lv_obj_set_style_line_color(top_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(top_line, 1, 0);
    
    // Calculate visible items
    int visible_items = UI_MENU_VISIBLE_ITEMS;
    int start_y = SEPARATOR_Y_TOP + 2;
    
    // Adjust scroll offset
    if (radio->selected_index < radio->scroll_offset) {
        radio->scroll_offset = radio->selected_index;
    } else if (radio->selected_index >= radio->scroll_offset + visible_items) {
        radio->scroll_offset = radio->selected_index - visible_items + 1;
    }
    
    // Check if nav arrows are needed
    bool has_nav_arrows = (radio->scroll_offset > 0) || 
                         (radio->scroll_offset + visible_items < radio->item_count);
    int lightbar_width = has_nav_arrows ? (DISPLAY_WIDTH - 9) : DISPLAY_WIDTH;
    int lightbar_x = 14;  // Start after radio circles
    
    // Draw items
    for (int i = 0; i < visible_items && (radio->scroll_offset + i) < radio->item_count; i++) {
        int item_idx = radio->scroll_offset + i;
        int item_y = start_y + (i * UI_MENU_ITEM_HEIGHT);
        
        // Rounded highlight bar for selected item (starts after circles)
        if (item_idx == radio->selected_index) {
            lv_obj_t *highlight = lv_obj_create(parent);
            lv_obj_set_size(highlight, lightbar_width - lightbar_x, UI_MENU_ITEM_HEIGHT - 2);
            lv_obj_set_pos(highlight, lightbar_x, item_y + 1);
            lv_obj_set_style_bg_color(highlight, lv_color_white(), 0);
            lv_obj_set_style_border_width(highlight, 0, 0);
            lv_obj_set_style_radius(highlight, 3, 0);
        }
        
        // Radio circles - 9px diameter
        int circle_x = 2;
        int circle_y = item_y + (UI_MENU_ITEM_HEIGHT - 9) / 2;
        
        // For single-select radio: filled circle shows the SAVED value (from selected_items[0])
        // Navigation (lightbar) is separate and controlled by selected_index
        bool is_checked = false;
        if (radio->mode == UI_RADIO_SINGLE) {
            // In single-select, selected_items[0] stores the saved/committed value
            is_checked = (radio->selected_items && radio->selected_items[0] == item_idx);
        } else {
            // In multi-select, each item has its own checked state
            is_checked = radio->selected_items[item_idx];
        }
        
        // Outer circle (9px) - always shown
        lv_obj_t *outer_circle = lv_obj_create(parent);
        lv_obj_set_size(outer_circle, 9, 9);
        lv_obj_set_pos(outer_circle, circle_x, circle_y);
        lv_obj_set_style_radius(outer_circle, 5, 0);
        lv_obj_set_style_bg_color(outer_circle, lv_color_black(), 0);
        lv_obj_set_style_border_color(outer_circle, lv_color_white(), 0);
        lv_obj_set_style_border_width(outer_circle, 1, 0);
        
        // Inner filled circle (5px) - only if checked
        if (is_checked) {
            lv_obj_t *inner_circle = lv_obj_create(parent);
            lv_obj_set_size(inner_circle, 5, 5);
            lv_obj_set_pos(inner_circle, circle_x + 2, circle_y + 2);
            lv_obj_set_style_radius(inner_circle, 3, 0);
            lv_obj_set_style_bg_color(inner_circle, lv_color_white(), 0);
            lv_obj_set_style_border_width(inner_circle, 0, 0);
        }
        
        // Item text (4px down) - inverted on lightbar
        lv_obj_t *text = lv_label_create(parent);
        lv_obj_add_style(text, item_idx == radio->selected_index ? &style_title : &style_text, 0);
        if (item_idx == radio->selected_index) {
            lv_obj_set_style_text_color(text, lv_color_black(), 0);
        }
        lv_label_set_text(text, items[item_idx]);
        lv_obj_set_pos(text, lightbar_x + 2, item_y + 2);
    }
    
    // Nav arrows
    if (radio->scroll_offset > 0) {
        radio->nav_up = lv_img_create(parent);
        lv_img_set_src(radio->nav_up, &nav_up);
        lv_obj_set_pos(radio->nav_up, UI_NAV_ARROW_X, UI_MENU_ARROW_Y_UP);
    }
    
    if (radio->scroll_offset + visible_items < radio->item_count) {
        radio->nav_down = lv_img_create(parent);
        lv_img_set_src(radio->nav_down, &nav_down);
        lv_obj_set_pos(radio->nav_down, UI_NAV_ARROW_X, UI_MENU_ARROW_Y_DOWN);
    }
    
    // Bottom separator and hints
    lv_obj_t *bottom_line = lv_line_create(parent);
    static lv_point_precise_t bottom_points[] = {{0, UI_BOTTOM_BAR_LINE_Y}, {DISPLAY_WIDTH, UI_BOTTOM_BAR_LINE_Y}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);
    
    ui_draw_icon_text(parent, &button_short_press, "Next", 2, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
    ui_draw_icon_text(parent, &button_long_press, "Select", 42, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
    ui_draw_icon_text(parent, &button_double_press, "Back", DISPLAY_WIDTH, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_RIGHT);
}

void ui_radio_select_navigate_down(ui_radio_select_t *radio) {
    if (!radio) return;
    radio->selected_index = (radio->selected_index + 1) % radio->item_count;
}

void ui_radio_select_navigate_up(ui_radio_select_t *radio) {
    if (!radio) return;
    radio->selected_index = (radio->selected_index - 1 + radio->item_count) % radio->item_count;
}

void ui_radio_select_toggle(ui_radio_select_t *radio) {
    if (!radio) return;
    
    if (radio->mode == UI_RADIO_MULTI) {
        radio->selected_items[radio->selected_index] = !radio->selected_items[radio->selected_index];
    }
    // For SINGLE mode, selection is handled by navigate
}

bool ui_radio_select_is_selected(ui_radio_select_t *radio, int index) {
    if (!radio || index < 0 || index >= radio->item_count) return false;
    
    if (radio->mode == UI_RADIO_SINGLE) {
        return index == radio->selected_index;
    } else {
        return radio->selected_items[index];
    }
}

// Confirmation dialog screen
ui_confirmation_t *ui_confirmation_create(void) {
    ui_confirmation_t *confirm = malloc(sizeof(ui_confirmation_t));
    if (!confirm) return NULL;
    confirm->screen = NULL;
    confirm->hold_start_time = 0;
    confirm->holding = false;
    return confirm;
}

void ui_confirmation_render(ui_confirmation_t *confirm, lv_obj_t *parent, const char *title, const char *message) {
    // Title
    lv_obj_t *title_label = lv_label_create(parent);
    lv_obj_add_style(title_label, &style_title, 0);
    lv_label_set_text(title_label, title);
    lv_obj_set_pos(title_label, UI_MARGIN_LEFT, UI_MARGIN_TOP);
    
    // Message
    lv_obj_t *msg_label = lv_label_create(parent);
    lv_obj_add_style(msg_label, &style_text, 0);
    lv_label_set_text(msg_label, message);
    lv_obj_set_pos(msg_label, UI_MARGIN_LEFT, 20);
    
    // Bottom bar hint
    ui_draw_icon_text(parent, &button_long_press, "Hold 5s", 2, UI_BOTTOM_BAR_ICON_Y, UI_ALIGN_LEFT);
}

bool ui_confirmation_check_hold(ui_confirmation_t *confirm, bool button_pressed, uint32_t hold_duration_ms) {
    if (button_pressed && !confirm->holding) {
        confirm->holding = true;
        confirm->hold_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    } else if (!button_pressed) {
        confirm->holding = false;
        confirm->hold_start_time = 0;
    }
    
    if (confirm->holding) {
        uint32_t elapsed = (xTaskGetTickCount() * portTICK_PERIOD_MS) - confirm->hold_start_time;
        return elapsed >= hold_duration_ms;
    }
    return false;
}

