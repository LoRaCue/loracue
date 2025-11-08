#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdint.h>
#include <stdbool.h>

#define CDC_ITF_DATA 0
#define CDC_ITF_DEBUG 1

void usb_cdc_init(void);
void usb_cdc_task(void);
bool usb_cdc_connected(void);
int usb_cdc_read(uint8_t *buffer, int len);
int usb_cdc_write(const uint8_t *buffer, int len);
void usb_cdc_check_bootloader_cmd(void);

#endif
