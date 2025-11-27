#include "lvgl.h"
#include "ui_compact_fonts.h"
#include "version.h"

// External declaration for generated boot logo
LV_IMG_DECLARE(boot_logo);

void screen_boot_create(lv_obj_t *parent) {
    // Set black background
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    // Boot logo at top
    lv_obj_t *logo = lv_img_create(parent);
    lv_img_set_src(logo, &boot_logo);
    lv_obj_set_pos(logo, (128 - boot_logo.header.w) / 2, 0);  // Centered horizontally, Y=0
    
    // Version at bottom left with micro font
    lv_obj_t *version = lv_label_create(parent);
    lv_label_set_text(version, LORACUE_VERSION_FULL);
    lv_obj_set_style_text_color(version, lv_color_white(), 0);
    lv_obj_set_style_text_font(version, &lv_font_micro_5, 0);
    lv_obj_set_pos(version, 0, 64 - 5);  // X=0 (left), bottom of screen
}
