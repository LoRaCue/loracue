# Display Refactor Quick Checklist

**Branch:** `feature/refactor-display-driver`  
**TODO ID:** `1763481816328`

## 📦 Phase 1: Foundation (Week 1)

### Dependencies
- [ ] Add `espressif/esp_lcd_ssd1306` to `main/idf_component.yml`
- [ ] Add `espressif/esp_lcd_ssd1681` to `main/idf_component.yml`
- [ ] Verify LVGL version `^9.2.0` in dependencies

### Display Component
- [ ] Create `components/display/` directory
- [ ] Create `components/display/include/display.h`
- [ ] Create `components/display/display.c`
- [ ] Create `components/display/display_ssd1306.c`
- [ ] Create `components/display/display_ssd1681.c`
- [ ] Create `components/display/CMakeLists.txt`
- [ ] Test build with new component

## 🎨 Phase 2: LVGL Integration (Week 1-2)

### UI LVGL Component
- [ ] Create `components/ui_lvgl/` directory
- [ ] Create `components/ui_lvgl/ui_lvgl.c`
- [ ] Create `components/ui_lvgl/lv_port_disp.c`
- [ ] Create `components/ui_lvgl/lv_port_indev.c`
- [ ] Create `components/ui_lvgl/include/ui_lvgl.h`
- [ ] Create `components/ui_lvgl/CMakeLists.txt`

### LVGL Configuration
- [ ] Add LVGL config to `sdkconfig.defaults`
- [ ] Configure memory management (64KB heap)
- [ ] Configure color depth (1-bit for mono displays)
- [ ] Configure tick timer
- [ ] Test LVGL "Hello World"

## 🖼️ Phase 3: Screen Porting (Week 2-3)

### Screen Files
- [ ] Create `components/ui_lvgl/screens/screen_boot.c`
- [ ] Create `components/ui_lvgl/screens/screen_main.c`
- [ ] Create `components/ui_lvgl/screens/screen_menu.c`
- [ ] Create `components/ui_lvgl/screens/screen_settings.c`
- [ ] Create `components/ui_lvgl/screens/screen_ota.c`
- [ ] Create `components/ui_lvgl/screens/screen_pairing.c`

### Widget Files
- [ ] Create `components/ui_lvgl/widgets/widget_statusbar.c`
- [ ] Create `components/ui_lvgl/widgets/widget_battery.c`
- [ ] Create `components/ui_lvgl/widgets/widget_rssi.c`

### Screen Implementation
- [ ] Port boot screen logic
- [ ] Port main screen logic
- [ ] Port menu navigation
- [ ] Port settings screens
- [ ] Port OTA progress
- [ ] Port BLE pairing UI

## 🔧 Phase 4: BSP Updates (Week 3)

### Heltec V3 BSP
- [ ] Update `components/bsp/bsp_heltec_v3.c` to use esp_lcd
- [ ] Remove u8g2 initialization
- [ ] Add esp_lcd_ssd1306 initialization
- [ ] Test on real hardware

### LilyGO T3-S3 BSP
- [ ] Create `components/bsp/bsp_lilygo_t3.c`
- [ ] Implement SPI initialization for E-Paper
- [ ] Implement esp_lcd_ssd1681 initialization
- [ ] Configure GPIO pins (see DISPLAY_REFACTOR_PLAN.md)
- [ ] Implement 2-button interface (GPIO43, GPIO44)

### Configuration Files
- [ ] Create `sdkconfig.lilygo_t3`
- [ ] Create `sdkconfig.model_beta`
- [ ] Update `components/bsp/Kconfig`
- [ ] Update `components/bsp/CMakeLists.txt`

### Build System
- [ ] Update main `CMakeLists.txt` for conditional UI
- [ ] Update `Makefile` with `MODEL=beta` target
- [ ] Test build for all models

## 📄 Phase 5: E-Paper Optimization (Week 4)

### Refresh Strategy
- [ ] Implement partial refresh function
- [ ] Implement full refresh function
- [ ] Add refresh counter (0-10)
- [ ] Implement auto-full-refresh every 10 partials
- [ ] Test ghosting prevention

### Performance
- [ ] Optimize LVGL buffer size for E-Paper
- [ ] Minimize unnecessary redraws
- [ ] Test refresh timing (target <1s for status)

## ✅ Phase 6: Testing (Week 4-5)

### Hardware Testing
- [ ] Test LC-Alpha (Heltec V3 OLED, 1 button)
- [ ] Test LC-Alpha+ (Heltec V3 OLED, 2 buttons)
- [ ] Test LC-Beta (T3-S3 E-Paper, 2 buttons)

### Functional Testing
- [ ] All screens render correctly
- [ ] Button navigation works
- [ ] Status bar updates
- [ ] Battery indicator works
- [ ] RSSI indicator works
- [ ] Bluetooth status works
- [ ] USB status works
- [ ] Menu navigation works
- [ ] Settings can be changed
- [ ] OTA updates work
- [ ] BLE pairing works

### Performance Testing
- [ ] Measure RAM usage
- [ ] Measure Flash usage
- [ ] Measure FPS (OLED)
- [ ] Measure refresh time (E-Paper)
- [ ] Check for memory leaks

## 📚 Phase 7: Documentation (Week 5)

### Wokwi Simulation
- [ ] Create `wokwi/lilygo_t3/diagram.json`
- [ ] Create `wokwi/lilygo_t3/wokwi.toml`
- [ ] Create `wokwi/lilygo_t3/README.md`
- [ ] Test simulation

### Documentation
- [ ] Update `README.md` with new architecture
- [ ] Create `MIGRATION_GUIDE.md` (u8g2 → esp_lcd+LVGL)
- [ ] Document LVGL customization
- [ ] Document E-Paper refresh strategy
- [ ] Update component READMEs

### CI/CD
- [ ] Update `.github/workflows/build.yml`
- [ ] Add LC-Beta build job
- [ ] Test all build variants

## 🚀 Phase 8: Release (Week 5-6)

### Code Quality
- [ ] Code review
- [ ] Refactor duplicated code
- [ ] Remove debug logging
- [ ] Add error handling
- [ ] Format code

### Release Preparation
- [ ] Update version numbers
- [ ] Update CHANGELOG.md
- [ ] Create comprehensive PR description
- [ ] Tag release candidate

### Merge
- [ ] Create PR to develop
- [ ] Address review comments
- [ ] Merge to develop
- [ ] Delete feature branch

---

## 🎯 Quick Commands

```bash
# View TODO list
q chat "show todo 1763481816328"

# Mark task complete (example)
q chat "mark task 0 complete in todo 1763481816328"

# Build LC-Alpha (OLED)
make build MODEL=alpha

# Build LC-Beta (E-Paper)
make build MODEL=beta

# Test in Wokwi
make sim-run

# Flash to hardware
make flash-monitor MODEL=beta
```

## 📊 Progress Tracking

- **Phase 1**: ⬜ Not Started
- **Phase 2**: ⬜ Not Started
- **Phase 3**: ⬜ Not Started
- **Phase 4**: ⬜ Not Started
- **Phase 5**: ⬜ Not Started
- **Phase 6**: ⬜ Not Started
- **Phase 7**: ⬜ Not Started
- **Phase 8**: ⬜ Not Started

**Overall Progress**: 0/48 tasks (0%)

---

**Last Updated**: 2025-11-18
