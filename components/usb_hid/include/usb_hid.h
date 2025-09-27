/**
 * @file usb_hid.h
 * @brief USB HID keyboard interface for LoRaCue
 * 
 * CONTEXT: Ticket 3.1 - TinyUSB HID Keyboard
 * PURPOSE: USB composite device (HID keyboard + CDC serial)
 * MAPPING: LoRa commands to USB HID keycodes
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB HID key codes for presentation control
 */
typedef enum {
    HID_KEY_PAGE_DOWN = 0x4E,   ///< Next slide (Page Down)
    HID_KEY_PAGE_UP = 0x4B,     ///< Previous slide (Page Up)  
    HID_KEY_B = 0x05,           ///< Black screen ('b')
    HID_KEY_F5 = 0x3E,          ///< Start presentation (F5)
} usb_hid_keycode_t;

/**
 * @brief Initialize USB HID interface
 * 
 * Sets up TinyUSB with HID keyboard and CDC serial interfaces.
 * 
 * @return ESP_OK on success
 */
esp_err_t usb_hid_init(void);

/**
 * @brief Send keyboard key press
 * 
 * @param keycode HID keycode to send
 * @return ESP_OK on success
 */
esp_err_t usb_hid_send_key(usb_hid_keycode_t keycode);

/**
 * @brief Send key press with modifier
 * 
 * @param keycode HID keycode to send
 * @param modifier Modifier keys (Ctrl, Alt, etc.)
 * @return ESP_OK on success
 */
esp_err_t usb_hid_send_key_with_modifier(usb_hid_keycode_t keycode, uint8_t modifier);

/**
 * @brief Check if USB is connected
 * 
 * @return true if USB host is connected
 */
bool usb_hid_is_connected(void);

#ifdef __cplusplus
}
#endif
