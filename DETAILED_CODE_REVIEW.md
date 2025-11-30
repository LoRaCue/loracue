# Detailed Code Review & Optimization List

This document contains a systematic, file-by-file code review of the project components.

## 1. Component: `ble`

### `ble.c`
- [ ] **Refactor**: `gap_event_handler` duplicates OTA connection handle setting (`ble_ota_set_connection_handle` and `ble_ota_handler_set_connection`). Unify if possible.
- [ ] **Optimization**: `ble_send_response` manually chunks data. Extract this logic to a helper function `ble_send_long_notification` for clarity ("vor die Klammer").
- [ ] **Optimization**: `get_release_type` and `get_build_number` parse strings at runtime. Initialize these values once in `ble_init` or use compile-time constants.
- [ ] **Hardcoded**: `BLE_CMD_QUEUE_SIZE`, `BLE_CMD_TASK_PRIORITY`, `BLE_MTU_MAX` are defined locally. Move to `include/ble_config.h` or similar if reused.

### `ble_ota_handler.c`
- [ ] **Bug/Risk**: `ota_task` deletes itself on error but `s_ota_task_handle` is static and might not be reset to NULL. This prevents restarting OTA without reboot. **Fix:** Set `s_ota_task_handle = NULL` before `vTaskDelete`.
- [ ] **Hardcoded**: `OTA_TASK_SIZE` (8192) and `OTA_RINGBUF_SIZE` (4096) are hardcoded. Move to `ble.h` or `task_config.h`.

## 2. Component: `lora`

### `lora_driver.c`
- [ ] **Optimization**: `lora_rx_task` polls `sx126x_receive` every 5ms. Convert to interrupt-driven architecture (using DIO1 ISR + Semaphore) to allow CPU sleep.
- [ ] **Refactor**: `lora_bandwidth_to_register` uses a switch statement. Convert to a static lookup table `struct { uint16_t bw; uint8_t reg; }` for cleaner code.

### `lora_protocol.c`
- [ ] **Performance**: `lora_protocol_receive_packet` initializes and frees `mbedtls_aes_context` for *every* packet. For a presentation remote (high traffic), this is overhead. Consider a persistent context or a pool.
- [ ] **Refactor**: The sliding window replay protection logic (bitmap shifting) is complex and inline. Extract to `static bool check_replay_window(...)` to simplify the main RX flow.
- [ ] **Refactor**: `lora_protocol_get_connection_state` logic is slightly redundant with RSSI thresholds. Ensure thresholds match UI display logic (or use shared constants).

## 3. Component: `power_mgmt`

### `power_mgmt.c`
- [ ] **Refactor**: `POWER_MGMT_DEFAULT_*` constants are hardcoded. Move to Kconfig or a header.
- [ ] **Optimization**: `power_mgmt_get_stats` uses float math. Verify if integer estimation is sufficient to save flash/cycles (though ESP32-S3 has FPU, so low priority).

## 4. Component: `ota_engine`

### `ota_engine.c`
- [ ] **Refactor**: `ota_engine_set_expected_sha256` and `ota_engine_verify_signature` contain duplicate hex string validation logic. Extract to `static bool validate_hex_string(const char *hex, size_t len)`.
- [ ] **Hardcoded**: `OTA_TIMEOUT_MS` (30000) is hardcoded.

## 5. Component: `ui_compact`

### `ui_compact.c`
- [ ] **Performance**: `ui_compact_get_status` calls `bsp_read_battery` which performs blocking ADC reads (multiple samples). If the status bar updates frequently (e.g. LVGL timer), this will cause UI stutter or needless CPU wake. **Fix:** Cache the battery value in a separate lower-priority task or timer (e.g., update every 30s) and read from cache in UI.