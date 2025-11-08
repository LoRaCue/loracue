# I2Console ESP-IDF Component

[![Component Registry](https://components.espressif.com/components/metaneutrons/i2console/badge.svg)](https://components.espressif.com/components/metaneutrons/i2console)

I2C to USB-CDC console bridge driver for ESP-IDF. Automatically mirrors ESP_LOG output to I2Console device.

## Features

- **Auto-detection**: Probes I2C bus for I2Console device (ID: 0x12C0)
- **Automatic logging**: Hooks into ESP-IDF logging system
- **Non-blocking**: Queue-based writes
- **Graceful fallback**: Continues with UART if device not found
- **Version query**: Read I2Console firmware version

## Installation

### Using ESP Component Manager (Recommended)

Add to your project's `idf_component.yml`:

```yaml
dependencies:
  metaneutrons/i2console:
    version: "^0.1.0"
```

Or via command line:

```bash
idf.py add-dependency "metaneutrons/i2console^0.1.0"
```

### Manual Installation

Copy this directory to your project's `components/` folder.

## Quick Start

```c
#include "driver/i2c.h"
#include "i2console.h"

void app_main(void)
{
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
    
    // Initialize I2Console (auto-detects device)
    i2console_init(I2C_NUM_0, 0x37);
    
    // All ESP_LOG calls now mirrored to I2Console!
    ESP_LOGI("APP", "Hello I2Console!");
}
```

## Hardware Setup

Connect I2Console device (RP2350-GEEK with I2Console firmware) to ESP32:

```
ESP32 GPIO21 (SDA) → I2Console GPIO28 (SDA)
ESP32 GPIO22 (SCL) → I2Console GPIO29 (SCL)
GND                → GND
```

## API Reference

### `i2console_init()`
```c
esp_err_t i2console_init(i2c_port_t port, uint8_t addr);
```
Initialize component. Returns `ESP_ERR_NOT_FOUND` if device not detected.

**Parameters:**
- `port`: I2C port number (must be already initialized)
- `addr`: I2C slave address (default: 0x37)

### `i2console_write()`
```c
esp_err_t i2console_write(const char *data, size_t len);
```
Manually write data to I2Console.

### `i2console_is_connected()`
```c
bool i2console_is_connected(void);
```
Check if device is connected.

### `i2console_get_version()`
```c
esp_err_t i2console_get_version(char *version);
```
Get firmware version string (16 bytes).

## Configuration

Via `idf.py menuconfig` → Component config → I2Console Configuration:
- Enable/disable component
- Change I2C address

## Example

See [example](../../) directory for complete working example.

## License

GPL-3.0

## Links

- [I2Console Firmware](https://github.com/metaneutrons/I2Console)
- [Documentation](https://github.com/metaneutrons/I2Console/blob/main/README.md)
- [Issues](https://github.com/metaneutrons/I2Console/issues)
