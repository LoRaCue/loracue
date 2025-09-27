# LoRaCue Wokwi Simulator

Test the complete LoRaCue system without physical hardware using the Wokwi online simulator.

## 🎮 Quick Start

```bash
# Build firmware for simulator
make sim-build

# Get simulator instructions
make sim-web
```

## 🌐 Using Wokwi Web Simulator

1. **Go to Wokwi**: https://wokwi.com/projects/new/esp32
2. **Upload diagram**: Copy contents of `diagram.json` 
3. **Upload firmware**: Upload `build/loracue.bin`
4. **Start simulation**: Click the green play button

## 🔌 Simulated Hardware

### ESP32-S3 Development Board
- **Microcontroller**: ESP32-S3 with dual-core CPU
- **Flash**: 8MB (simulated)
- **RAM**: 512KB SRAM + 8MB PSRAM

### Peripherals
- **OLED Display**: SSD1306 128x64 (SH1106 compatible)
  - SDA: GPIO17
  - SCL: GPIO18
- **Buttons**: Two pushbuttons for navigation
  - PREV: GPIO45 (Red button)
  - NEXT: GPIO46 (Green button)
- **Battery Simulation**: Potentiometer on GPIO1
  - Simulates battery voltage 3.0V - 4.2V
- **Status LEDs**: Visual feedback
  - Power: GPIO2 (Green)
  - LoRa TX: GPIO3 (Blue) 
  - LoRa RX: GPIO4 (Yellow)

## 🎯 Testing Scenarios

### 1. UI Navigation Test
- **Boot sequence**: Watch boot logo → main screen
- **Menu navigation**: Hold both buttons to enter menu
- **Button response**: Test PREV/NEXT navigation
- **Menu hierarchy**: Navigate through all menu levels

### 2. Power Management Test
- **Battery monitoring**: Adjust potentiometer to see voltage changes
- **Sleep simulation**: Test inactivity timeouts
- **Wake behavior**: Button presses should wake system

### 3. System Integration Test
- **Component initialization**: All systems start correctly
- **State persistence**: Settings survive simulated "reboots"
- **Error handling**: System gracefully handles edge cases

## 🔧 Simulator Features

### What Works
✅ **Complete UI system** - Full OLED menu navigation  
✅ **Button input** - Two-button navigation with timing  
✅ **Battery monitoring** - Voltage reading via potentiometer  
✅ **Power management** - Sleep mode simulation  
✅ **Device registry** - NVS storage simulation  
✅ **System logging** - Full ESP-IDF logging output  

### What's Simulated
🔄 **LoRa communication** - Placeholder (no RF simulation)  
🔄 **USB HID** - Placeholder (no USB host simulation)  
🔄 **Wi-Fi** - Limited simulation capabilities  
🔄 **Hardware encryption** - Software fallback  

## 📊 Performance Analysis

### Memory Usage (Simulated)
- **Flash**: ~1.5MB firmware + bootloader
- **RAM**: ~200KB runtime allocation
- **NVS**: Device registry and settings storage

### Timing Analysis
- **Boot time**: ~3-5 seconds (simulated)
- **UI response**: <100ms button to display update
- **Sleep entry**: Configurable timeouts (30s/5min)

## 🐛 Debugging Features

### Serial Monitor
- **ESP-IDF logs**: Full system logging output
- **Component traces**: Detailed component behavior
- **Error messages**: Clear error reporting

### Visual Indicators
- **OLED display**: Real-time UI state visualization
- **Status LEDs**: System state indication
- **Button feedback**: Visual button press confirmation

## 🚀 Development Workflow

### 1. Code → Simulate → Test
```bash
# Make changes to code
vim components/oled_ui/oled_ui.c

# Build for simulator
make sim-build

# Test in Wokwi
# Upload new firmware and test changes
```

### 2. Rapid Prototyping
- **UI design**: Test menu layouts and navigation
- **Logic validation**: Verify state machines and algorithms
- **Integration testing**: Test component interactions

### 3. Hardware Preparation
- **Pin validation**: Verify pin assignments match hardware
- **Timing verification**: Ensure timing constraints are met
- **Resource usage**: Check memory and performance limits

## 💡 Tips & Tricks

### Effective Testing
1. **Start simple**: Test basic functionality first
2. **Use logging**: Add ESP_LOGI statements for debugging
3. **Test edge cases**: Try unusual button combinations
4. **Verify persistence**: Test settings across "reboots"

### Common Issues
- **Build errors**: Ensure ESP-IDF v5.5 is properly configured
- **Upload issues**: Check firmware binary size and format
- **Simulation lag**: Complex operations may run slower than hardware

### Advanced Usage
- **Custom scenarios**: Modify `diagram.json` for different hardware configs
- **Automated testing**: Script button presses for regression testing
- **Performance profiling**: Use Wokwi's built-in performance tools

## 🔗 Resources

- **Wokwi Documentation**: https://docs.wokwi.com/
- **ESP32-S3 Reference**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/
- **SSD1306 Display**: https://docs.wokwi.com/parts/wokwi-ssd1306

## 🎯 Next Steps

After successful simulation:
1. **Order hardware**: Heltec LoRa V3 development boards
2. **Build prototype**: Assemble physical system
3. **Hardware validation**: Compare simulation vs. real hardware
4. **Production testing**: Full system integration testing

The simulator provides 90%+ of the development and testing capabilities without requiring physical hardware! 🎉
