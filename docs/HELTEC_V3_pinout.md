# Heltec LoRa V3 Pinout Reference

**Board**: WiFi LoRa 32 V3  
**MCU**: ESP32-S3FN8  
**LoRa**: SX1262  
**Display**: SH1106 OLED 128x64  

## Pin Assignments

### LoRa SX1262 (SPI)
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
| NSS/CS   | 8    | 8   | SPI Chip Select |
| SCK      | 9    | 9   | SPI Clock |
| MISO     | 10   | 10  | SPI Master In |
| MOSI     | 11   | 11  | SPI Master Out |
| RST      | 12   | 12  | Reset |
| BUSY     | 13   | 13  | Busy Status |
| DIO1     | 14   | 14  | Digital I/O 1 |

### OLED Display SH1106 (I2C)
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
| SDA      | 17   | 17  | I2C Data |
| SCL      | 18   | 18  | I2C Clock |
| RST      | 21   | 21  | Reset |

### User Interface
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
| PREV     | 45   | 45  | Previous Button |
| NEXT     | 46   | 46  | Next Button |
| LED      | 35   | 35  | Status LED |

### Power & Battery
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
| VBAT     | 1    | 1   | Battery ADC |
| VBAT_CTRL| 37   | 37  | Battery Control |

### Communication
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
| UART_TX  | 43   | 43  | Serial TX |
| UART_RX  | 44   | 44  | Serial RX |
| USB_D-   | 19   | 19  | USB Data - |
| USB_D+   | 20   | 20  | USB Data + |

## Power Supply
- **Operating Voltage**: 3.3V
- **USB**: 5V via USB-C
- **Battery**: 3.7V Li-Po (JST connector)
- **Charging**: Built-in charging circuit

## Features
- **WiFi**: 2.4GHz 802.11 b/g/n
- **Bluetooth**: BLE 5.0
- **LoRa**: 863-928MHz (region dependent)
- **Flash**: 8MB SPI Flash
- **PSRAM**: 8MB PSRAM
- **Display**: 0.96" OLED 128x64

## Official Resources
- **Pinmap**: [Official Heltec Pinmap](https://resource.heltec.cn/download/WiFi_LoRa_32_V3/Wi-Fi_LoRa32_V3.2_Pinmap.png)
- **Documentation**: [Heltec Docs](https://docs.heltec.org/en/node/esp32/wifi_lora_32/index.html)
- **Product Page**: [Heltec WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/)

## LoRaCue Usage
This pinout is used in the LoRaCue project BSP (Board Support Package) for hardware abstraction. All pin definitions match the official Heltec specifications for maximum compatibility.
