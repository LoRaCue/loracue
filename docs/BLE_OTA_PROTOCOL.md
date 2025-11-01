# BLE OTA Protocol for LoRaCue

## Overview

LoRaCue uses the [espressif/ble_ota](https://components.espressif.com/components/espressif/ble_ota) component (v0.1.12) for wireless firmware updates over Bluetooth Low Energy 5.0. Signature verification is performed by LoRaCueManager before transmission to minimize device complexity and memory usage.

**BLE 5.0 Features:**
- **2M PHY**: 2x faster data transfer (2 Mbps vs 1 Mbps)
- **Backward compatible**: Falls back to 1M PHY for BLE 4.x devices
- **LE Secure Connections**: BLE 4.2+ security with ECDH key exchange

## Security Architecture

```
┌─────────────────┐         ┌──────────────┐         ┌─────────────┐
│ GitHub Release  │────────▶│LoRaCueManager│────────▶│   Device    │
│                 │         │              │         │             │
│ • firmware.bin  │         │ 1. Download  │         │ 3. Stream   │
│ • firmware.sig  │         │ 2. Verify    │         │ 4. Flash    │
│ • manifest.json │         │    signature │         │ 5. Reboot   │
└─────────────────┘         └──────────────┘         └─────────────┘
                                   │                        │
                                   │    BLE Pairing         │
                                   │◀──────────────────────▶│
                                   │  (Passkey + MITM)      │
```

**Security Layers:**
1. **Signature Verification** (LoRaCueManager): RSA-4096 signature validates firmware authenticity
2. **BLE Pairing**: Passkey authentication with MITM protection
3. **LE Secure Connections**: ECDH P-256 key exchange, AES-128-CCM encryption
4. **No Embedded Secrets**: Device firmware contains no private keys

**Performance:**
- **BLE 5.0 2M PHY**: ~2x faster OTA transfers (~1.1MB in ~45 seconds vs ~90 seconds)
- **Automatic fallback**: Uses 1M PHY if peer doesn't support BLE 5.0

## Firmware Package Structure

Each GitHub release contains firmware packages as ZIP files:

```
loracue-heltec_v3-v1.2.3.zip
├── heltec_v3.bin              # Main application binary (~1.1MB)
├── heltec_v3.bin.sig          # RSA-4096 signature (base64)
├── heltec_v3.bin.sha256       # SHA256 checksum
├── bootloader.bin             # ESP32-S3 bootloader
├── bootloader.bin.sig         # Bootloader signature
├── bootloader.bin.sha256      # Bootloader checksum
├── partition-table.bin        # Flash partition layout
├── partition-table.bin.sig    # Partition table signature
├── partition-table.bin.sha256 # Partition table checksum
├── manifest.json              # Firmware metadata
└── README.md                  # Installation instructions
```

**Combined Manifest:**
```
manifests.json                 # Array of all board manifests
```

## Protocol Flow

### 1. Download and Parse Manifests

```python
import requests
import json

# Download combined manifests
url = "https://github.com/LoRaCue/loracue/releases/latest/download/manifests.json"
manifests = requests.get(url).json()

# Find board-specific manifest
board_manifest = next(m for m in manifests if m["board_id"] == "heltec_v3")

print(f"Latest version: {board_manifest['version']}")
print(f"Download URL: {board_manifest['download_url']}")
```

**Manifest Structure:**
```json
{
  "board_id": "heltec_v3",
  "version": "1.2.3",
  "release_type": "release",
  "commit_sha": "abc123...",
  "tag": "v1.2.3",
  "download_url": "https://github.com/.../loracue-heltec_v3-v1.2.3.zip",
  "files": {
    "firmware": {
      "name": "heltec_v3.bin",
      "size": 1153024,
      "sha256": "a1b2c3...",
      "signature": "d4e5f6..."
    },
    "bootloader": { ... },
    "partition_table": { ... }
  }
}
```

### 2. Download Firmware Package

```python
import zipfile
import io

# Download ZIP
zip_url = board_manifest["download_url"]
zip_data = requests.get(zip_url).content

# Extract firmware binary
with zipfile.ZipFile(io.BytesIO(zip_data)) as z:
    firmware_data = z.read("heltec_v3.bin")
    signature_b64 = z.read("heltec_v3.bin.sig").decode('utf-8')
```

### 3. Verify Signature

```python
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding
import base64
import hashlib

# Load public key (embedded in LoRaCueManager)
with open('keys/firmware_public.pem', 'rb') as f:
    public_key = serialization.load_pem_public_key(f.read())

# Decode signature
signature = base64.b64decode(signature_b64)

# Calculate firmware hash
firmware_hash = hashlib.sha256(firmware_data).digest()

# Verify RSA-4096 signature
try:
    public_key.verify(
        signature,
        firmware_hash,
        padding.PKCS1v15(),
        hashes.SHA256()
    )
    print("✓ Signature verified - firmware is authentic")
except Exception as e:
    print(f"✗ Signature verification failed: {e}")
    exit(1)
```

### 4. Connect and Pair

```python
from bleak import BleakScanner, BleakClient

# Scan for LoRaCue device
devices = await BleakScanner.discover()
loracue = next(d for d in devices if d.name and "LoRaCue" in d.name)

# Connect
client = BleakClient(loracue.address)
await client.connect()

# Pair (user enters 6-digit passkey shown on device OLED)
passkey = input("Enter passkey from device display: ")
await client.pair(passkey)
```

### 5. Send Firmware via ESP BLE OTA

The `espressif/ble_ota` component provides a standard BLE service for OTA updates.

**Service UUID:** `0x8018` (ESP BLE OTA Service)

**Characteristics:**
- **Data RX:** `0x8020` - Write firmware data chunks
- **Control:** `0x8019` - Send control commands
- **Status:** `0x8021` - Read OTA status (notify enabled)

```python
# ESP BLE OTA UUIDs
OTA_SERVICE_UUID = "00008018-0000-1000-8000-00805f9b34fb"
OTA_DATA_UUID = "00008020-0000-1000-8000-00805f9b34fb"
OTA_CONTROL_UUID = "00008019-0000-1000-8000-00805f9b34fb"
OTA_STATUS_UUID = "00008021-0000-1000-8000-00805f9b34fb"

# Enable status notifications
await client.start_notify(OTA_STATUS_UUID, status_callback)

# Send firmware in chunks (MTU - 3 bytes, typically 512 bytes)
chunk_size = 512
for i in range(0, len(firmware_data), chunk_size):
    chunk = firmware_data[i:i+chunk_size]
    await client.write_gatt_char(OTA_DATA_UUID, chunk, response=False)
    
    # Progress tracking
    progress = (i / len(firmware_data)) * 100
    print(f"Progress: {progress:.1f}%")
```

### 6. Monitor Device Feedback

**Status Notifications:**
The device sends status updates via the Status characteristic:
- `0x00` - OTA started
- `0x01` - Writing to flash
- `0x02` - OTA complete
- `0xFF` - OTA failed

**Visual Feedback:**

OLED Display (ui_mini):
```
┌──────────────────────┐
│   Updating...        │
│ ┌────────────────┐   │
│ │████████░░░░░░░░│   │
│ └────────────────┘   │
│        45%           │
│ Do not power off!    │
└──────────────────────┘
```

E-Paper Display (ui_rich):
```
┌──────────────────────┐
│                      │
│  Firmware Update     │
│  in Progress         │
│                      │
│  Please wait...      │
│                      │
└──────────────────────┘
```

### 7. Automatic Reboot

On successful OTA:
1. Device validates new firmware
2. Sets boot partition to new firmware
3. Displays "OTA Successful!" (2 seconds)
4. Automatically reboots

On failure:
1. Device aborts OTA
2. Displays "OTA Failed!"
3. Remains on current firmware
4. Ready for retry

## Device Implementation Details

### Streaming Architecture

```c
// Ring buffer decouples BLE reception from flash writing
#define OTA_RINGBUF_SIZE 8192

// BLE callback writes to ring buffer
void ota_recv_fw_cb(uint8_t *buf, uint32_t length) {
    xRingbufferSend(s_ringbuf, buf, length, portMAX_DELAY);
}

// OTA task reads from ring buffer and writes to flash
void ota_task(void *param) {
    while (receiving) {
        uint8_t *data = xRingbufferReceive(s_ringbuf, &size, portMAX_DELAY);
        esp_ota_write(out_handle, data, size);
        vRingbufferReturnItem(s_ringbuf, data);
    }
}
```

**Benefits:**
- No RAM buffering of entire firmware (1.1MB firmware, 512KB RAM)
- Smooth BLE reception without blocking
- Direct streaming to flash partition

### Task Suspension

During OTA, UI tasks are suspended to prevent display conflicts:

```c
// Suspend before OTA
vTaskSuspend(ui_data_update_task_handle);
vTaskSuspend(status_bar_update_task_handle);
vTaskSuspend(pc_history_update_task_handle);

// Resume after OTA (or failure)
vTaskResume(ui_data_update_task_handle);
vTaskResume(status_bar_update_task_handle);
vTaskResume(pc_history_update_task_handle);
```

### Progress Tracking

```c
// Update UI every 1% change
static uint8_t last_progress = 0;
uint8_t current_progress = (bytes_written * 100) / total_size;

if (current_progress != last_progress) {
    ui_mini_update_ota_progress(current_progress);
    ESP_LOGI(TAG, "OTA Progress: %d%%", current_progress);
    last_progress = current_progress;
}
```

## Error Handling

| Error | Cause | Device Behavior | Recovery |
|-------|-------|-----------------|----------|
| Signature verification failed | Invalid/tampered firmware | N/A - LoRaCueManager blocks transfer | User must obtain valid firmware |
| Connection lost | BLE disconnected during transfer | Aborts OTA, stays on current firmware | Reconnect and retry |
| Flash write error | Hardware issue or corrupted data | Aborts OTA, stays on current firmware | Check device, retry |
| Partition error | Invalid partition table | Aborts OTA, stays on current firmware | Reflash via USB |
| Out of memory | Ring buffer full | Slows BLE reception (backpressure) | Automatic - ring buffer drains |

## Memory Constraints

**ESP32-S3 Memory:**
- Total RAM: 512KB
- Firmware size: ~1.1MB
- OTA partition: 2MB flash

**Why streaming is essential:**
- Cannot buffer entire firmware in RAM
- Ring buffer (8KB) provides smooth data flow
- Direct flash writes minimize memory usage

## Public Key Distribution

The RSA-4096 public key is embedded in LoRaCueManager:

```
LoRaCueManager/
└── keys/
    └── firmware_public.pem    # RSA-4096 public key
```

**Key Format:**
```
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA...
-----END PUBLIC KEY-----
```

## Example: Complete OTA Flow

```python
async def perform_ota(board_id: str):
    # 1. Download manifests
    manifests = requests.get(MANIFESTS_URL).json()
    manifest = next(m for m in manifests if m["board_id"] == board_id)
    
    # 2. Download firmware package
    zip_data = requests.get(manifest["download_url"]).content
    with zipfile.ZipFile(io.BytesIO(zip_data)) as z:
        firmware = z.read(f"{board_id}.bin")
        signature = z.read(f"{board_id}.bin.sig").decode()
    
    # 3. Verify signature
    verify_signature(firmware, signature, PUBLIC_KEY)
    
    # 4. Connect to device
    device = await scan_for_loracue()
    client = BleakClient(device.address)
    await client.connect()
    
    # 5. Pair
    passkey = input("Enter passkey: ")
    await client.pair(passkey)
    
    # 6. Send firmware
    await send_ota_firmware(client, firmware)
    
    # 7. Wait for reboot
    await asyncio.sleep(5)
    print("✓ OTA complete - device rebooting")
```

## References

- **ESP BLE OTA Component**: https://components.espressif.com/components/espressif/ble_ota
- **LoRaCueManager**: https://github.com/LoRaCue/loracue-manager
- **Release Assets**: https://github.com/LoRaCue/loracue/releases
