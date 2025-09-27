# LoRaCue - Enterprise LoRa Presentation Clicker

**Current Status**: Foundation complete, ready for BSP implementation
**Build Status**: ✅ Compiles successfully (ESP-IDF v5.5)
**Next Task**: Ticket 1.1 - BSP Core Infrastructure

Enterprise-grade presentation clicker with long-range LoRa communication based on ESP32-S3 and Heltec LoRa V3 boards.

## Quick Context (for AI)
- **What**: Wireless presentation remote with >100m range, <50ms latency
- **Hardware**: Heltec LoRa V3 (ESP32-S3 + SX1262 LoRa + SH1106 OLED)
- **Architecture**: BSP abstraction, dual OTA, AES encryption, USB-C pairing
- **Status**: Project structure complete, ready for hardware implementation

## Current Implementation Status
- [x] ESP-IDF v5.5 project setup
- [x] BSP architecture design  
- [x] Build system (Make, CMake, partitions)
- [x] CI/CD pipeline (GitHub Actions)
- [x] Development workflow (Husky, Commitlint)
- [ ] **NEXT**: BSP hardware abstraction (Ticket 1.1)
- [ ] LoRa communication layer
- [ ] USB HID implementation
- [ ] OLED user interface
- [ ] Secure pairing system

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
