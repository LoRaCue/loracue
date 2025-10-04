#include "icons/ui_status_icons.h"
#include "u8g2.h"
#include "ui_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern u8g2_t u8g2;

// USB icon (14x7)
static const unsigned char usb_icon[] = {
    0xfe, 0xdf, 0x01, 0xe0, 0x01, 0xe0, 0xfd, 0xef, 0x01, 0xe0, 0x01, 0xe0, 0xfe, 0xdf};

// Bluetooth icon (5x8)
static const unsigned char bluetooth_icon[] = {0xe4, 0xec, 0xf5, 0xee, 0xee, 0xf5, 0xec, 0xe4};

// Battery icons (16x8)
static const unsigned char battery_4_bars[] = {
    0xfe, 0x3f, 0x01, 0x40, 0x6d, 0xdb, 0x6d, 0xdb, 0x6d, 0xdb, 0x6d, 0xdb, 0x01, 0x40, 0xfe, 0x3f};

static const unsigned char battery_3_bars[] = {
    0xfe, 0x3f, 0x01, 0x40, 0x6d, 0xc3, 0x6d, 0xc3, 0x6d, 0xc3, 0x6d, 0xc3, 0x01, 0x40, 0xfe, 0x3f};

static const unsigned char battery_2_bars[] = {
    0xfe, 0x3f, 0x01, 0x40, 0x6d, 0xc0, 0x6d, 0xc0, 0x6d, 0xc0, 0x6d, 0xc0, 0x01, 0x40, 0xfe, 0x3f};

static const unsigned char battery_1_bar[] = {
    0xfe, 0x3f, 0x01, 0x40, 0x0d, 0xc0, 0x0d, 0xc0, 0x0d, 0xc0, 0x0d, 0xc0, 0x01, 0x40, 0xfe, 0x3f};

static const unsigned char battery_0_bars[] = {
    0xfe, 0x3f, 0x01, 0x40, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0x40, 0xfe, 0x3f};

// RF signal icons (11x8)
static const unsigned char rf_4_bars[] = {
    0x00, 0xfe, 0x00, 0xfe, 0xc0, 0xfe, 0xc0, 0xfe, 0xd8, 0xfe, 0xd8, 0xfe, 0xdb, 0xfe, 0xdb, 0xfe};

static const unsigned char rf_3_bars[] = {
    0x00, 0xf8, 0x00, 0xf8, 0xc0, 0xf8, 0xc0, 0xf8, 0xd8, 0xf8, 0xd8, 0xf8, 0xdb, 0xf8, 0xdb, 0xfe};

static const unsigned char rf_2_bars[] = {
    0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0x18, 0xf8, 0x18, 0xf8, 0x1b, 0xf8, 0xdb, 0xfe};

static const unsigned char rf_1_bar[] = {
    0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0x03, 0xf8, 0xdb, 0xfe};

static const unsigned char rf_0_bars[] = {
    0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0xf8, 0xdb, 0xfe};

void ui_usb_draw_at(int x, int y)
{
    u8g2_DrawXBM(&u8g2, x, y, 14, 7, usb_icon);
}

void ui_bluetooth_draw_at(int x, int y, bool connected)
{
    if (connected) {
        u8g2_DrawXBM(&u8g2, x, y, 5, 8, bluetooth_icon);
    } else {
        u8g2_DrawXBM(&u8g2, x, y, 5, 8, bluetooth_icon);
    }
}

void ui_battery_draw(uint8_t battery_level)
{
    const unsigned char *bitmap;

    if (battery_level > 75) {
        bitmap = battery_4_bars;
    } else if (battery_level > 50) {
        bitmap = battery_3_bars;
    } else if (battery_level > 25) {
        bitmap = battery_2_bars;
    } else if (battery_level > 5) {
        bitmap = battery_1_bar;
    } else {
        bitmap = battery_0_bars;
        // Blink for critical battery (≤5%)
        static uint32_t last_toggle = 0;
        static bool visible = true;
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        if (now - last_toggle > 500) {  // Toggle every 500ms
            visible = !visible;
            last_toggle = now;
        }
        
        if (!visible) {
            return;  // Skip drawing to create blink effect
        }
    }

    u8g2_DrawXBM(&u8g2, BATTERY_ICON_X, BATTERY_ICON_Y, 16, 8, bitmap);
}

void ui_rf_draw(signal_strength_t strength)
{
    const unsigned char *bitmap;

    switch (strength) {
    case SIGNAL_STRONG:
        bitmap = rf_4_bars;
        break;
    case SIGNAL_GOOD:
        bitmap = rf_3_bars;
        break;
    case SIGNAL_FAIR:
        bitmap = rf_2_bars;
        break;
    case SIGNAL_WEAK:
        bitmap = rf_1_bar;
        break;
    case SIGNAL_NONE:
    default:
        bitmap = rf_0_bars;
        break;
    }

    u8g2_DrawXBM(&u8g2, RF_ICON_X, RF_ICON_Y, 11, 8, bitmap);
}
