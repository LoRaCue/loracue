/**
 * @file usb_hid.c
 * @brief USB composite device (HID + CDC) with LoRa integration
 */

#include "usb_hid.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb.h"
#include "class/hid/hid_device.h"
#include "class/cdc/cdc_device.h"
#include "cJSON.h"
#include "device_registry.h"
#include "device_config.h"
#include "button_manager.h"

static const char *TAG = "USB_COMPOSITE";

// HID Report Descriptor for keyboard
static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

static char rx_buffer[256];
static size_t rx_len = 0;


static void send_key(uint8_t keycode, uint8_t modifier)
{
    if (!tud_hid_ready()) {
        ESP_LOGW(TAG, "HID not ready");
        return;
    }
    
    uint8_t keycodes[6] = {0};
    keycodes[0] = keycode;
    
    // Send key press
    tud_hid_keyboard_report(0, modifier, keycodes);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Send key release
    tud_hid_keyboard_report(0, 0, NULL);
    
    ESP_LOGI(TAG, "Sent key: 0x%02X (modifier: 0x%02X)", keycode, modifier);
}



// Key mappings for different modes
typedef struct {
    uint8_t keycode;
    uint8_t modifier;
} key_mapping_t;

// PC Mode key mappings
static const key_mapping_t pc_mode_keys[] = {
    [BUTTON_EVENT_PREV_SHORT] = {HID_KEY_PAGE_UP, 0},       // Previous slide
    [BUTTON_EVENT_PREV_LONG]  = {0x29, 0},                  // Escape key
    [BUTTON_EVENT_NEXT_SHORT] = {HID_KEY_PAGE_DOWN, 0},     // Next slide
    [BUTTON_EVENT_NEXT_LONG]  = {HID_KEY_F5, 0},            // Start slideshow
};

// Presenter Mode key mappings
static const key_mapping_t presenter_mode_keys[] = {
    [BUTTON_EVENT_PREV_SHORT] = {HID_KEY_PAGE_UP, 0},       // Previous slide
    [BUTTON_EVENT_PREV_LONG]  = {HID_KEY_B, 0},             // Blank screen
    [BUTTON_EVENT_NEXT_SHORT] = {HID_KEY_PAGE_DOWN, 0},     // Next slide
    [BUTTON_EVENT_NEXT_LONG]  = {0x2C, 0},                  // Space key
};

static void button_event_handler(button_event_type_t event, void* arg)
{
    device_config_t config;
    device_config_get(&config);
    const key_mapping_t* key_map = (config.device_mode == DEVICE_MODE_PC) ? pc_mode_keys : presenter_mode_keys;
    
    if (event >= sizeof(pc_mode_keys) / sizeof(key_mapping_t)) {
        ESP_LOGW(TAG, "Invalid button event: %d", event);
        return;
    }
    
    key_mapping_t mapping = key_map[event];
    send_key(mapping.keycode, mapping.modifier);
}

// JSON command handlers
static void handle_ping_command(void)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "PONG");
    char *json_string = cJSON_Print(response);
    tud_cdc_write_str(json_string);
    tud_cdc_write_flush();
    free(json_string);
    cJSON_Delete(response);
}

static void handle_pair_device_command(cJSON *json)
{
    cJSON *name_item = cJSON_GetObjectItem(json, "name");
    cJSON *mac_item = cJSON_GetObjectItem(json, "mac");
    cJSON *key_item = cJSON_GetObjectItem(json, "key");
    
    if (!cJSON_IsString(name_item) || !cJSON_IsString(mac_item) || !cJSON_IsString(key_item)) {
        tud_cdc_write_str("{\"type\":\"ERROR\",\"message\":\"Invalid parameters\"}\n");
        tud_cdc_write_flush();
        return;
    }
    
    uint8_t mac_address[6];
    uint8_t aes_key[16];
    
    // Convert hex MAC string to bytes
    sscanf(mac_item->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &mac_address[0], &mac_address[1], &mac_address[2],
           &mac_address[3], &mac_address[4], &mac_address[5]);
    
    // Convert hex key string to bytes
    for (int i = 0; i < 16 && i < strlen(key_item->valuestring) / 2; i++) {
        sscanf(key_item->valuestring + i * 2, "%02hhx", &aes_key[i]);
    }
    
    // Generate device ID from MAC address
    uint16_t device_id = (mac_address[4] << 8) | mac_address[5];
    
    esp_err_t result = device_registry_add(device_id, name_item->valuestring, mac_address, aes_key);
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "PAIR_RESULT");
    cJSON_AddBoolToObject(response, "success", result == ESP_OK);
    char *json_string = cJSON_Print(response);
    tud_cdc_write_str(json_string);
    tud_cdc_write_flush();
    free(json_string);
    cJSON_Delete(response);
}

static void process_json_command(const char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    if (!json) return;
    
    cJSON *type = cJSON_GetObjectItem(json, "type");
    if (!cJSON_IsString(type)) {
        cJSON_Delete(json);
        return;
    }
    
    if (strcmp(type->valuestring, "PING") == 0) {
        handle_ping_command();
    } else if (strcmp(type->valuestring, "PAIR_DEVICE") == 0) {
        handle_pair_device_command(json);
    }
    
    cJSON_Delete(json);
}

// TinyUSB CDC callbacks
void tud_cdc_rx_cb(uint8_t itf)
{
    while (tud_cdc_available()) {
        char c = tud_cdc_read_char();
        if (c == '\n' || rx_len >= sizeof(rx_buffer) - 1) {
            rx_buffer[rx_len] = '\0';
            process_json_command(rx_buffer);
            rx_len = 0;
        } else {
            rx_buffer[rx_len++] = c;
        }
    }
}

// TinyUSB HID callbacks
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
}

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance)
{
    return hid_report_descriptor;
}

bool usb_hid_is_connected(void)
{
    return tud_hid_ready() && tud_cdc_connected();
}

esp_err_t usb_hid_send_key(usb_hid_keycode_t keycode)
{
    send_key(keycode, 0);
    return ESP_OK;
}

esp_err_t usb_hid_init(void)
{
    ESP_LOGI(TAG, "Initializing USB composite device (HID + CDC)");
    
    tinyusb_config_t tusb_cfg = {
        .port = TINYUSB_PORT_FULL_SPEED_0,
        .phy = {.skip_setup = false, .self_powered = false},
        .task = {.size = 4096, .priority = 5, .xCoreID = 0},
        .descriptor = {0},
        .event_cb = NULL,
        .event_arg = NULL
    };
    
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    
    // Register button event handler
    ESP_ERROR_CHECK(button_manager_register_callback(button_event_handler, NULL));
    
    ESP_LOGI(TAG, "USB composite device initialized");
    return ESP_OK;
}
