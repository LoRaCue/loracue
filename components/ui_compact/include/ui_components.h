#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "lvgl.h"
#include "ui_lvgl_config.h"
#include <stdbool.h>

// Layout constants
#define UI_MARGIN_TOP 2
#define UI_MARGIN_BOTTOM 2
#define UI_MARGIN_LEFT 2
#define UI_MARGIN_RIGHT 2
#define UI_LINE_HEIGHT 10
#define UI_ICON_SIZE 8
#define UI_PROGRESS_BAR_HEIGHT 6

// Bottom bar constants (aligned with main screen)
#define UI_BOTTOM_BAR_LINE_Y SEPARATOR_Y_BOTTOM
#define UI_BOTTOM_BAR_ICON_Y (SEPARATOR_Y_BOTTOM + 3)
#define UI_BOTTOM_BAR_TEXT_Y (SEPARATOR_Y_BOTTOM + 2)

// Menu constants
#define UI_MENU_ITEM_HEIGHT 13
#define UI_MENU_BOTTOM_BAR_HEIGHT 11
#define UI_MENU_VISIBLE_ITEMS ((DISPLAY_HEIGHT - UI_MENU_BOTTOM_BAR_HEIGHT) / UI_MENU_ITEM_HEIGHT)
#define UI_MENU_ARROW_Y_UP 3
#define UI_MENU_ARROW_Y_DOWN (SEPARATOR_Y_BOTTOM - 7)

// Icon positions
#define UI_NAV_ARROW_X 120
#define UI_BACK_ICON_X_OFFSET -4

// LVGL styles
extern lv_style_t style_title;
extern lv_style_t style_text;
extern lv_style_t style_small;

// Initialize styles (call once at startup)
void ui_components_init(void);

// Icon + text helper
typedef enum {
    UI_ALIGN_LEFT,
    UI_ALIGN_CENTER,
    UI_ALIGN_RIGHT
} ui_align_t;

int ui_draw_icon_text(lv_obj_t *parent, const void *icon_src, const char *text, int x, int y, ui_align_t align);

// 3-item scrollable menu
typedef struct {
    lv_obj_t *screen;
    lv_obj_t *items[UI_MENU_VISIBLE_ITEMS];
    lv_obj_t *nav_up;
    lv_obj_t *nav_down;
    lv_obj_t *back_icon;
    int total_items;
    int selected_index;
    int scroll_offset;
} ui_menu_t;

ui_menu_t *ui_menu_create(lv_obj_t *parent, const char **item_names, int count);
void ui_menu_update(ui_menu_t *menu, const char **item_names);
void ui_menu_set_selected(ui_menu_t *menu, int index);
void ui_menu_set_checkmark(ui_menu_t *menu, int index, bool checked);

// Edit mode screen
typedef struct {
    lv_obj_t *screen;
    bool edit_mode;
} ui_edit_screen_t;

ui_edit_screen_t *ui_edit_screen_create(const char *title);
void ui_edit_screen_render(const ui_edit_screen_t *screen, lv_obj_t *parent, const char *title, const char *value, int current, int max);

// Numeric input screen
typedef struct {
    lv_obj_t *screen;
    float value;
    float min;
    float max;
    float step;
    bool edit_mode;
} ui_numeric_input_t;

ui_numeric_input_t *ui_numeric_input_create(float initial, float min, float max, float step);
void ui_numeric_input_render(const ui_numeric_input_t *input, lv_obj_t *parent, const char *title, const char *unit);
void ui_numeric_input_increment(ui_numeric_input_t *input);
void ui_numeric_input_decrement(ui_numeric_input_t *input);

// Dropdown selection screen
typedef struct {
    lv_obj_t *screen;
    int selected_index;
    int option_count;
    bool edit_mode;
} ui_dropdown_t;

ui_dropdown_t *ui_dropdown_create(int initial_index, int option_count);
void ui_dropdown_render(ui_dropdown_t *dropdown, lv_obj_t *parent, const char *title, const char **options);
void ui_dropdown_next(ui_dropdown_t *dropdown);
void ui_dropdown_prev(ui_dropdown_t *dropdown);

// Radio select screen (single or multi selection)
typedef enum {
    UI_RADIO_SINGLE,
    UI_RADIO_MULTI
} ui_radio_mode_t;

typedef struct {
    lv_obj_t *screen;
    int selected_index;
    bool *selected_items;  // For multi-select
    int item_count;
    int scroll_offset;
    ui_radio_mode_t mode;
    lv_obj_t *nav_up;
    lv_obj_t *nav_down;
} ui_radio_select_t;

ui_radio_select_t *ui_radio_select_create(int item_count, ui_radio_mode_t mode);
void ui_radio_select_render(ui_radio_select_t *radio, lv_obj_t *parent, const char *title, const char **items);
void ui_radio_select_navigate_down(ui_radio_select_t *radio);
void ui_radio_select_navigate_up(ui_radio_select_t *radio);
void ui_radio_select_toggle(ui_radio_select_t *radio);
bool ui_radio_select_is_selected(ui_radio_select_t *radio, int index);

// Confirmation dialog screen
typedef struct {
    lv_obj_t *screen;
    uint32_t hold_start_time;
    bool holding;
} ui_confirmation_t;

ui_confirmation_t *ui_confirmation_create(void);
void ui_confirmation_render(ui_confirmation_t *confirm, lv_obj_t *parent, const char *title, const char *message);
bool ui_confirmation_check_hold(ui_confirmation_t *confirm, bool button_pressed, uint32_t hold_duration_ms);

// Info screen (3 lines of text)
typedef struct {
    lv_obj_t *screen;
} ui_info_screen_t;

ui_info_screen_t *ui_info_screen_create(const char *title);
void ui_info_screen_render(ui_info_screen_t *screen, lv_obj_t *parent, const char *title, const char *lines[3]);
void ui_info_screen_set_line(const ui_info_screen_t *screen, int line, const char *text);

// Text viewer component
typedef struct {
    const char **lines;
    uint8_t line_count;
    uint8_t scroll_pos;
} ui_text_viewer_t;

ui_text_viewer_t *ui_text_viewer_create(const char **lines, uint8_t line_count);
void ui_text_viewer_render(ui_text_viewer_t *viewer, lv_obj_t *parent, const char *title);
void ui_text_viewer_scroll(ui_text_viewer_t *viewer);
void ui_text_viewer_destroy(ui_text_viewer_t *viewer);

#endif // UI_COMPONENTS_H
