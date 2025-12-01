/**
 * @file usb_hid.c
 * @brief USB composite device (HID + CDC) with LoRa integration
 */

#include "usb_hid.h"
#include "input_manager.h"
#include "cJSON.h"
#include "class/cdc/cdc_device.h"
#include "class/hid/hid_device.h"
#include "device_registry.h"
#include "esp_log.h"
#include "general_config.h"
#include "tinyusb.h"
#include "tusb.h"
#include "usb_cdc.h"
#include "usb_descriptors.h"

static const char *TAG = "USB_COMPOSITE";

static void send_key(uint8_t keycode, uint8_t modifier)
{
    if (!tud_hid_ready()) {
        ESP_LOGW(TAG, "HID not ready");
        return;
    }

    uint8_t keycodes[6] = {0};
    keycodes[0]         = keycode;

    // Send key press
    tud_hid_keyboard_report(0, modifier, keycodes);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Send key release
    tud_hid_keyboard_report(0, 0, NULL);

    ESP_LOGI(TAG, "Sent key: 0x%02X (modifier: 0x%02X)", keycode, modifier);
}

// Key mappings for different modes
typedef struct {
    uint8_t keycode;
    uint8_t modifier;
} key_mapping_t;

// PC Mode key mappings (one-button UI)
static const key_mapping_t pc_mode_keys[] = {
    [BUTTON_EVENT_SHORT]  = {HID_KEY_ARROW_RIGHT, 0}, // Cursor right
    [BUTTON_EVENT_DOUBLE] = {HID_KEY_ARROW_LEFT, 0},  // Cursor left
    [BUTTON_EVENT_LONG]   = {HID_KEY_F5, 0},          // (unused - menu handled by UI)
};

// Presenter Mode key mappings (one-button UI)
static const key_mapping_t presenter_mode_keys[] = {
    [BUTTON_EVENT_SHORT]  = {HID_KEY_PAGE_DOWN, 0}, // Next slide
    [BUTTON_EVENT_DOUBLE] = {HID_KEY_PAGE_UP, 0},   // Previous slide
    [BUTTON_EVENT_LONG]   = {HID_KEY_B, 0},         // Blank screen
};

static void button_event_handler(button_event_type_t event, void *arg)
{
    general_config_t config;
    general_config_get(&config);

    // In PC mode, long press opens menu (handled by UI), not HID
    if (config.device_mode == DEVICE_MODE_PC && event == BUTTON_EVENT_LONG) {
        return;
    }

    const key_mapping_t *key_map = (config.device_mode == DEVICE_MODE_PC) ? pc_mode_keys : presenter_mode_keys;

    if (event >= sizeof(pc_mode_keys) / sizeof(key_mapping_t)) {
        ESP_LOGW(TAG, "Invalid button event: %d", event);
        return;
    }

    key_mapping_t mapping = key_map[event];
    send_key(mapping.keycode, mapping.modifier);
}

// TinyUSB HID Callbacks
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_keyboard_report_desc;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

bool usb_hid_is_connected(void)
{
    return tud_hid_ready();
}

esp_err_t usb_hid_send_key(usb_hid_keycode_t keycode)
{
    send_key(keycode, 0);
    return ESP_OK;
}

esp_err_t usb_hid_init(void)
{
    ESP_LOGI(TAG, "Initializing USB composite device (HID + CDC)");

    // esp_tinyusb 1.7.6+ API
    tinyusb_config_t tusb_cfg = {.device_descriptor        = usb_get_device_descriptor(),
                                 .string_descriptor        = usb_get_string_descriptors(),
                                 .string_descriptor_count  = 6,
                                 .external_phy             = false,
                                 .configuration_descriptor = usb_get_config_descriptor()};

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // Initialize USB CDC protocol
    ESP_ERROR_CHECK(usb_cdc_init());

    // Register button event handler
    ESP_ERROR_CHECK(input_manager_register_button_callback(button_event_handler, NULL));

    ESP_LOGI(TAG, "USB composite device initialized");
    return ESP_OK;
}
