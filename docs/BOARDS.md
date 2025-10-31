# Board Support

LoRaCue supports multiple hardware platforms with board-specific builds.

## Supported Boards

### 1. Heltec WiFi LoRa 32 V3
**Target:** `build-heltec`

- **MCU:** ESP32-S3 (Dual-core, 240MHz)
- **Display:** 128×64 OLED (SSD1306, I2C)
- **LoRa:** SX1262 (868/915MHz)
- **Input:** Single button
- **UI:** Embedded (u8g2)
- **Flash:** 8MB
- **Use Case:** Development, prototyping, portable remote

### 2. LilyGO T5 4.7" E-Paper
**Target:** `build-lilygo`

- **MCU:** ESP32-S3 (Dual-core, 240MHz)
- **Display:** 960×540 E-Paper (ED047TC1, 16 grayscale)
- **Touch:** GT911 capacitive touch
- **LoRa:** SX1262 (868/915MHz)
- **Battery:** BQ25896 charger + BQ27220 fuel gauge
- **RTC:** PCF85063
- **GPIO Expander:** PCA9535
- **Power Driver:** TPS65185
- **UI:** Rich (LVGL)
- **Flash:** 16MB
- **Use Case:** Professional presentations, conference halls

### 3. Wokwi Simulator
**Target:** `build-sim`

- **Platform:** Wokwi web simulator
- **Display:** 128×64 OLED (simulated)
- **LoRa:** Custom SX1262 chip (simulated)
- **UI:** Embedded (u8g2)
- **Use Case:** Development, testing, CI/CD

## Build Commands

### Build All Boards
```bash
make build
```
Builds firmware for all three boards.

### Build Specific Board
```bash
make build-heltec    # Heltec V3
make build-lilygo    # LilyGO T5
make build-sim       # Wokwi Simulator
```

### Clean Build
```bash
make fullclean       # Remove all build artifacts
make rebuild         # Clean + build all
```

## Configuration Files

Each board has a dedicated sdkconfig:

```
sdkconfig.heltec_v3   # Heltec V3 configuration
sdkconfig.lilygo_t5   # LilyGO T5 configuration
sdkconfig.wokwi       # Wokwi simulator configuration
```

## Board Selection Architecture

### Kconfig
```kconfig
choice LORACUE_BOARD
    config BOARD_HELTEC_V3
        select UI_EMBEDDED
    config BOARD_LILYGO_T5
        select UI_RICH
    config BOARD_WOKWI
        select UI_EMBEDDED
endchoice
```

### BSP Auto-Selection
```cmake
if(CONFIG_BOARD_LILYGO_T5)
    set(BSP_SRCS "bsp_lilygo_t5.c")
    set(BSP_REQUIRES ... pca9535 tps65185 gt911 ...)
elseif(CONFIG_BOARD_HELTEC_V3)
    set(BSP_SRCS "bsp_heltec_v3.c" ...)
    set(BSP_REQUIRES ... u8g2)
endif()
```

### Component Conditional Compilation

**LilyGO T5 only:**
- `pca9535` - GPIO expander
- `tps65185` - E-Paper power driver
- `gt911` - Touch controller
- `bq25896` - Battery charger
- `bq27220` - Fuel gauge
- `pcf85063` - RTC
- `lvgl` - Graphics library

**Heltec V3 / Wokwi:**
- `u8g2` - OLED graphics library

## Adding a New Board

1. **Create sdkconfig file:**
   ```bash
   cp sdkconfig.heltec_v3 sdkconfig.myboard
   ```

2. **Add to Kconfig.projbuild:**
   ```kconfig
   config BOARD_MYBOARD
       bool "My Custom Board"
       select UI_EMBEDDED  # or UI_RICH
   ```

3. **Create BSP file:**
   ```c
   // components/bsp/bsp_myboard.c
   esp_err_t bsp_init(void) { ... }
   ```

4. **Update BSP CMakeLists.txt:**
   ```cmake
   elseif(CONFIG_BOARD_MYBOARD)
       set(BSP_SRCS "bsp_myboard.c")
       set(BSP_REQUIRES ...)
   ```

5. **Add Makefile target:**
   ```makefile
   build-myboard: check-idf
       $(IDF_SETUP) idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.myboard" build
   ```

## CI/CD Integration

### GitHub Actions Matrix Build
```yaml
strategy:
  matrix:
    board: [heltec, lilygo, sim]
steps:
  - run: make build-${{ matrix.board }}
```

### Artifact Naming
```
loracue-heltec_v3-v1.0.0.bin
loracue-lilygo_t5-v1.0.0.bin
loracue-wokwi-v1.0.0.bin
```

## Flashing

### Heltec V3
```bash
make build-heltec
idf.py -p /dev/ttyUSB0 flash
```

### LilyGO T5
```bash
make build-lilygo
idf.py -p /dev/ttyUSB0 flash
```

### Wokwi
```bash
make sim-run
```

## Benefits

✅ **Single codebase** - No branches or forks  
✅ **Type-safe** - Kconfig validates configuration  
✅ **Scalable** - Easy to add new boards  
✅ **CI/CD friendly** - Matrix builds  
✅ **Zero conflicts** - Board-specific components only compiled when needed
