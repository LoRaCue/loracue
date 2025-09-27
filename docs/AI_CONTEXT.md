# AI Context Preservation - LoRaCue Project

## WHAT WE'VE BUILT
- ✅ ESP-IDF v5.5 project structure with BSP architecture
- ✅ Complete build system (Makefile, CMakeLists.txt, partitions)
- ✅ GitHub Actions CI/CD pipeline
- ✅ Conventional commits with Husky/Commitlint
- ✅ 8MB partition table with dual OTA + SPIFFS + coredump

## KEY DECISIONS MADE
- **Framework**: ESP-IDF v5.5 (NOT Arduino) for enterprise grade
- **Hardware**: Heltec LoRa V3 (ESP32-S3 + SX1262 + SH1106 OLED)
- **Architecture**: BSP abstraction layer for multi-board support
- **Protocol**: SF7/BW500kHz LoRa for <50ms latency vs range tradeoff
- **Security**: AES-128 hardware encryption with per-device keys
- **Pairing**: USB-C secure pairing (PC=Host, STAGE=Device)

## CURRENT STATE
- Project builds successfully (289KB firmware, 72% partition free)
- Ready to implement Ticket 1.1: BSP Core Infrastructure
- All tooling configured (make build/flash/monitor works)

## PIN MAPPING (Heltec LoRa V3)
```
LoRa SX1262:  MOSI=11, MISO=10, SCK=9, CS=8, BUSY=13, DIO1=14, RST=12
OLED SH1106:  SDA=17, SCL=18  
Buttons:      PREV=46, NEXT=45 (with pullups)
Battery:      ADC=1, CTRL=37 (voltage divider)
```

## NEXT STEPS
1. Implement BSP hardware abstraction (GPIO, SPI, I2C, ADC)
2. Add LoRa driver integration (nopnop2002/esp-idf-sx126x)
3. Create USB HID composite device (keyboard + CDC)

## FILES TO REFERENCE
- `/docs/blueprint.md` - Complete technical specification
- `/docs/IMPLEMENTATION_ROADMAP.md` - Phase-by-phase tickets
- `/components/bsp/` - Hardware abstraction layer
- `/main/` - Application logic

This file preserves context across AI conversation resets.
