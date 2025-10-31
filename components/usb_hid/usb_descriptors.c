/**
 * @file usb_descriptors.c
 * @brief USB descriptor data for LoRaCue (device, config, strings, HID report)
 */

#include "usb_descriptors.h"
#include "bsp.h"
#include "esp_mac.h"
#include "version.h"
#include <stdio.h>
#include <string.h>

// USB VID from pid.codes
#define USB_VID 0x1209

// Device Descriptor
static tusb_desc_device_t device_descriptor = {.bLength            = sizeof(tusb_desc_device_t),
                                               .bDescriptorType    = TUSB_DESC_DEVICE,
                                               .bcdUSB             = 0x0200,
                                               .bDeviceClass       = TUSB_CLASS_MISC,
                                               .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
                                               .bDeviceProtocol    = MISC_PROTOCOL_IAD,
                                               .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
                                               .idVendor           = USB_VID,
                                               .idProduct          = 0xFAB0, // Updated from BSP
                                               .bcdDevice          = 0x0100, // Updated from version
                                               .iManufacturer      = 0x01,
                                               .iProduct           = 0x02,
                                               .iSerialNumber      = 0x03,
                                               .bNumConfigurations = 0x01};

// HID Report Descriptor (Keyboard)
const uint8_t hid_keyboard_report_desc[] = {TUD_HID_REPORT_DESC_KEYBOARD()};

// Configuration Descriptor (CDC + HID)
enum { ITF_NUM_CDC_CTRL = 0, ITF_NUM_CDC_DATA, ITF_NUM_HID, ITF_NUM_TOTAL };
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

static const uint8_t config_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 250),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_CTRL, 4, 0x81, 8, 0x02, 0x82, 64),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, HID_ITF_PROTOCOL_KEYBOARD, sizeof(hid_keyboard_report_desc), 0x83, 16, 10)};

// String descriptor state
static char serial_number[32];
static const char *string_descriptors[6];
static bool descriptors_initialized = false;

static void init_descriptors(void)
{
    if (descriptors_initialized) {
        return;
    }

    const bsp_usb_config_t *usb_config = bsp_get_usb_config();
    device_descriptor.idProduct        = usb_config->usb_pid;
    device_descriptor.bcdDevice = (LORACUE_VERSION_MAJOR << 8) | (LORACUE_VERSION_MINOR << 4) | LORACUE_VERSION_PATCH;

    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(serial_number, sizeof(serial_number), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
             mac[5]);

    // Setup string descriptor array
    string_descriptors[0] = ""; // Language (handled by TinyUSB)
    string_descriptors[1] = "LoRaCue";
    string_descriptors[2] = usb_config->usb_product;
    string_descriptors[3] = serial_number;
    string_descriptors[4] = "LoRaCue Commands";
    string_descriptors[5] = "LoRaCue HID";

    descriptors_initialized = true;
}

const char **usb_get_string_descriptors(void)
{
    init_descriptors();
    return string_descriptors;
}

const tusb_desc_device_t *usb_get_device_descriptor(void)
{
    init_descriptors();
    return &device_descriptor;
}

const uint8_t *usb_get_config_descriptor(void)
{
    return config_descriptor;
}
