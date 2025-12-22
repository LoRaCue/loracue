# LilyGO T3 E-Paper Pinout Reference

**Board**: T3-S3 E-Paper  
**MCU**: ESP32-S3  
**LoRa**: SX1262  
**Display**: SSD1681 E-Paper 2.13" 250x122  

## Pin Assignments

### LoRa SX1262 (SPI)
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
| CS       | 7    | 7   | SPI Chip Select |
| SCK      | 5    | 5   | SPI Clock |
| MISO     | 3    | 3   | SPI Master In |
| MOSI     | 6    | 6   | SPI Master Out |
| RST      | 8    | 8   | Reset |
| BUSY     | 34   | 34  | Busy Status |
| DIO1     | 33   | 33  | Digital I/O 1 |
| POW      | 35   | 35  | Power Control |

### E-Paper Display SSD1681 (SPI)
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
| CS       | 15   | 15  | SPI Chip Select |
| CLK      | 14   | 14  | SPI Clock |
| MOSI     | 11   | 11  | SPI Master Out |
| DC       | 16   | 16  | Data/Command |
| RST      | 47   | 47  | Reset |
| BUSY     | 48   | 48  | Busy Status |

### SD Card (Shared SPI with E-Paper)
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
| CS       | 13   | 13  | SPI Chip Select |
| CLK      | 14   | 14  | SPI Clock (shared) |
| MOSI     | 11   | 11  | SPI Master Out (shared) |
| MISO     | 2    | 2   | SPI Master In |

### User Interface
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
| BUTTON   | 0    | 0   | User Button |
| LED      | 37   | 37  | Status LED |

### Power & Battery
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
| VBAT     | 1    | 1   | Battery ADC |

### Communication
| Function | GPIO | Pin | Description |
|----------|------|-----|-------------|
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
- **Display**: 2.13" E-Paper 250x122 (monochrome)
- **SD Card**: MicroSD card slot

## SPI Bus Configuration
The board uses two separate SPI buses:
- **SPI2**: LoRa module (GPIO 3, 5, 6, 7)
- **SPI3**: E-Paper display + SD card (GPIO 11, 14, 15 + 2, 13)

## Official Resources
- **GitHub**: [LilyGO LoRa E-Paper Series](https://github.com/Xinyuan-LilyGO/Lilygo-LoRa-Epaper-series)
- **Product Page**: [LilyGO T3-S3](https://www.lilygo.cc/products/t3-s3)

## LoRaCue Usage
This pinout is used in the LoRaCue project BSP (Board Support Package) for the LC-Beta model. The E-Paper display provides excellent outdoor visibility and ultra-low power consumption in sleep mode.
