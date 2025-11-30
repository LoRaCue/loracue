/**
 * @file usb_descriptors.h
 * @brief USB descriptor data for LoRaCue
 */

#pragma once

#include "class/cdc/cdc_device.h"
#include "class/hid/hid_device.h"
#include "tusb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get USB device descriptor
 * @return Pointer to device descriptor
 */
const tusb_desc_device_t *usb_get_device_descriptor(void);

/**
 * @brief Get USB configuration descriptor
 * @return Pointer to configuration descriptor
 */
const uint8_t *usb_get_config_descriptor(void);

/**
 * @brief Get USB string descriptor
 * @param index String descriptor index (0=language, 1=manufacturer, 2=product, 3=serial)
 * @return Pointer to string descriptor (UTF-16LE format)
 */
const char **usb_get_string_descriptors(void);

/**
 * @brief HID keyboard report descriptor
 */
extern const uint8_t hid_keyboard_report_desc[];

#ifdef __cplusplus
}
#endif
