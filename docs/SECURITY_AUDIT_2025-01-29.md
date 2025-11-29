# LoRaCue Security & Code Quality Audit Report

**Date:** 2025-01-29  
**Auditor:** Amazon Q Developer  
**Scope:** Security vulnerabilities, performance bottlenecks, hardcoded values, missing enums

---

## Executive Summary

Comprehensive audit of LoRaCue codebase identified **4 critical security issues**, **8 code quality improvements**, and **3 performance optimizations**. The codebase demonstrates strong architectural patterns (event-driven, mutex-protected state, Ed25519 signatures) but requires immediate attention to production security logging and input validation.

**Architecture Validation:** ✅ Confirmed `commands.c` (JSON-RPC adapter) and `commands_api.c` (core business logic) separation is correct and follows documented architecture.

---

## 🚨 Critical Security Vulnerabilities

### 1. AES Key Logging in Production Code
**Severity:** CRITICAL  
**Location:** `components/lora/lora_protocol.c:295-303`  
**CWE:** CWE-532 (Insertion of Sensitive Information into Log File)

```c
ESP_LOGI(TAG, "AES Key: %02X%02X%02X%02X...", sender_device.aes_key[0], ...);
```

**Risk:** Full AES-256 encryption keys logged to serial output. Anyone with serial/UART access can capture all device encryption keys, completely compromising the security model.

**Recommendation:**
- Remove entirely from production builds
- If needed for debugging, use `ESP_LOGD` (debug-only) with `#ifdef DEBUG_CRYPTO`
- Never log cryptographic material at INFO level

---

### 2. Unsafe sprintf() Usage
**Severity:** HIGH  
**Locations:**
- `components/ota_engine/ota_engine.c:258`
- `components/commands/commands.c:295`

```c
sprintf(&calculated_hex[i * 2], "%02x", calculated_hash[i]);
sprintf(&hex_key[i * 2], "%02x", config.aes_key[i]);
```

**Risk:** `sprintf()` has no bounds checking. While current usage appears safe (fixed-size loops), it's a dangerous pattern that could lead to buffer overflows during maintenance.

**Recommendation:** Replace with `snprintf()`:
```c
snprintf(&calculated_hex[i * 2], 3, "%02x", calculated_hash[i]);
```

---

### 3. Missing Firmware Size Validation
**Severity:** HIGH  
**Location:** `components/ota_engine/ota_engine.c:48`

```c
if (firmware_size == 0 || firmware_size > 4 * 1024 * 1024) {
```

**Risk:** Hardcoded 4MB limit checked before partition size validation. Could accept firmware larger than available OTA partition space.

**Recommendation:**
```c
if (firmware_size == 0) return ESP_ERR_INVALID_SIZE;
if (firmware_size > ota_partition->size) {
    ESP_LOGE(TAG, "Firmware too large: %zu > %zu", firmware_size, ota_partition->size);
    return ESP_ERR_INVALID_SIZE;
}
```

---

### 4. Replay Attack Window Too Large
**Severity:** MEDIUM  
**Location:** `components/lora/lora_protocol.c:380-410`

```c
} else if (seq_diff > -64) {
    // Within recent window (out-of-order packet)
```

**Risk:** 64-packet sliding window allows replay attacks within this range. An attacker could capture and replay packets.

**Recommendation:**
- Reduce window to 16-32 packets
- Add timestamp-based validation (reject packets >5 seconds old)
- Document security trade-offs in code comments

---

## ⚠️ Hardcoded Values Requiring Constants

### 5. Power Management Timeouts
**Severity:** MEDIUM (Code Quality)  
**Location:** `components/power_mgmt/power_mgmt.c:36-38`

```c
.display_sleep_timeout_ms  = 10000,   // ✗ Hardcoded
.light_sleep_timeout_ms    = 30000,   // ✗ Hardcoded
.deep_sleep_timeout_ms     = 300000,  // ✗ Hardcoded
```

**Recommendation:**
```c
#define POWER_MGMT_DEFAULT_DISPLAY_SLEEP_MS  10000   // 10 seconds
#define POWER_MGMT_DEFAULT_LIGHT_SLEEP_MS    30000   // 30 seconds
#define POWER_MGMT_DEFAULT_DEEP_SLEEP_MS     300000  // 5 minutes
```

---

### 6. LoRa Protocol Magic Numbers
**Severity:** MEDIUM (Code Quality)  
**Locations:**
- `components/lora/lora_protocol.c:465` - Connection timeout
- `components/lora/lora_protocol.c:470-476` - RSSI thresholds
- `components/lora/lora_driver.c:189, 396` - Sync word

```c
if (time_since_last_packet > 30000000) {  // ✗ Magic number
if (last_rssi > -70) {                    // ✗ Magic number
SetSyncWord(0x1424);                      // ✗ Magic number
```

**Recommendation:**
```c
#define LORA_CONNECTION_TIMEOUT_US      30000000  // 30 seconds
#define LORA_RSSI_EXCELLENT_THRESHOLD   -70       // dBm
#define LORA_RSSI_GOOD_THRESHOLD        -85       // dBm
#define LORA_RSSI_WEAK_THRESHOLD        -100      // dBm
#define LORA_PRIVATE_SYNC_WORD          0x1424    // Private network ID
```

---

### 7. OTA Timeout Constant
**Severity:** LOW (Code Quality)  
**Location:** `components/ota_engine/ota_engine.c:27`

```c
#define OTA_TIMEOUT_MS 30000  // ✓ Good - but should be configurable
```

**Recommendation:** Already defined, but consider making it runtime-configurable for slow networks.

---

## 📊 Missing Enums for Code Clarity

### 8. CPU Frequency Values
**Severity:** LOW (Code Quality)  
**Location:** `components/power_mgmt/power_mgmt.c:283`

```c
if (freq_mhz != 80 && freq_mhz != 160 && freq_mhz != 240) {
```

**Recommendation:**
```c
typedef enum {
    CPU_FREQ_80MHZ  = 80,
    CPU_FREQ_160MHZ = 160,
    CPU_FREQ_240MHZ = 240
} cpu_freq_t;
```

---

### 9. LoRa Bandwidth Values
**Severity:** LOW (Code Quality)  
**Location:** `components/lora/lora_driver.c:160-170` (duplicated at 367-377)

```c
switch (bandwidth) {
    case 7:   bw_reg = 0x00; break;  // 7.8 kHz
    case 10:  bw_reg = 0x08; break;  // 10.4 kHz
    // ... 8 more cases
```

**Recommendation:**
```c
typedef enum {
    LORA_BW_7_8KHZ   = 7,
    LORA_BW_10_4KHZ  = 10,
    LORA_BW_15_6KHZ  = 15,
    LORA_BW_20_8KHZ  = 20,
    LORA_BW_31_25KHZ = 31,
    LORA_BW_41_7KHZ  = 41,
    LORA_BW_62_5KHZ  = 62,
    LORA_BW_125KHZ   = 125,
    LORA_BW_250KHZ   = 250,
    LORA_BW_500KHZ   = 500
} lora_bandwidth_t;
```

---

### 10. Wake GPIO Pin in Power Management
**Severity:** LOW (Code Quality)  
**Location:** `components/power_mgmt/power_mgmt.c:44`

```c
#define WAKE_GPIO_BUTTON 0 // User button on Heltec V3
```

**Issue:** Hardcoded GPIO in power_mgmt violates BSP abstraction layer.

**Recommendation:** Move to BSP header:
```c
// In bsp.h
#define BSP_GPIO_BUTTON_WAKE  BSP_GPIO_BUTTON_NEXT
```

---

## 🐛 Performance Bottlenecks

### 11. Duplicate Bandwidth Validation Code
**Severity:** LOW (Performance/Maintainability)  
**Location:** `lora_driver.c:160-170` and `367-377`

**Issue:** Identical switch statement duplicated in two functions.

**Recommendation:** Extract to helper function:
```c
static uint8_t lora_bandwidth_to_register(uint16_t bandwidth) {
    switch (bandwidth) {
        case 7:   return 0x00;
        case 10:  return 0x08;
        case 15:  return 0x01;
        case 20:  return 0x09;
        case 31:  return 0x02;
        case 41:  return 0x0A;
        case 62:  return 0x03;
        case 125: return 0x04;
        case 250: return 0x05;
        case 500: return 0x06;
        default:  return 0x04;  // 125 kHz default
    }
}
```

---

### 12. Inefficient RSSI Monitoring
**Severity:** LOW (Performance/Power)  
**Location:** `components/lora/lora_protocol.c:501`

```c
vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
```

**Issue:** Dedicated task wakes every 5 seconds just to check RSSI. Wastes power and CPU cycles.

**Recommendation:** Use event-driven approach:
- Update RSSI on packet reception (already done)
- Remove polling task entirely
- Trigger state callbacks only on actual state changes

---

### 13. Task Stack Size Inconsistencies
**Severity:** LOW (Resource Usage)  
**Locations:** Multiple task creations

```c
xTaskCreate(lora_tx_task, "lora_tx", 3072, ...);           // 3KB
xTaskCreate(lora_rx_task, "lora_rx", 3072, ...);           // 3KB
xTaskCreate(protocol_rx_task, "protocol_rx", 3072, ...);   // 3KB
xTaskCreate(button_manager_task, "button_mgr", 4096, ...); // 4KB (increased due to overflow)
xTaskCreate(rssi_monitor_task, "lora_rssi", 3072, ...);    // 3KB
```

**Issue:** Inconsistent stack sizes, one task already had overflow. No systematic sizing strategy.

**Recommendation:** Define named constants:
```c
#define TASK_STACK_SIZE_SMALL   2048  // Simple state machines
#define TASK_STACK_SIZE_MEDIUM  3072  // Network/protocol tasks
#define TASK_STACK_SIZE_LARGE   4096  // UI/complex logic
```

---

## 🔧 Additional Code Quality Issues

### 14. Missing Null Pointer Checks
**Severity:** MEDIUM  
**Locations:**
- `components/commands/commands_api.c:126`
- `components/device_registry/device_registry.c:126-127`

```c
memcpy(config.aes_key, key, 32);  // No null check on 'key'
memcpy(device.mac_address, mac_address, DEVICE_MAC_ADDR_LEN);
```

**Recommendation:** Add validation at function entry:
```c
if (!key || !mac_address || !aes_key) {
    return ESP_ERR_INVALID_ARG;
}
```

---

### 15. Inconsistent Error Handling
**Severity:** LOW  
**Location:** Throughout codebase

**Issue:** Some functions return `ESP_ERR_INVALID_ARG`, others `ESP_FAIL`, some don't validate.

**Recommendation:** Document error code conventions:
- `ESP_ERR_INVALID_ARG` - Invalid parameters
- `ESP_ERR_INVALID_STATE` - Wrong state for operation
- `ESP_ERR_NOT_FOUND` - Resource not found
- `ESP_ERR_TIMEOUT` - Operation timeout
- `ESP_FAIL` - Generic failure (avoid when possible)

---

## ✅ Positive Findings

1. **Proper `strncpy()` usage** with size limits in most places
2. **Mutex protection** for all shared state
3. **Event-driven architecture** reduces coupling
4. **Sliding window replay protection** is enterprise-grade (just needs smaller window)
5. **Ed25519 signature verification** for OTA is excellent security practice
6. **Consistent button timing constants** are well-defined
7. **Correct separation of concerns** between `commands.c` (JSON-RPC adapter) and `commands_api.c` (core logic)

---

## 📋 Priority Recommendations

### Immediate (Security Critical)
1. ✅ Remove AES key logging from production code
2. ✅ Replace `sprintf()` with `snprintf()`
3. ✅ Validate firmware size against actual partition size
4. ✅ Add null pointer checks to all public APIs

### Short-term (Code Quality)
5. ✅ Extract hardcoded timing values to named constants
6. ✅ Create enums for CPU frequencies and LoRa bandwidths
7. ✅ Reduce replay attack window to 16-32 packets
8. ✅ Eliminate duplicate bandwidth validation code

### Long-term (Architecture)
9. ✅ Move wake GPIO to BSP abstraction
10. ✅ Replace RSSI polling with event-driven updates
11. ✅ Standardize task stack sizes with named constants
12. ✅ Document error code conventions in developer guide

---

## Conclusion

The LoRaCue codebase demonstrates strong architectural foundations with proper separation of concerns, thread-safe state management, and enterprise-grade security features (Ed25519 signatures, AES-256 encryption, replay protection). However, **critical security logging issues must be addressed immediately** before any production deployment.

The identified issues are typical of embedded systems development and can be systematically resolved without major architectural changes. The codebase is well-positioned for production hardening.

**Overall Security Rating:** B+ (would be A- after addressing critical logging issue)  
**Code Quality Rating:** B  
**Architecture Rating:** A

---

**Next Steps:**
1. Create GitHub issues for each finding
2. Prioritize security fixes for next sprint
3. Establish code review checklist based on this audit
4. Schedule follow-up audit after fixes
