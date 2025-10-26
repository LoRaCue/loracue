# Wokwi Simulation Configurations

This directory contains board-specific Wokwi simulation configurations for LoRaCue.

## Directory Structure

```
wokwi/
├── chips/              # Custom Wokwi chip implementations
│   ├── uart.chip.c     # UART bridge chip for RFC2217
│   ├── sx1262.chip.c   # SX1262 LoRa transceiver simulation
│   └── ...
├── heltec_v3/          # Heltec WiFi LoRa 32 V3 simulation
│   ├── diagram.json    # Hardware diagram
│   └── wokwi.toml      # Build configuration
├── lilygo_t5/          # LilyGO T5 EPD47 simulation
│   ├── diagram.json    # Hardware diagram
│   └── wokwi.toml      # Build configuration
└── wokwi/              # Generic ESP32-S3 simulation
    ├── diagram.json    # Hardware diagram
    └── wokwi.toml      # Build configuration
```

## Usage

### Run simulation for specific board:
```bash
make sim-run WOKWI_BOARD=heltec_v3
make sim-run WOKWI_BOARD=lilygo_t5
make sim-run WOKWI_BOARD=wokwi  # Generic
```

### Default (generic):
```bash
make sim-run
```

## Custom Chips

### UART Bridge Chip
- **Purpose**: RFC2217 serial port forwarding
- **Port**: localhost:4000
- **Usage**: `telnet localhost 4000` for UART0 access

### SX1262 LoRa Chip
- **Purpose**: LoRa transceiver simulation
- **Features**: SPI communication, interrupt handling
- **Registers**: Full SX1262 register set emulation

## Adding New Boards

1. Create directory: `wokwi/your_board/`
2. Add `diagram.json` with hardware connections
3. Create `wokwi.toml` with build configuration:
   ```toml
   [wokwi]
   version = 1
   firmware = "../../build/your_board.bin"
   elf = "../../build/your_board.elf"
   
   [[chip]]
   name = "uart"
   binary = "../../build/wokwi/chips/uart.chip.wasm"
   
   [[chip]]
   name = "sx1262"
   binary = "../../build/wokwi/chips/sx1262.chip.wasm"
   ```
4. Run: `make sim-run WOKWI_BOARD=your_board`

## Build Process

Custom chips are compiled to WebAssembly:
```bash
make chips  # Compile all custom chips
```

Output: `build/wokwi/chips/*.wasm`
