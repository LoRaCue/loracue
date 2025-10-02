#pragma once

#include "esp_err.h"

esp_err_t usb_cdc_init(void);
void usb_cdc_process_commands(void);
