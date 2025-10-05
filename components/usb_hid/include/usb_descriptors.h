/**
 * @file usb_descriptors.h
 * @brief Custom USB descriptors for LoRaCue
 */

#pragma once

#include "tusb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get custom USB device descriptor
 * 
 * @return Pointer to device descriptor
 */
const tusb_desc_device_t* usb_get_device_descriptor(void);

/**
 * @brief Get custom USB configuration descriptor
 * 
 * @return Pointer to configuration descriptor
 */
const uint8_t* usb_get_config_descriptor(void);

/**
 * @brief Get USB string descriptor
 * 
 * @param index String descriptor index
 * @return Pointer to string descriptor (UTF-16LE format)
 */
const uint16_t* usb_get_string_descriptor(uint8_t index);

#ifdef __cplusplus
}
#endif
