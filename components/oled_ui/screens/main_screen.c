#include "main_screen.h"
#include "presenter_main_screen.h"
#include "pc_main_screen.h"
#include "device_config.h"

void main_screen_draw(const ui_status_t* status) {
    device_config_t config;
    device_config_get(&config);
    
    if (config.device_mode == DEVICE_MODE_PRESENTER) {
        presenter_main_screen_draw(status);
    } else {
        pc_main_screen_draw(status);
    }
}
