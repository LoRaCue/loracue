# BLE Not Discoverable - Debug Guide

## Check These Logs

Run `make monitor` and look for these messages:

### 1. BLE Initialization
```
I (xxx) MAIN: Initializing Bluetooth configuration...
I (xxx) ble_config: Initializing NimBLE BLE stack
I (xxx) ble_config: NimBLE initialized successfully
```

**If missing:** BLE init failed, check error message

### 2. NimBLE Host Task
```
I (xxx) ble_config: NimBLE host task started
```

**If missing:** Host task not created

### 3. NimBLE Sync
```
I (xxx) ble_config: NimBLE host synced
I (xxx) ble_config: Device address: xx:xx:xx:xx:xx:xx
```

**If missing:** NimBLE stack not syncing

### 4. Advertising Started
```
I (xxx) ble_config: BLE OTA handler initialized
I (xxx) ble_config: Advertising started with NUS service UUID
```

**If missing:** Advertising not starting

---

## Common Issues

### Issue 1: BLE Not Enabled in Config
```bash
# Check if BLE is enabled
grep CONFIG_BT_ENABLED sdkconfig

# Should show:
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
```

**Fix:** Run `make menuconfig` → Component config → Bluetooth → Enable

### Issue 2: Not Enough Memory
```
E (xxx) ble_config: Failed to init nimble: 257
```

**Fix:** Increase heap size in menuconfig

### Issue 3: Advertising Fails
```
E (xxx) ble_config: Failed to start advertising: X
```

**Error codes:**
- `3` (BLE_HS_EALREADY): Already advertising
- `8` (BLE_HS_EINVAL): Invalid parameters
- `12` (BLE_HS_EBUSY): Stack busy

**Fix:** Check if `on_sync()` is called

### Issue 4: Services Not Registered
```
E (xxx) ble_config: Failed to add GATT services: X
```

**Fix:** Check GATT service definitions

---

## Manual Test

### Using nRF Connect (Android/iOS)
1. Open nRF Connect app
2. Tap "SCAN"
3. Look for "LoRaCue" in device list
4. Should show service UUID: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`

### Using bluetoothctl (Linux)
```bash
bluetoothctl
scan on
# Should see: [NEW] Device XX:XX:XX:XX:XX:XX LoRaCue
```

### Using bleak (Python)
```python
import asyncio
from bleak import BleakScanner

async def scan():
    devices = await BleakScanner.discover()
    for d in devices:
        if "LoRaCue" in d.name:
            print(f"Found: {d.name} - {d.address}")
            print(f"Services: {d.metadata}")

asyncio.run(scan())
```

---

## Quick Fixes

### Force Restart Advertising
Add this command to test:
```c
// In commands.c
if (strcmp(command_line, "BLE_RESTART") == 0) {
    ble_gap_adv_stop();
    vTaskDelay(pdMS_TO_TICKS(100));
    ble_advertise();
    s_send_response("OK BLE restarted");
    return;
}
```

### Check BLE Status
Add this command:
```c
if (strcmp(command_line, "BLE_STATUS") == 0) {
    char buf[128];
    snprintf(buf, sizeof(buf), 
             "BLE: init=%d enabled=%d connected=%d",
             s_ble_initialized, s_ble_enabled, s_conn_state.connected);
    s_send_response(buf);
    return;
}
```

---

## What to Check Now

1. **Flash new firmware** with advertising fix
2. **Run `make monitor`** and check logs
3. **Look for** "Advertising started with NUS service UUID"
4. **Scan with phone** using nRF Connect
5. **Report back** which log message is missing

---

## Expected Full Log Sequence

```
I (1234) MAIN: Initializing Bluetooth configuration...
I (1235) ble_config: Initializing NimBLE BLE stack
I (1240) ble_config: NimBLE initialized successfully
I (1245) ble_config: NimBLE host task started
I (1250) NimBLE: GAP procedure initiated: stop advertising.
I (1255) NimBLE: GAP procedure initiated: advertise; 
I (1260) ble_config: NimBLE host synced
I (1265) ble_config: Device address: aa:bb:cc:dd:ee:ff
I (1270) ble_config: BLE OTA handler initialized
I (1275) ble_config: Advertising started with NUS service UUID
```

If you see all these, BLE is working and device should be discoverable.
