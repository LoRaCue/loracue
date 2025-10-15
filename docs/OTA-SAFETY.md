# OTA Safety and Rollback Protection

This document explains LoRaCue's multi-layered OTA safety mechanisms that prevent bricking devices with bad firmware.

## Safety Layers

### 1. Upload Validation (Immediate)

**Location:** `ota_engine_finish()` → `esp_ota_end()`

**Checks:**
- Magic bytes (0xE9 header)
- SHA256 checksum
- Partition alignment
- Size validation

**Result:** If validation fails, boot partition is **NOT** changed. Device continues running old firmware.

```c
ret = ota_engine_finish();  // Calls esp_ota_end() internally
if (ret == ESP_OK) {
    // Only set boot partition if validation passed
    esp_ota_set_boot_partition(update_partition);
    esp_restart();
}
```

### 2. Boot Counter Protection (First 3 Boots)

**Location:** `main.c` → `app_main()`

**Mechanism:**
- New firmware boots in `ESP_OTA_IMG_PENDING_VERIFY` state
- Boot counter increments on each boot
- If counter reaches `MAX_BOOT_ATTEMPTS` (3), automatic rollback

**Purpose:** Catches firmware that crashes during initialization before 60s timer.

```c
uint32_t boot_counter = ota_get_boot_counter();
if (boot_counter >= MAX_BOOT_ATTEMPTS) {
    ESP_LOGE(TAG, "Max boot attempts reached, forcing rollback");
    esp_ota_mark_app_invalid_rollback_and_reboot();
}
```

### 3. Health Check Timer (60 Seconds)

**Location:** `main.c` → `ota_validation_timer_cb()`

**Mechanism:**
- Timer starts after successful initialization
- After 60 seconds of stable operation, firmware is marked valid
- Boot counter is reset

**Purpose:** Ensures firmware runs stably for a full minute before committing.

```c
static void ota_validation_timer_cb(TimerHandle_t timer)
{
    ESP_LOGI(TAG, "60s health check passed - marking firmware valid");
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running_partition, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            esp_ota_mark_app_valid_cancel_rollback();
            ota_reset_boot_counter();
        }
    }
}
```

### 4. Bootloader Rollback (Hardware Level)

**Location:** ESP32 bootloader (ROM)

**Mechanism:**
- Bootloader reads `otadata` partition
- If app is marked `ESP_OTA_IMG_INVALID`, boots previous partition
- Automatic rollback after 3 failed boot attempts

**Purpose:** Last-resort protection if firmware crashes before app_main().

## Boot State Machine

```
┌─────────────────────────────────────────────────────────────┐
│ OTA Upload Complete                                         │
│ esp_ota_set_boot_partition() called                        │
│ Device reboots                                              │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│ BOOT 1: ESP_OTA_IMG_PENDING_VERIFY                         │
│ - Boot counter = 1                                          │
│ - 60s timer starts                                          │
│ - If crash → BOOT 2                                         │
│ - If 60s pass → ESP_OTA_IMG_VALID                          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│ BOOT 2: ESP_OTA_IMG_PENDING_VERIFY                         │
│ - Boot counter = 2                                          │
│ - If crash → BOOT 3                                         │
│ - If 60s pass → ESP_OTA_IMG_VALID                          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│ BOOT 3: ESP_OTA_IMG_PENDING_VERIFY                         │
│ - Boot counter = 3                                          │
│ - If crash → ROLLBACK (boot counter >= MAX_BOOT_ATTEMPTS)  │
│ - If 60s pass → ESP_OTA_IMG_VALID                          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│ ROLLBACK: esp_ota_mark_app_invalid_rollback_and_reboot()  │
│ - Bootloader boots previous partition                      │
│ - Device returns to last known good firmware                │
└─────────────────────────────────────────────────────────────┘
```

## Debugging Bootloops

If you're experiencing bootloops after OTA, check the serial logs for these indicators:

### 1. Upload Failed (Safe - No Bootloop)
```
E (12345) OTA_ENGINE: OTA validation failed: ESP_ERR_OTA_VALIDATE_FAILED
E (12346) COMMANDS: XMODEM upload failed: ESP_ERR_OTA_VALIDATE_FAILED
```
**Cause:** Corrupted upload or wrong firmware binary  
**Result:** Boot partition NOT changed, device continues running old firmware  
**Action:** Re-upload correct firmware

### 2. Immediate Crash (Rollback After 3 Boots)
```
I (1234) LORACUE_MAIN: Running from partition: ota_0 (0x230000)
I (1235) LORACUE_MAIN: OTA state: PENDING_VERIFY, boot counter: 1
...
Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
...
[Device reboots]
I (1234) LORACUE_MAIN: OTA state: PENDING_VERIFY, boot counter: 2
...
[Crash again]
I (1234) LORACUE_MAIN: OTA state: PENDING_VERIFY, boot counter: 3
E (1235) LORACUE_MAIN: Max boot attempts reached (3), forcing rollback NOW
I (1236) LORACUE_MAIN: Running from partition: ota_1 (0x430000)
```
**Cause:** Firmware has critical bug causing immediate crash  
**Result:** Automatic rollback to previous firmware after 3 attempts  
**Action:** Fix bug and re-upload

### 3. Delayed Crash (Before 60s Timer)
```
I (1234) LORACUE_MAIN: Running from partition: ota_0 (0x230000)
I (1235) LORACUE_MAIN: OTA state: PENDING_VERIFY, boot counter: 1
...
[30 seconds of operation]
...
Guru Meditation Error: Core 0 panic'ed (StoreProhibited)
...
[Device reboots, boot counter increments]
```
**Cause:** Firmware crashes during initialization (before 60s)  
**Result:** Rollback after 3 boot attempts  
**Action:** Fix initialization bug

### 4. Successful Update
```
I (1234) LORACUE_MAIN: Running from partition: ota_0 (0x230000)
I (1235) LORACUE_MAIN: OTA state: PENDING_VERIFY, boot counter: 1
...
[60 seconds pass]
...
I (61234) LORACUE_MAIN: 60s health check passed - marking firmware valid
I (61235) LORACUE_MAIN: OTA state: VALID, boot counter: 0
```
**Result:** Firmware committed, boot counter reset

## Common Causes of Bootloops

### 1. Wrong Target Architecture
```bash
# Building for wrong chip
idf.py set-target esp32    # WRONG for Heltec V3
idf.py set-target esp32s3  # CORRECT for Heltec V3
```

### 2. Corrupted Binary
- Network interruption during upload
- Disk corruption
- Wrong file uploaded (e.g., bootloader.bin instead of app.bin)

### 3. Incompatible Configuration
- Different partition table
- Missing required components
- Incompatible ESP-IDF version

### 4. Critical Bugs
- Null pointer dereference in initialization
- Stack overflow in early boot
- Hardware initialization failure

## Diagnostic Commands

### Check Current Partition
```bash
# Via UART command interface
GET_SYSTEM_INFO
```

### Check OTA State
```bash
# Via serial monitor
esptool.py --port /dev/ttyUSB0 read_flash 0x630000 0x2000 otadata.bin
python -m esp_idf_ota_tools.otadata otadata.bin
```

### Force Rollback (Emergency)
```bash
# Erase otadata to force factory boot
esptool.py --port /dev/ttyUSB0 erase_region 0x630000 0x2000
```

## Best Practices

1. **Always test firmware in simulator first** (`make sim-run`)
2. **Test on development hardware before field deployment**
3. **Monitor serial logs during first boot after OTA**
4. **Keep factory partition as last-resort recovery**
5. **Use BLE OTA for field updates** (can monitor logs on UART simultaneously)
6. **Verify firmware version in BLE device name** after successful update

## Recovery Procedures

### If Device is Bootlooping

1. **Wait for automatic rollback** (3 boot attempts = ~30 seconds)
2. **Check serial logs** to identify crash cause
3. **Fix firmware bug** and re-upload
4. **Test in simulator** before deploying to hardware

### If Rollback Fails (Rare)

1. **Connect via USB**
2. **Erase flash**: `make erase`
3. **Flash factory firmware**: `make flash`
4. **Reconfigure device**

### If Both OTA Partitions are Bad

1. **Boot from factory partition** (always preserved)
2. **Erase otadata**: `esptool.py erase_region 0x630000 0x2000`
3. **Device boots factory firmware**
4. **Upload working firmware via OTA**

## Partition Layout

```
0x030000 - 0x22FFFF: factory  (2MB) - Factory firmware (never overwritten)
0x230000 - 0x42FFFF: ota_0    (2MB) - OTA partition 0
0x430000 - 0x62FFFF: ota_1    (2MB) - OTA partition 1
0x630000 - 0x631FFF: otadata  (8KB) - OTA state tracking
```

**Factory partition** is your safety net - it's never touched by OTA updates.

## Summary

LoRaCue's OTA system is designed to be **brick-proof**:

1. ✅ **Upload validation** prevents bad binaries from being set as boot partition
2. ✅ **Boot counter** catches crashes during initialization
3. ✅ **60s health check** ensures stable operation before committing
4. ✅ **Automatic rollback** recovers from bad firmware
5. ✅ **Factory partition** provides last-resort recovery

**You should NEVER experience permanent bootloops.** If you do, it indicates:
- Hardware failure
- Corrupted flash memory
- All three partitions (factory + ota_0 + ota_1) are bad (extremely unlikely)

In such cases, use `make erase && make flash` to recover.
