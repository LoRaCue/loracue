# Display Driver Refactor Plan

**Branch:** `feature/refactor-display-driver`  
**Goal:** Migrate from u8g2 to unified esp_lcd + LVGL architecture  
**Status:** Planning Phase

## 🎯 Objectives

1. **Unified Display API**: Single abstraction layer for OLED, E-Paper, and TFT displays
2. **LVGL Integration**: Professional UI framework with touch support
3. **Multi-Model Support**: Seamless support for LC-Alpha (OLED), LC-Beta (E-Paper), LC-Gamma (TFT+Touch)
4. **Backward Compatibility**: Keep ui_mini working during transition
5. **Future-Proof**: Ready for touch displays and advanced UI features

## 📊 Current vs Target Architecture

### Current Architecture
```
ui_mini → u8g2 → SSD1306 (OLED only)
         ↓
    Direct hardware calls
```

### Target Architecture
```
ui_lvgl → LVGL → esp_lcd → Display Drivers
                    ↓
         ┌──────────┼──────────┐
         ↓          ↓          ↓
    SSD1306    SSD1681    ILI9341
    (OLED)    (E-Paper)    (TFT)
```

## 🗂️ Component Structure

```
components/
├── display/                    # NEW: Display abstraction layer
│   ├── display.c              # Unified display API
│   ├── display_ssd1306.c      # OLED driver (esp_lcd)
│   ├── display_ssd1681.c      # E-Paper driver (esp_lcd)
│   ├── include/
│   │   └── display.h          # Public API
│   └── CMakeLists.txt
│
├── ui_lvgl/                   # NEW: LVGL-based UI
│   ├── ui_lvgl.c              # Main UI controller
│   ├── lv_port_disp.c         # LVGL display driver
│   ├── lv_port_indev.c        # LVGL input driver (buttons)
│   ├── screens/               # LVGL screen implementations
│   │   ├── screen_boot.c
│   │   ├── screen_main.c
│   │   ├── screen_menu.c
│   │   ├── screen_settings.c
│   │   ├── screen_ota.c
│   │   └── screen_pairing.c
│   ├── widgets/               # Custom LVGL widgets
│   │   ├── widget_statusbar.c
│   │   ├── widget_battery.c
│   │   └── widget_rssi.c
│   ├── include/
│   │   └── ui_lvgl.h
│   └── CMakeLists.txt
│
├── ui_mini/                   # EXISTING: Keep for backward compat
│   └── ...                    # (unchanged)
│
├── bsp/
│   ├── bsp_heltec_v3.c        # UPDATE: Use esp_lcd
│   ├── bsp_lilygo_t3.c        # NEW: T3-S3 E-Paper BSP
│   └── ...
│
└── ui_interface/              # UPDATE: Support both UIs
    └── ui_interface.h
```

## 📋 Implementation Phases

### Phase 1: Foundation (Week 1)
**Goal:** Set up esp_lcd infrastructure

- [ ] Add esp_lcd dependencies to `main/idf_component.yml`
- [ ] Create `components/display/` with abstraction layer
- [ ] Implement `display_ssd1306.c` for OLED
- [ ] Implement `display_ssd1681.c` for E-Paper
- [ ] Create display API header (`display.h`)
- [ ] Add LVGL configuration to sdkconfig

**Deliverable:** Working esp_lcd drivers for both display types

### Phase 2: LVGL Integration (Week 1-2)
**Goal:** Get LVGL rendering on displays

- [ ] Create `components/ui_lvgl/` structure
- [ ] Implement `lv_port_disp.c` (display driver)
- [ ] Implement `lv_port_indev.c` (button input)
- [ ] Configure LVGL memory management
- [ ] Create LVGL task and timer
- [ ] Test basic LVGL rendering (hello world)

**Deliverable:** LVGL rendering "Hello World" on both displays

### Phase 3: Screen Porting (Week 2-3)
**Goal:** Port all ui_mini screens to LVGL

- [ ] Port boot screen
- [ ] Port main screen with status bar
- [ ] Port menu system (LVGL list widget)
- [ ] Port settings screens
- [ ] Port OTA progress screen
- [ ] Port BLE pairing screen
- [ ] Implement status bar widget
- [ ] Implement battery indicator widget
- [ ] Implement RSSI indicator widget

**Deliverable:** Feature parity with ui_mini

### Phase 4: BSP Updates (Week 3)
**Goal:** Update BSPs to use new display system

- [ ] Update `bsp_heltec_v3.c` to use esp_lcd
- [ ] Create `bsp_lilygo_t3.c` for T3-S3
- [ ] Create `sdkconfig.lilygo_t3`
- [ ] Create `sdkconfig.model_beta`
- [ ] Update BSP Kconfig for display selection
- [ ] Update CMakeLists.txt for conditional UI compilation

**Deliverable:** Both BSPs working with new display system

### Phase 5: E-Paper Optimization (Week 4)
**Goal:** Optimize E-Paper refresh strategy

- [ ] Implement partial refresh for status updates
- [ ] Implement full refresh for screen transitions
- [ ] Add refresh counter (full refresh every 10 partials)
- [ ] Optimize LVGL buffer size for E-Paper
- [ ] Test ghosting prevention strategy

**Deliverable:** Optimized E-Paper performance

### Phase 6: Testing & Validation (Week 4-5)
**Goal:** Comprehensive testing on all hardware

- [ ] Test OLED on Heltec V3 (LC-Alpha/Alpha+)
- [ ] Test E-Paper on T3-S3 (LC-Beta)
- [ ] Test all UI screens on both displays
- [ ] Test button input on both models
- [ ] Test system events integration
- [ ] Test OTA updates with new UI
- [ ] Test BLE pairing flow
- [ ] Benchmark memory usage (RAM/Flash)
- [ ] Benchmark performance (FPS, refresh time)

**Deliverable:** Validated on real hardware

### Phase 7: Wokwi & Documentation (Week 5)
**Goal:** Simulation and documentation

- [ ] Create Wokwi simulation for T3-S3
- [ ] Update Makefile with LC-Beta target
- [ ] Update README.md with new architecture
- [ ] Create migration guide
- [ ] Document LVGL customization
- [ ] Update CI/CD for dual UI builds

**Deliverable:** Complete documentation

### Phase 8: Cleanup & Release (Week 5-6)
**Goal:** Production-ready code

- [ ] Code review and refactoring
- [ ] Optimize memory usage
- [ ] Remove debug code
- [ ] Update version numbers
- [ ] Create comprehensive PR
- [ ] Merge to develop

**Deliverable:** Production-ready release

## 🔧 Technical Details

### Display Abstraction API

```c
// display.h - Unified display API
typedef enum {
    DISPLAY_TYPE_OLED_SSD1306,
    DISPLAY_TYPE_EPAPER_SSD1681,
    DISPLAY_TYPE_TFT_ILI9341,
} display_type_t;

typedef struct {
    display_type_t type;
    uint16_t width;
    uint16_t height;
    esp_lcd_panel_handle_t panel;
    lv_disp_t *lv_disp;
} display_handle_t;

// Initialize display
display_handle_t* display_init(display_type_t type, const display_config_t *config);

// LVGL integration
void display_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
void display_lvgl_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area);

// E-Paper specific
void display_epaper_set_refresh_mode(display_handle_t *disp, bool full_refresh);
```

### LVGL Configuration

```c
// sdkconfig.defaults additions
CONFIG_LV_MEM_CUSTOM=y
CONFIG_LV_MEM_SIZE=65536
CONFIG_LV_COLOR_DEPTH_1=y  // For OLED/E-Paper
CONFIG_LV_DPI_DEF=128
CONFIG_LV_TICK_CUSTOM=y
CONFIG_LV_DISP_DEF_REFR_PERIOD=30
```

### Memory Budget

| Component | OLED (128x64) | E-Paper (250x122) |
|-----------|---------------|-------------------|
| Display Buffer | 1KB | 4KB |
| LVGL Heap | 64KB | 64KB |
| UI Code | ~50KB | ~50KB |
| **Total** | **~115KB** | **~118KB** |

### E-Paper Refresh Strategy

```c
// Refresh counter for ghosting prevention
static uint8_t partial_refresh_count = 0;
#define FULL_REFRESH_INTERVAL 10

void display_epaper_refresh(display_handle_t *disp, bool force_full) {
    if (force_full || partial_refresh_count >= FULL_REFRESH_INTERVAL) {
        // Full refresh (2-3 seconds)
        display_epaper_set_refresh_mode(disp, true);
        partial_refresh_count = 0;
    } else {
        // Partial refresh (0.3 seconds)
        display_epaper_set_refresh_mode(disp, false);
        partial_refresh_count++;
    }
}
```

## 🎨 UI Design Considerations

### OLED (128x64) - Monochrome
- Compact layout
- High contrast
- Fast refresh (instant)
- Always-on display

### E-Paper (250x122) - Monochrome
- Larger canvas
- Slower refresh (2-3s full, 0.3s partial)
- Zero power when static
- Sunlight readable

### Future TFT (320x240+) - Color + Touch
- Full color UI
- Touch gestures
- Smooth animations
- Interactive widgets

## 🚧 Known Challenges

1. **E-Paper Refresh Speed**: Need smart refresh strategy
2. **Memory Constraints**: LVGL uses more RAM than u8g2
3. **LVGL Learning Curve**: Team needs to learn LVGL API
4. **Backward Compatibility**: Keep ui_mini working during transition
5. **Testing Complexity**: Need to test on multiple hardware variants

## 📈 Success Metrics

- [ ] All UI screens working on OLED
- [ ] All UI screens working on E-Paper
- [ ] Memory usage < 150KB RAM
- [ ] E-Paper refresh < 1s for status updates
- [ ] No visual glitches or artifacts
- [ ] Smooth button navigation
- [ ] System events properly integrated
- [ ] OTA updates working
- [ ] BLE pairing working

## 🔄 Rollback Plan

If migration fails:
1. Keep `ui_mini` as default
2. Make `ui_lvgl` optional (Kconfig)
3. Document issues and blockers
4. Revisit architecture decisions

## 📚 Resources

- [ESP-IDF LCD Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/lcd/index.html)
- [LVGL Documentation](https://docs.lvgl.io/)
- [esp_lcd Component Registry](https://components.espressif.com/components?q=esp_lcd)
- [LVGL ESP32 Examples](https://github.com/lvgl/lv_port_esp32)

## 👥 Team Notes

- **Estimated Effort**: 5-6 weeks (1 developer)
- **Risk Level**: Medium (new framework, hardware dependencies)
- **Priority**: High (blocks LC-Beta development)
- **Dependencies**: None (can proceed immediately)

---

**Last Updated**: 2025-11-18  
**Status**: Planning Complete - Ready for Implementation
