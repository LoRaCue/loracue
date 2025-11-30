# LoRaCue Code Review & Optimization To-Do List

This document contains a systematic review of the project components, focusing on optimizations, coding standards, documentation, and refactoring opportunities.

**Date:** 2025-11-30
**Reviewer:** Gemini CLI Agent

---

## 1. Core System (`common_types`, `system_events`, `commands`, `device_registry`)

### Findings
- **`common_types/include/common_types.h`**:
  - `command_history_entry_t` struct members have `__attribute__((unused))`. This is incorrect for a type definition.
- **`system_events/system_events.c`**:
  - Hardcoded values for task configuration (`task_priority = 10`, `task_stack_size = 4096`, `queue_size = 32`).
  - Should utilize `task_config.h`.
- **`commands/commands_api.c`**:
  - `cmd_set_lora_config` has hardcoded bandwidth values (7, 10, 15...).
  - `ui_data_provider_reload_config` is defined as a weak symbol; consider an event-based approach.
- **`device_registry/device_registry.c`**:
  - `device_registry_update_sequence` updates sequence numbers in RAM only. Replay protection state is lost on reboot, which is a security trade-off that should be documented.

### Action Items
- [ ] **Refactor**: Remove `__attribute__((unused))` from `command_history_entry_t` members in `common_types.h`.
- [ ] **Refactor**: Use `task_config.h` constants in `system_events.c`. Define `SYSTEM_EVENT_QUEUE_SIZE`.
- [ ] **Refactor**: Define constants or a validation helper for LoRa bandwidths in `commands_api.c`.
- [ ] **Documentation**: Add security note to `device_registry.c` regarding replay protection persistence.

---

## 2. Hardware Abstraction (`bsp`, `i2console`)

### Findings
- **`bsp/bsp_i2c.c`**:
  - Hardcodes `I2C_NUM_0`. Prevents using a second I2C bus.
- **`bsp/bsp_heltec_v3.c`**:
  - Hardcodes `SPI2_HOST`.
- **`i2console/i2console.c`**:
  - `i2console_vprintf` allocates 256 bytes on the stack. In tight tasks, this could cause overflow.

### Action Items
- [ ] **Refactor**: Update `bsp_i2c_init` to accept an I2C port number or config struct.
- [ ] **Refactor**: Make SPI host configurable in `bsp_heltec_v3.c` (or via Kconfig).
- [ ] **Optimization**: Verify stack usage of tasks using `i2console`. Consider reducing buffer size or using a static/heap buffer if safe.

---

## 3. Drivers (`bq25896`, `led_manager`, etc.)

### Findings
- **`bq25896/bq25896.c`**:
  - **CRITICAL**: Uses legacy I2C API (`i2c_master_write_read_device`) which is incompatible with the new driver (`i2c_new_master_bus`) used by BSP. This will likely fail to link or work if drivers are mixed.
- **`led_manager/led_manager.c`**:
  - Hardcodes GPIO 35. This violates BSP abstraction.

### Action Items
- [ ] **Fix**: Rewrite `bq25896.c` (and other I2C drivers) to use the ESP-IDF v5 I2C master driver API (`i2c_master_transmit`), receiving `i2c_master_dev_handle_t` from BSP.
- [ ] **Refactor**: Retrieve LED GPIO pin from BSP (e.g., `bsp_get_led_gpio()`) instead of hardcoding in `led_manager.c`.

---

## 4. Connectivity (`lora`, `sx126x`, `ble`)

### Findings
- **`sx126x/sx126x.c`**:
  - `ReadBuffer` and `WriteBuffer` use `malloc`/`free` for small buffers (~260 bytes). This causes heap fragmentation and overhead in high-frequency paths.
- **`lora/lora_driver.c`**:
  - `lora_rx_task` polls `sx126x_receive` every 5ms. This prevents effective deep sleep/power saving. Interrupt-driven RX is preferred.
- **`ble/ble.c`**:
  - `BLE_CMD_TASK_STACK_SIZE` is 6144 bytes. This seems excessive for command processing and could be optimized.

### Action Items
- [ ] **Optimization**: Use stack buffers (e.g., `uint8_t buf[260]`) in `sx126x.c` instead of dynamic allocation.
- [ ] **Optimization**: Investigate using a semaphore/notification for `lora_rx_task` triggered by the SX126x DIO1 interrupt.
- [ ] **Optimization**: Tune `BLE_CMD_TASK_STACK_SIZE`.

---

## 5. UI & Logic (`display`, `ui_*`, `presenter_mode_manager`)

### Findings
- **`display/display.c`**:
  - `display_lvgl_flush_cb` appears unused if `esp_lvgl_port` is used (which provides its own flush).
- **`presenter_mode_manager/presenter_mode_manager.c`**:
  - Uses magic numbers (0x4F, 0x50) for HID keycodes.

### Action Items
- [ ] **Cleanup**: Remove unused `display_lvgl_flush_cb` in `display.c` if confirmed dead code.
- [ ] **Refactor**: Define and use `HID_KEY_ARROW_RIGHT` / `HID_KEY_ARROW_LEFT` constants in `presenter_mode_manager.c`.
