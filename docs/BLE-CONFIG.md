# BLE Configuration Interface

This document describes how to use the Bluetooth Low Energy (BLE) Nordic UART Service to access LoRaCue's command parser for device configuration and management.

## Overview

LoRaCue provides a **Nordic UART Service (NUS)** over BLE that acts as a wireless serial interface to the command parser. This allows you to:

- Configure device settings (name, mode, LoRa parameters)
- Manage paired devices (add, update, remove)
- Query device status and information
- Access all commands available via USB-UART

**Note:** Firmware upgrades use a separate dedicated OTA service. See [BLE-OTA.md](BLE-OTA.md) for details.

## Service UUIDs

**Nordic UART Service:** `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`

**Characteristics:**
- **TX (Notify):** `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` - Device → Client (responses)
- **RX (Write):** `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` - Client → Device (commands)

## Connection Procedure

### 1. Scan for Device

Look for BLE devices with name pattern: `LoRaCue-XXXXXX-vX.X.X`

Example: `LoRaCue-LC4952-v0.1.0-alpha.323`

- `LC4952` = Device name (configurable)
- `v0.1.0-alpha.323` = Firmware version

### 2. Connect to Device

Initiate BLE connection to the device.

### 3. Pair with Device (Optional but Recommended)

**Pairing is OPTIONAL for UART command access** but provides:
- Encrypted communication
- Persistent bonding (no re-pairing needed)
- **Required for OTA firmware updates** (OTA characteristics enforce encryption)

**Pairing Process:**
1. Initiate BLE pairing from your client
2. Device generates a 6-digit passkey
3. Passkey is displayed on device OLED screen
4. Enter passkey on client device
5. Pairing completes and keys are stored in NVS

**Security Configuration:**
- Authentication: `ESP_LE_AUTH_REQ_SC_MITM_BOND` (Secure Connections with MITM protection)
- IO Capability: `ESP_IO_CAP_OUT` (Display only - device shows passkey)
- Key Size: 16 bytes (128-bit)
- Bonding: Enabled (keys stored in NVS flash via `CONFIG_BT_BLE_SMP_BOND_NVS_FLASH=y`)

**Important Notes:**
- **Same pairing for both services**: UART and OTA use the same BLE pairing/bonding
- **Persistent across reboots**: Bonding keys stored in NVS, no re-pairing needed
- **UART characteristics**: No encryption required (`ESP_GATT_PERM_READ/WRITE`)
- **OTA characteristics**: Encryption required (`ESP_GATT_PERM_*_ENCRYPTED`)

### 4. Discover Services

Discover the Nordic UART Service (`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`) and its characteristics.

### 5. Enable Notifications

Enable notifications on the TX characteristic (`6E400003`) to receive command responses.

## Command Protocol

### Sending Commands

Write text commands to the **RX characteristic** (`6E400002`):

- Commands are **newline-terminated** (`\n` or `\r`)
- Maximum command length: **16,384 bytes** (for large JSON payloads)
- Commands can be sent in multiple BLE packets
- Device buffers incoming data until newline is received

**Example:**
```
GET_DEVICE_INFO\n
```

### Receiving Responses

Responses are sent via **TX characteristic notifications** (`6E400003`):

- Each response ends with a newline (`\n`)
- Responses may span multiple BLE packets
- Format: `OK <data>` or `ERROR <message>`

**Example:**
```
OK {"device_id":"0x4952","mac":"24:DC:C3:46:49:52","version":"0.1.0-alpha.323"}\n
```

## Available Commands

### Query Commands (No Parameters)

| Command | Description | Response Format |
|---------|-------------|-----------------|
| `PING` | Test connectivity | `OK PONG` |
| `GET_DEVICE_INFO` | Device ID, MAC, version | `OK {"device_id":"0xXXXX","mac":"...","version":"..."}` |
| `GET_GENERAL` | General configuration | `OK {"device_name":"...","device_mode":"...","slot_id":...}` |
| `GET_POWER_MANAGEMENT` | Power settings | `OK {"sleep_timeout_ms":...,"deep_sleep_enabled":...}` |
| `GET_LORA` | LoRa configuration | `OK {"frequency":...,"bandwidth":...,"spreading_factor":...}` |
| `GET_PAIRED_DEVICES` | List of paired devices | `OK [{"name":"...","mac":"...","aes_key":"..."},...]` |
| `GET_LORA_BANDS` | Available frequency bands | `OK [{"name":"EU868","frequencies":[...]},...]` |

### Configuration Commands (With JSON Parameters)

#### SET_GENERAL

Configure device name, mode, and slot ID.

**Format:**
```
SET_GENERAL {"device_name":"MyClicker","device_mode":"presenter","slot_id":1}
```

**Parameters:**
- `device_name` (string, max 31 chars): Device name (shown in BLE advertisement)
- `device_mode` (string): `"presenter"` or `"pc"`
- `slot_id` (integer, 0-255): Slot ID for multi-device setups

**Response:**
```
OK Configuration updated
```

#### SET_POWER_MANAGEMENT

Configure power management settings.

**Format:**
```
SET_POWER_MANAGEMENT {"sleep_timeout_ms":30000,"deep_sleep_enabled":true}
```

**Parameters:**
- `sleep_timeout_ms` (integer): Inactivity timeout before sleep (milliseconds)
- `deep_sleep_enabled` (boolean): Enable deep sleep mode

**Response:**
```
OK Power management updated
```

#### SET_LORA

Configure LoRa radio parameters.

**Format:**
```
SET_LORA {"frequency":868100000,"bandwidth":500000,"spreading_factor":7,"tx_power":14}
```

**Parameters:**
- `frequency` (integer): Frequency in Hz (e.g., 868100000 for 868.1 MHz)
- `bandwidth` (integer): Bandwidth in Hz (125000, 250000, 500000)
- `spreading_factor` (integer): SF7-SF12
- `tx_power` (integer): TX power in dBm (2-20)

**Response:**
```
OK LoRa configuration updated
```

### Device Pairing Commands

#### PAIR_DEVICE

Add a new paired device (for PC mode).

**Format:**
```
PAIR_DEVICE {"name":"Remote1","mac":"AA:BB:CC:DD:EE:FF","aes_key":"0123456789ABCDEF..."}
```

**Parameters:**
- `name` (string, max 31 chars): Device name
- `mac` (string): MAC address in format `AA:BB:CC:DD:EE:FF`
- `aes_key` (string, 64 hex chars): AES-256 key (32 bytes as hex string)

**Response:**
```
OK Device paired successfully
```

#### UPDATE_PAIRED_DEVICE

Update an existing paired device.

**Format:**
```
UPDATE_PAIRED_DEVICE {"mac":"AA:BB:CC:DD:EE:FF","name":"NewName","aes_key":"..."}
```

**Parameters:**
- `mac` (string, read-only): MAC address identifies the device
- `name` (string, optional): New device name
- `aes_key` (string, optional): New AES-256 key

**Response:**
```
OK Device updated successfully
```

#### UNPAIR_DEVICE

Remove a paired device.

**Format:**
```
UNPAIR_DEVICE {"mac":"AA:BB:CC:DD:EE:FF"}
```

**Parameters:**
- `mac` (string): MAC address of device to remove

**Response:**
```
OK Device unpaired successfully
```

## Error Handling

All errors return format: `ERROR <message>`

**Common Errors:**
- `ERROR Unknown command` - Command not recognized
- `ERROR Missing parameters` - Required JSON fields missing
- `ERROR Invalid JSON` - Malformed JSON payload
- `ERROR Invalid MAC address` - MAC format incorrect
- `ERROR Device not found` - Device doesn't exist in registry
- `ERROR Failed to save configuration` - NVS write failure

**Example:**
```
SET_GENERAL {"device_name":""}
ERROR Device name cannot be empty
```

## Python Example

```python
import asyncio
from bleak import BleakClient, BleakScanner

UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
UART_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # Device → Client
UART_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # Client → Device

async def send_command(client, command):
    """Send command and wait for response"""
    response = []
    
    def notification_handler(sender, data):
        response.append(data.decode('utf-8'))
    
    await client.start_notify(UART_TX_CHAR_UUID, notification_handler)
    
    # Send command with newline
    await client.write_gatt_char(UART_RX_CHAR_UUID, (command + "\n").encode('utf-8'))
    
    # Wait for response (simple approach - wait 1 second)
    await asyncio.sleep(1)
    
    await client.stop_notify(UART_TX_CHAR_UUID)
    
    return ''.join(response).strip()

async def main():
    # Scan for LoRaCue device
    print("Scanning for LoRaCue devices...")
    devices = await BleakScanner.discover()
    loracue = next((d for d in devices if d.name and d.name.startswith("LoRaCue-")), None)
    
    if not loracue:
        print("No LoRaCue device found")
        return
    
    print(f"Found: {loracue.name} ({loracue.address})")
    
    async with BleakClient(loracue.address) as client:
        print("Connected!")
        
        # Test connectivity
        response = await send_command(client, "PING")
        print(f"PING: {response}")
        
        # Get device info
        response = await send_command(client, "GET_DEVICE_INFO")
        print(f"Device Info: {response}")
        
        # Get general configuration
        response = await send_command(client, "GET_GENERAL")
        print(f"General Config: {response}")
        
        # Update device name
        response = await send_command(client, 
            'SET_GENERAL {"device_name":"MyClicker","device_mode":"presenter","slot_id":1}')
        print(f"Set General: {response}")
        
        # Get paired devices
        response = await send_command(client, "GET_PAIRED_DEVICES")
        print(f"Paired Devices: {response}")

asyncio.run(main())
```

## Advanced Usage

### Streaming Large Responses

Some commands (like `GET_PAIRED_DEVICES` with many devices) may return responses larger than a single BLE packet (MTU - 3 bytes).

**Handling:**
1. Enable notifications on TX characteristic
2. Send command
3. Accumulate notification data until newline (`\n`) is received
4. Parse complete response

### MTU Negotiation

Default MTU is 23 bytes (20 bytes payload). For better performance:

```python
# Request larger MTU (up to 512 bytes)
await client.exchange_mtu(512)
```

This reduces packet fragmentation for large commands/responses.

### Concurrent Commands

The command parser processes commands **sequentially**. Wait for response before sending next command.

**Don't do this:**
```python
await client.write_gatt_char(UART_RX_CHAR_UUID, b"GET_DEVICE_INFO\n")
await client.write_gatt_char(UART_RX_CHAR_UUID, b"GET_GENERAL\n")  # Too fast!
```

**Do this:**
```python
response1 = await send_command(client, "GET_DEVICE_INFO")
response2 = await send_command(client, "GET_GENERAL")
```

### Firmware Upgrade Rejection

If you try to send `FIRMWARE_UPGRADE` command via Nordic UART Service, you'll receive:

```
ERROR Use dedicated OTA GATT service (UUID 49589A79-7CC5-465D-BFF1-FE37C5065000) for firmware upgrades
```

This is intentional - firmware upgrades use a dedicated OTA service with proper flow control. See [BLE-OTA.md](BLE-OTA.md).

## Comparison with USB-UART

| Feature | BLE Nordic UART | USB-UART |
|---------|-----------------|----------|
| Range | 5-10 meters | Cable required |
| Speed | ~20 KB/s | ~115 KB/s |
| Pairing | Optional (recommended) | Not required |
| Commands | All except FIRMWARE_UPGRADE | All commands |
| Use Case | Wireless configuration | Development/debugging |
| Simultaneous Logging | Yes (UART0 for logs) | No (UART busy) |

## Security Considerations

### Without Pairing (Open Access)

- UART commands are **not encrypted** (characteristics use `ESP_GATT_PERM_READ/WRITE`)
- Anyone in BLE range can connect and configure device
- Suitable for development/testing only
- **OTA service will reject access** (requires encrypted connection)

### With Pairing (Recommended)

- All BLE communication is **AES-128 encrypted** at link layer
- Passkey authentication prevents unauthorized access
- Bonding stores keys in **NVS flash** (`CONFIG_BT_BLE_SMP_BOND_NVS_FLASH=y`)
- Keys persist across reboots - no re-pairing needed
- **Same pairing protects both UART and OTA services**
- Suitable for production deployment

### Pairing vs. Characteristic Permissions

**UART Service (Nordic UART):**
- TX characteristic: `ESP_GATT_PERM_READ` (no encryption required)
- RX characteristic: `ESP_GATT_PERM_WRITE` (no encryption required)
- **Works without pairing**, but communication is unencrypted

**OTA Service:**
- Control characteristic: `ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED`
- Data characteristic: `ESP_GATT_PERM_WRITE_ENCRYPTED`
- Progress characteristic: `ESP_GATT_PERM_READ_ENCRYPTED`
- **Requires pairing** - BLE stack rejects access without encryption

### Best Practices

1. **Always pair in production** - Prevents unauthorized configuration
2. **Use unique device names** - Easier to identify in crowded BLE environments
3. **Monitor connection status** - Disconnect idle clients to save power
4. **Validate all inputs** - Command parser validates JSON, but client should too
5. **Use OTA service for firmware** - Don't try to send binary data via UART service

## Troubleshooting

### Device Not Found in Scan

- Check device is powered on
- Verify BLE is enabled on client device
- Ensure device is not already connected to another client
- Check BLE range (stay within 10 meters)

### Connection Drops Immediately

- Device may be in deep sleep - press button to wake
- Check battery level (low battery may cause instability)
- Verify BLE stack is initialized on client

### No Response to Commands

- Check notifications are enabled on TX characteristic
- Verify command syntax (newline-terminated)
- Check for typos in command name (case-sensitive)
- Monitor serial logs on device for error messages

### Pairing Fails

- Ensure passkey is entered correctly (6 digits)
- Check passkey on device OLED display
- Try unpairing and re-pairing
- Clear bonding data on client device

### Response Truncated

- Increase MTU size (`exchange_mtu()`)
- Accumulate multiple notification packets
- Wait for newline before parsing response

## Related Documentation

- [BLE-OTA.md](BLE-OTA.md) - Firmware updates via BLE
- [OTA-SAFETY.md](OTA-SAFETY.md) - OTA safety mechanisms
- [COMMANDS.md](COMMANDS.md) - Complete command reference (if exists)

## Notes

- **Command buffer size:** 16,384 bytes (sufficient for large JSON payloads)
- **Response buffer:** Limited by BLE MTU (negotiate larger MTU for big responses)
- **Connection timeout:** Device may disconnect after inactivity (configurable via power management)
- **Concurrent connections:** Only one BLE client can connect at a time
- **UART logging:** UART0 remains available for logging even when BLE is active

## Example Use Cases

### 1. Field Configuration

Use smartphone app to configure device without USB cable:
- Change device name
- Adjust LoRa parameters
- Pair with PC receiver
- Update power settings

### 2. Multi-Device Setup

Configure multiple devices in a conference room:
- Scan for all LoRaCue devices
- Connect to each sequentially
- Set unique slot IDs
- Configure same LoRa frequency

### 3. Remote Diagnostics

Check device status without physical access:
- Query device info and version
- Check paired devices
- Verify LoRa configuration
- Monitor connection status

### 4. Automated Testing

Script-based testing of device configuration:
- Automated pairing tests
- Configuration validation
- Stress testing with rapid commands
- Integration with CI/CD pipelines
