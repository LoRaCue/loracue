# BLE OTA Firmware Update

This document describes how to perform Over-The-Air (OTA) firmware updates via Bluetooth Low Energy (BLE) on LoRaCue devices.

## Overview

LoRaCue provides a dedicated BLE GATT service for firmware updates, separate from the Nordic UART Service used for commands. This ensures reliable, flow-controlled firmware transfers with progress monitoring.

## Service UUIDs

**OTA Service:** `49589A79-7CC5-465D-BFF1-FE37C5065000`

**Characteristics:**
- **Control:** `49589A79-7CC5-465D-BFF1-FE37C5065001` (Write + Indicate)
- **Data:** `49589A79-7CC5-465D-BFF1-FE37C5065002` (Write without response)
- **Progress:** `49589A79-7CC5-465D-BFF1-FE37C5065003` (Read + Notify)

## Protocol

### Control Commands

Write to Control characteristic:

| Command | Byte 0 | Bytes 1-4 | Description |
|---------|--------|-----------|-------------|
| START   | `0x01` | Size (big-endian) | Start OTA with firmware size |
| ABORT   | `0x02` | - | Abort current OTA |
| FINISH  | `0x03` | - | Finalize and reboot |

### Control Responses

Received via Control characteristic indications:

| Response | Byte 0 | Bytes 1+ | Description |
|----------|--------|----------|-------------|
| READY    | `0x10` | - | Ready for data |
| ERROR    | `0x11` | Error message (optional) | Error occurred |
| COMPLETE | `0x12` | - | Upload complete, rebooting |

### Progress Notifications

Received via Progress characteristic notifications:
- Single byte: 0-100 (percentage)
- Sent every 5% progress

## Update Procedure

### 1. Connect to Device

Scan for BLE devices with name pattern: `LoRaCue-XXXXXX-vX.X.X`

### 2. Pair with Device (REQUIRED)

**OTA characteristics require encrypted connection.**

- Initiate BLE pairing
- Device displays 6-digit passkey on OLED
- Enter passkey on client device
- Complete bonding process
- Keys stored for future connections

**Without pairing:** BLE stack returns "Insufficient Authentication" error when accessing OTA characteristics.

### 3. Discover Services

Discover the OTA service (`49589A79-7CC5-465D-BFF1-FE37C5065000`) and its characteristics.

### 4. Enable Notifications

Enable indications on Control characteristic and notifications on Progress characteristic.

### 5. Send START Command

```
Byte 0: 0x01 (START)
Byte 1-4: Firmware size in bytes (big-endian)
```

Example for 1,598,432 bytes (0x186680):
```
01 00 18 66 80
```

Wait for READY response (`0x10`).

### 6. Stream Firmware Data

Write firmware binary to Data characteristic in chunks:
- Recommended chunk size: 512 bytes
- Use "Write without response" for maximum throughput
- Monitor Progress notifications to track upload

### 7. Send FINISH Command

```
Byte 0: 0x03 (FINISH)
```

Wait for COMPLETE response (`0x12`). Device will validate firmware and reboot.

### 8. Reconnect

After reboot (~5 seconds), reconnect to verify new firmware version in device name.

## Error Handling

If ERROR response (`0x11`) is received:
- Read optional error message from bytes 1+
- Send ABORT command (`0x02`)
- Retry from step 4

Common errors:
- Invalid firmware size
- OTA engine failure
- Write errors during upload
- Validation failure

## Rollback Protection

LoRaCue uses ESP32's OTA rollback mechanism:
- New firmware boots in "pending verification" state
- After 60 seconds of successful operation, firmware is marked valid
- If device crashes or fails to boot, bootloader automatically rolls back to previous firmware
- Maximum 3 boot attempts before rollback

## Python Example

```python
import asyncio
from bleak import BleakClient

OTA_SERVICE = "49589A79-7CC5-465D-BFF1-FE37C5065000"
OTA_CONTROL = "49589A79-7CC5-465D-BFF1-FE37C5065001"
OTA_DATA = "49589A79-7CC5-465D-BFF1-FE37C5065002"
OTA_PROGRESS = "49589A79-7CC5-465D-BFF1-FE37C5065003"

async def ota_update(address, firmware_path):
    async with BleakClient(address) as client:
        # Read firmware
        with open(firmware_path, 'rb') as f:
            firmware = f.read()
        
        print(f"Firmware size: {len(firmware)} bytes")
        
        # Enable notifications
        def control_handler(sender, data):
            if data[0] == 0x10:
                print("Device ready")
            elif data[0] == 0x11:
                msg = data[1:].decode('utf-8', errors='ignore')
                print(f"Error: {msg}")
            elif data[0] == 0x12:
                print("Upload complete, device rebooting")
        
        def progress_handler(sender, data):
            print(f"Progress: {data[0]}%")
        
        await client.start_notify(OTA_CONTROL, control_handler)
        await client.start_notify(OTA_PROGRESS, progress_handler)
        
        # Send START command
        size_bytes = len(firmware).to_bytes(4, 'big')
        await client.write_gatt_char(OTA_CONTROL, bytes([0x01]) + size_bytes)
        await asyncio.sleep(1)
        
        # Stream firmware
        chunk_size = 512
        for i in range(0, len(firmware), chunk_size):
            chunk = firmware[i:i+chunk_size]
            await client.write_gatt_char(OTA_DATA, chunk, response=False)
        
        print("Upload complete, sending FINISH command")
        
        # Send FINISH command
        await client.write_gatt_char(OTA_CONTROL, bytes([0x03]))
        await asyncio.sleep(2)

# Usage
asyncio.run(ota_update("AA:BB:CC:DD:EE:FF", "firmware.bin"))
```

## Notes

- **MTU Size:** Default MTU is 23 bytes (20 bytes payload). Negotiate larger MTU (up to 500 bytes) for faster transfers.
- **Transfer Speed:** ~10-20 KB/s typical, ~40-50 KB/s with large MTU
- **Battery:** Ensure device has sufficient battery (>20%) before starting OTA
- **Range:** Stay within 5 meters during update for reliable connection
- **Interruption:** If connection drops, restart from step 1. Incomplete uploads are automatically discarded.

## Comparison with UART OTA

| Feature | BLE OTA | UART OTA |
|---------|---------|----------|
| Speed | 10-50 KB/s | 100-200 KB/s |
| Range | 5-10 meters | Cable required |
| Progress | Real-time notifications | No feedback |
| Debugging | Can monitor logs on UART | UART busy during transfer |
| Use Case | Field updates | Development/factory |

## Security

- BLE pairing required before OTA access
- Firmware signature validation (if enabled)
- Rollback protection prevents bricking
- AES-256 encryption over BLE connection

## Enterprise Features

### State Machine
- **IDLE**: Ready to accept new OTA
- **ACTIVE**: Upload in progress
- **FINISHING**: Validating and setting boot partition
- Prevents concurrent OTA sessions

### Timeout Protection
- 30-second timeout without data
- Automatic abort on timeout
- Timer resets on each data packet
- Prevents hung OTA sessions

### Connection Tracking
- Tracks active BLE connection ID
- Automatic abort on disconnect
- Prevents orphaned OTA sessions
- Clean state recovery

### Error Handling
- Detailed error messages (up to 127 chars)
- Automatic cleanup on failures
- State machine prevents invalid operations
- Comprehensive logging for debugging

### Robustness
- Connection drop detection
- Automatic resource cleanup
- No memory leaks on abort
- Safe concurrent command handling
