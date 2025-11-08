# I2Console ESP-IDF Example

Enterprise-grade example demonstrating I2Console integration with ESP-IDF v5.5+.

## Features

- **Auto-detection**: Automatically detects I2Console on I2C bus
- **Log mirroring**: All `ESP_LOG` output automatically sent to I2Console
- **Non-blocking**: Queue-based writes won't block your application
- **Graceful fallback**: If I2Console not found, continues with UART only

## Hardware Setup

1. **I2Console Device** (RP2350-GEEK running I2Console firmware)
   - Connect to ESP32 I2C bus
   - Default address: 0x37

2. **ESP32 Connections**:
   ```
   ESP32 GPIO21 (SDA) → I2Console GPIO28 (SDA)
   ESP32 GPIO22 (SCL) → I2Console GPIO29 (SCL)
   GND                → GND
   ```

3. **USB Connection**:
   - Connect I2Console to PC via USB
   - Open serial terminal to see mirrored logs

## Building

```bash
# Set ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Configure (optional - defaults work)
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Configuration

Via `idf.py menuconfig`:
- **Component config → I2Console Configuration**
  - Enable/disable I2Console
  - Change I2C address (default: 0x37)

## Usage in Your Project

1. Copy `components/i2console/` to your project's `components/` directory

2. Initialize I2C and I2Console:
   ```c
   #include "i2console.h"
   
   // Initialize I2C master
   i2c_config_t conf = {
       .mode = I2C_MODE_MASTER,
       .sda_io_num = 21,
       .scl_io_num = 22,
       .sda_pullup_en = GPIO_PULLUP_ENABLE,
       .scl_pullup_en = GPIO_PULLUP_ENABLE,
       .master.clk_speed = 100000,
   };
   i2c_param_config(I2C_NUM_0, &conf);
   i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
   
   // Initialize I2Console (auto-detects)
   i2console_init(I2C_NUM_0, 0x37);
   
   // All ESP_LOG calls now mirrored to I2Console!
   ESP_LOGI("APP", "Hello I2Console!");
   ```

3. That's it! All logging automatically goes to both UART and I2Console.

## API Reference

### `i2console_init()`
```c
esp_err_t i2console_init(i2c_port_t port, uint8_t addr);
```
Initialize I2Console component. Returns `ESP_ERR_NOT_FOUND` if device not detected.

### `i2console_write()`
```c
esp_err_t i2console_write(const char *data, size_t len);
```
Manually write data to I2Console (bypasses ESP_LOG).

### `i2console_is_connected()`
```c
bool i2console_is_connected(void);
```
Check if I2Console device is connected and operational.

### `i2console_get_version()`
```c
esp_err_t i2console_get_version(char *version);
```
Get I2Console firmware version string.

## Troubleshooting

**I2Console not detected:**
- Check I2C connections (SDA, SCL, GND)
- Verify I2Console is powered and running
- Check I2C address matches (default: 0x37)
- Use `i2cdetect` to scan bus

**Logs not appearing:**
- Verify USB connection to I2Console
- Open serial terminal to I2Console USB port
- Check ESP_LOG level in menuconfig

## License

GPL-3.0 - See LICENSE file in repository root
