#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "circular_buffer.h"
#include "i2c_slave.h"
#include "usb_cdc.h"
#include "flash_config.h"
#include "lcd_ui.h"
#include "log.h"
#include "version.h"

#define TX_BUFFER_SIZE 256
#define RX_BUFFER_SIZE 1024
#define WATCHDOG_TIMEOUT_MS 8000
#define UI_UPDATE_INTERVAL_MS 100

static uint8_t tx_buffer_data[TX_BUFFER_SIZE];
static uint8_t rx_buffer_data[RX_BUFFER_SIZE];
static circular_buffer_t tx_buffer;
static circular_buffer_t rx_buffer;

int main(void) {
    // Initialize system
    
    // Enable watchdog
    if (watchdog_caused_reboot()) {
        // Will log after USB init
    }
    watchdog_enable(WATCHDOG_TIMEOUT_MS, 1);
    
    // Initialize flash config
    flash_config_init();
    
    // Initialize buffers
    circular_buffer_init(&tx_buffer, tx_buffer_data, TX_BUFFER_SIZE);
    circular_buffer_init(&rx_buffer, rx_buffer_data, RX_BUFFER_SIZE);
    
    // Initialize peripherals
    usb_cdc_init();
    log_init();
    
    // Wait for USB to stabilize
    sleep_ms(1000);
    
    LOG_INFO("I2Console starting...");
    LOG_INFO("Firmware version: %s", FW_VERSION);
    LOG_INFO("I2C address: 0x%02X", flash_config_get_i2c_address());
    
    if (watchdog_caused_reboot()) {
        LOG_WARN("System recovered from watchdog reset");
    }
    
    i2c_slave_init(&tx_buffer, &rx_buffer);
    LOG_INFO("I2C slave initialized on GPIO28/29");
    
    lcd_ui_init();
    LOG_INFO("LCD initialized");
    
    LOG_INFO("System ready");
    
    uint32_t last_ui_update = 0;
    
    while (1) {
        watchdog_update();
        
        // USB CDC task
        usb_cdc_task();
        
        // Check for bootloader command
        usb_cdc_check_bootloader_cmd();
        
        // Transfer data from TX buffer to USB-CDC
        if (usb_cdc_connected()) {
            uint8_t data;
            int count = 0;
            while (circular_buffer_pop(&tx_buffer, &data) && count < 64) {
                usb_cdc_write(&data, 1);
                count++;
            }
        }
        
        // Transfer data from USB-CDC to RX buffer
        uint8_t usb_buf[64];
        int len = usb_cdc_read(usb_buf, sizeof(usb_buf));
        for (int i = 0; i < len; i++) {
            circular_buffer_push(&rx_buffer, usb_buf[i]);
        }
        
        // Update UI periodically
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_ui_update >= UI_UPDATE_INTERVAL_MS) {
            last_ui_update = now;
            
            i2c_stats_t stats = i2c_slave_get_stats();
            uint32_t total_errors = stats.tx_overflow + stats.rx_overflow + stats.i2c_errors;
            
            lcd_ui_update(
                flash_config_get_i2c_address(),
                circular_buffer_available(&tx_buffer),
                circular_buffer_available(&rx_buffer),
                stats.tx_bytes,
                stats.rx_bytes,
                usb_cdc_connected(),
                total_errors
            );
        }
        
        tight_loop_contents();
    }
    
    return 0;
}
