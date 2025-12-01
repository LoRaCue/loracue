#include "lvgl.h"
#include "screens.h"
#include "ui_compact_fonts.h"
#include "ui_navigator.h"
#include "version.h"

// External declaration for generated boot logo
LV_IMG_DECLARE(boot_logo);

void screen_boot_create(lv_obj_t *parent)
{
    // Set black background
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    // Boot logo at top
    lv_obj_t *logo = lv_img_create(parent);
    lv_img_set_src(logo, &boot_logo);
    lv_obj_set_pos(logo, (128 - boot_logo.header.w) / 2, 0); // Centered horizontally, Y=0

    // Version at bottom center with micro font
    lv_obj_t *version = lv_label_create(parent);
    lv_label_set_text(version, LORACUE_VERSION_FULL);
    lv_obj_set_style_text_color(version, lv_color_white(), 0);
    lv_obj_set_style_text_font(version, &lv_font_micro_10, 0);
    lv_obj_set_style_text_align(version, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(version, 128);
    lv_obj_set_pos(version, 0, 64 - 10); // Full width, bottom of screen
}
static void handle_input_event(input_event_t event)
{
    // Boot screen might not handle input, or could skip boot on click
    (void)event;
}

static void screen_boot_reset(void)
{
    // Nothing to reset
}

static ui_screen_t boot_screen = {
    .type               = UI_SCREEN_BOOT,
    .create             = screen_boot_create,
    .destroy            = screen_boot_reset,
    .handle_input_event = handle_input_event,
};

ui_screen_t *screen_boot_get_interface(void)
{
    return &boot_screen;
}
