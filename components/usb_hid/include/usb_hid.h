/**
 * @file usb_hid.h
 * @brief USB HID keyboard interface for LoRaCue
 *
 * CONTEXT: Ticket 3.1 - TinyUSB HID Keyboard
 * PURPOSE: USB composite device (HID keyboard + CDC serial)
 * MAPPING: LoRa commands to USB HID keycodes
 */

#pragma once

#include "class/hid/hid.h" // Include TinyUSB HID constants
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB HID key codes for presentation control
 * Using TinyUSB HID key constants to avoid conflicts
 */
typedef enum {
    USB_HID_KEY_PAGE_DOWN = HID_KEY_PAGE_DOWN, ///< Next slide (Page Down)
    USB_HID_KEY_PAGE_UP   = HID_KEY_PAGE_UP,   ///< Previous slide (Page Up)
    USB_HID_KEY_B         = HID_KEY_B,         ///< Black screen ('b')
    USB_HID_KEY_F5        = HID_KEY_F5,        ///< Start presentation (F5)
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
