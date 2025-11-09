#include "main_screen.h"
#include "esp_log.h"
#include "general_config.h"
#include "pc_mode_screen.h"
#include "presenter_main_screen.h"
#include "ui_mini.h"

static const char *TAG = "main_screen";

// Main screen adapts to current device mode
void main_screen_draw(const ui_status_t *status)
{
    if (ui_state.current_mode == DEVICE_MODE_PC) {
        // Draw PC mode layout
        pc_mode_screen_draw();
    } else {
        // Draw presenter mode layout
        presenter_main_screen_draw(status);
    }
}
