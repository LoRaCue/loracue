/**
 * @file usb_descriptors.c
 * @brief Custom USB descriptors for LoRaCue
 */

#include "usb_descriptors.h"
#include "bsp.h"
#include "esp_mac.h"
#include "version.h"
#include <stdio.h>
#include <string.h>

// USB VID/VID
#define USB_VID 0x1209  // pid.codes
// PID comes from BSP

// Device Descriptor
static tusb_desc_device_t device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,  // USB 2.0
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = 0xFAB0,  // Will be updated from BSP
    .bcdDevice = 0x0100,  // Will be updated from version.h
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

// String descriptors
static char serial_number[32];
static bool descriptors_initialized = false;

static void init_descriptors(void)
{
    if (descriptors_initialized) {
        return;
    }

    // Get BSP USB config
    const bsp_usb_config_t *usb_config = bsp_get_usb_config();
    device_descriptor.idProduct = usb_config->usb_pid;

    // Set version from GitVersion
    device_descriptor.bcdDevice = (LORACUE_VERSION_MAJOR << 8) | 
                                  (LORACUE_VERSION_MINOR << 4) | 
                                  LORACUE_VERSION_PATCH;

    // Generate serial number from ESP32 MAC
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(serial_number, sizeof(serial_number), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    descriptors_initialized = true;
}

const tusb_desc_device_t* usb_get_device_descriptor(void)
{
    init_descriptors();
    return &device_descriptor;
}

// HID Report Descriptor (Keyboard) - must be declared before config descriptor
const uint8_t hid_keyboard_report_desc[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// Configuration Descriptor (CDC + HID)
enum {
    ITF_NUM_CDC_CTRL = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

static const uint8_t config_descriptor[] = {
    // Config: bus-powered, 500mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 250),

    // CDC: Interface 0 & 1
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_CTRL, 4, 0x81, 8, 0x02, 0x82, 64),

    // HID: Interface 2 (Keyboard)
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, HID_ITF_PROTOCOL_KEYBOARD, sizeof(hid_keyboard_report_desc), 0x83, 16, 10)
};

const uint8_t* usb_get_config_descriptor(void)
{
    return config_descriptor;
}

// String Descriptors
const uint16_t* usb_get_string_descriptor(uint8_t index)
{
    init_descriptors();

    static uint16_t desc_str[32];
    uint8_t chr_count;

    switch (index) {
        case 0:
            // Language ID: English (US)
            desc_str[1] = 0x0409;
            chr_count = 1;
            break;

        case 1:
            // Manufacturer
            chr_count = 0;
            for (const char *s = "LoRaCue"; *s; s++) {
                desc_str[1 + chr_count++] = *s;
            }
            break;

        case 2:
            // Product (from BSP)
            {
                const bsp_usb_config_t *usb_config = bsp_get_usb_config();
                chr_count = 0;
                for (const char *s = usb_config->usb_product; *s; s++) {
                    desc_str[1 + chr_count++] = *s;
                }
            }
            break;

        case 3:
            // Serial Number
            chr_count = 0;
            for (const char *s = serial_number; *s; s++) {
                desc_str[1 + chr_count++] = *s;
            }
            break;

        default:
            return NULL;
    }

    // First byte is length (in bytes), second byte is descriptor type
    desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return desc_str;
}

// TinyUSB callbacks
uint8_t const* tud_descriptor_device_cb(void)
{
    return (uint8_t const*)usb_get_device_descriptor();
}

uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return usb_get_config_descriptor();
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    return usb_get_string_descriptor(index);
}

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_keyboard_report_desc;
}
