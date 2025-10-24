#include "ui_rich.h"
#include "esp_log.h"

static const char *TAG = "ui_rich";
static ui_rich_screen_t current_screen = UI_RICH_SCREEN_HOME;

extern void home_screen_create(void);
extern void settings_screen_create(void);
extern void presenter_screen_create(void);
extern void pc_mode_screen_create(void);

esp_err_t ui_rich_init(void)
{
    ESP_LOGI(TAG, "Initializing rich UI system");
    
    // Create home screen by default
    home_screen_create();
    current_screen = UI_RICH_SCREEN_HOME;
    
    return ESP_OK;
}

void ui_rich_navigate(ui_rich_screen_t screen)
{
    ESP_LOGI(TAG, "Navigating to screen %d", screen);
    
    // Clean current screen
    lv_obj_clean(lv_scr_act());
    
    // Create new screen
    switch (screen) {
        case UI_RICH_SCREEN_HOME:
            home_screen_create();
            break;
        case UI_RICH_SCREEN_SETTINGS:
            settings_screen_create();
            break;
        case UI_RICH_SCREEN_PRESENTER:
            presenter_screen_create();
            break;
        case UI_RICH_SCREEN_PC_MODE:
            pc_mode_screen_create();
            break;
    }
    
    current_screen = screen;
}

ui_rich_screen_t ui_rich_get_current_screen(void)
{
    return current_screen;
}
