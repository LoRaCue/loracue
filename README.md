# LoRaCue - Enterprise LoRa Presentation Clicker

Enterprise-grade presentation clicker with long-range LoRa communication based on ESP32-S3 and Heltec LoRa V3 boards.

## Features

- Long-range LoRa communication (SF7/BW500kHz for low latency)
- Hardware-accelerated AES encryption
- USB-HID keyboard emulation
- OLED display with intuitive UI
- Battery monitoring and power management
- OTA firmware updates
- Multi-board BSP architecture

## Hardware Requirements

- Heltec LoRa V3 (ESP32-S3 + SX1262) development boards
- USB-C cables for programming and pairing

## Development Setup

### Prerequisites

1. **ESP-IDF v5.5**: Install from [Espressif's official guide](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/get-started/index.html)
2. **Node.js**: For development tools (commitlint, husky)

### Quick Start

```bash
# Clone and setup
git clone <repository-url>
cd LoRaCue
npm install

# Build firmware
idf.py set-target esp32s3
idf.py build

# Flash to device
idf.py -p /dev/ttyUSB0 flash monitor
```

### Development Workflow

- Use conventional commits (enforced by commitlint)
- All commits are automatically built via GitHub Actions
- Semantic versioning with GitVersion

## Project Structure

```
loracue/
├── main/                   # Main application
├── components/bsp/         # Board Support Package
├── docs/                   # Documentation
├── .github/workflows/      # CI/CD pipelines
└── partitions.csv         # Flash partition table
```

## License

[Add your license here]
