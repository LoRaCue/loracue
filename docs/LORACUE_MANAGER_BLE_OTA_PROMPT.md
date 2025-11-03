# LoRaCueManager BLE OTA Implementation Prompt

## Objective

Implement BLE OTA firmware update functionality in LoRaCueManager (Rust) using the ESP BLE OTA protocol that is already implemented in the LoRaCue device firmware.

---

## Reference Implementation

**Protocol reference (Java, translate to Rust):**
- Repository: https://github.com/EspressifApps/esp-ble-ota-android
- Path: `app/src/main/java/com/espressif/bleota/`

**Key insight:** The ESP BLE OTA protocol is proven and compatible. Translate the Java logic to Rust using `btleplug` and `prost` crates.

---

## Device-Side Implementation (Already Complete)

### BLE OTA Service
- **Service UUID:** `0x8018`
- **Characteristics:**
  - `0x8020` - Receive Firmware (Write)
  - `0x8021` - OTA Status (Read/Indicate)
  - `0x8022` - Command (Write/Indicate)
  - `0x8023` - Customer (Write/Indicate)

### Protocol
Uses **Protocol Buffers** for message serialization:
```protobuf
message CmdStartOTA {
    uint64 file_size = 1;
    uint64 block_size = 2;
    string partition_name = 3;
}

message CmdSendFile {
    bytes data = 1;
}

message CmdFinishOTA {
}
```

### Security
- **Pairing Required:** OTA only accepted from bonded devices
- **Bonding:** Persistent across reboots (NVS storage)
- **Pairing Method:** Passkey display on device, user enters on phone
- **Encryption:** BLE Secure Connections with MITM protection

### Component Source
- **Fork:** https://github.com/LoRaCue/ble_ota
- **Upstream:** espressif/esp-iot-solution @ components/bluetooth/ble_profiles/esp/ble_ota
- **Version:** 0.1.15

---

## Requirements for LoRaCueManager (Rust)

### 1. BLE OTA Service Discovery
```rust
use btleplug::api::{Central, Peripheral, ScanFilter};
use uuid::Uuid;

const OTA_SERVICE_UUID: Uuid = Uuid::from_u128(0x8018);
const RECV_FW_UUID: Uuid = Uuid::from_u128(0x8020);
const OTA_STATUS_UUID: Uuid = Uuid::from_u128(0x8021);
const COMMAND_UUID: Uuid = Uuid::from_u128(0x8022);

// Scan for LoRaCue devices
// Connect to selected device
// Discover services and find OTA service
// Verify all 4 characteristics are present
```

### 2. Pairing Flow
```rust
// If device not bonded:
//   - Initiate pairing via OS Bluetooth API
//   - Display "Enter PIN shown on device" dialog
//   - User enters 6-digit PIN from device OLED
//   - Complete bonding (handled by OS)
// If device already bonded:
//   - Connect directly (no PIN needed)
```

### 3. Firmware Selection
```rust
use std::fs::File;
use std::io::Read;

// Allow user to select .bin file from:
//   - App bundle (embedded firmware)
//   - File system
//   - Download from server (future)
// Validate file:
//   - Check file size > 0
//   - Check file extension is .bin
//   - Optionally verify ESP32-S3 app header
```

### 4. OTA Transfer Protocol
```rust
use prost::Message;

// Step 1: Start OTA
let start_ota = CmdStartOta {
    file_size: firmware_data.len() as u64,
    block_size: 512,
    partition_name: "ota_0".to_string(),
};
let payload = SendFilePayload {
    msg: SendFileMsgType::TypeCmdStartOta as i32,
    status: Status::Success as i32,
    payload: Some(send_file_payload::Payload::CmdStartOta(start_ota)),
};
// Serialize with prost and write to Command characteristic

// Step 2: Send firmware data
let total_blocks = (file_size + block_size - 1) / block_size;
for block_idx in 0..total_blocks {
    let start = block_idx * block_size;
    let end = std::cmp::min(start + block_size, file_size);
    let data = &firmware_data[start..end];
    
    let send_file = CmdSendFile {
        data: data.to_vec(),
    };
    let payload = SendFilePayload {
        msg: SendFileMsgType::TypeCmdSendFile as i32,
        status: Status::Success as i32,
        payload: Some(send_file_payload::Payload::CmdSendFile(send_file)),
    };
    // Write to Receive FW characteristic
    // Update progress: (block_idx + 1) * 100 / total_blocks
}

// Step 3: Finish OTA
let finish_ota = CmdFinishOta {};
let payload = SendFilePayload {
    msg: SendFileMsgType::TypeCmdFinishOta as i32,
    status: Status::Success as i32,
    payload: Some(send_file_payload::Payload::CmdFinishOta(finish_ota)),
};
// Write to Command characteristic

// Step 4: Device reboots automatically
// Wait for disconnection
```

### 5. Progress Tracking
```rust
use std::sync::Arc;
use tokio::sync::RwLock;

struct OtaProgress {
    bytes_sent: usize,
    total_bytes: usize,
    percentage: u8,
    speed_bps: f64,
}

// Update UI with progress
// Allow cancellation via channel
```

### 6. Error Handling
```rust
#[derive(Debug, thiserror::Error)]
enum OtaError {
    #[error("Device not found")]
    DeviceNotFound,
    #[error("Connection failed: {0}")]
    ConnectionFailed(String),
    #[error("Pairing failed")]
    PairingFailed,
    #[error("OTA service not found")]
    ServiceNotFound,
    #[error("Transfer failed: {0}")]
    TransferFailed(String),
}

// Handle errors with user-friendly messages
// Allow retry
```

---

## Implementation Steps

### Phase 1: BLE Infrastructure
1. Add `btleplug` crate for cross-platform BLE
2. Implement device scanning and connection
3. Add pairing flow (OS-level, show PIN dialog)
4. Test connection and bonding persistence

### Phase 2: Protocol Buffers
1. Copy `.proto` files from LoRaCue firmware:
   - `components/ble_ota/proto/ota_send_file.proto`
   - `components/ble_ota/proto/constants.proto`
2. Add `prost` and `prost-build` to dependencies
3. Generate Rust structs in `build.rs`
4. Create serialization helpers

### Phase 3: OTA Transfer Logic
1. Implement OTA state machine
2. Add firmware chunking logic
3. Implement progress tracking with channels
4. Add error handling and retry logic

### Phase 4: UI Integration
1. Add "Update Firmware" button in device view
2. Create firmware selection dialog
3. Create OTA progress view with:
   - Progress bar
   - Status text
   - Cancel button
4. Add success/error dialogs

### Phase 5: Testing
1. Test with bundled firmware
2. Test pairing flow (first time)
3. Test reconnection (already bonded)
4. Test transfer interruption and retry
5. Test with different firmware sizes
6. Test error scenarios

---

## Key Files to Reference

### From esp-ble-ota-android
```
app/src/main/java/com/espressif/bleota/
├── BleOtaManager.java          # Main OTA logic
├── BleConnection.java          # BLE connection handling
├── ProtobufHelper.java         # Protobuf serialization
└── OtaActivity.java            # UI for OTA process
```

### From LoRaCue Firmware
```
components/ble_ota/
├── proto/
│   ├── ota_send_file.proto     # Protocol definition
│   └── constants.proto         # Status codes
├── include/ble_ota.h           # C API
└── src/nimble_ota.c            # Implementation
```

---

## Protocol Details

### Message Flow
```
Phone                           Device
  |                               |
  |--- CmdStartOTA -------------->|  (file_size, block_size)
  |<-- RespStartOTA --------------|  (status: SUCCESS)
  |                               |
  |--- CmdSendFile (block 0) ---->|
  |<-- RespSendFile --------------|  (status: SUCCESS)
  |--- CmdSendFile (block 1) ---->|
  |<-- RespSendFile --------------|  (status: SUCCESS)
  |         ...                   |
  |--- CmdSendFile (block N) ---->|
  |<-- RespSendFile --------------|  (status: SUCCESS)
  |                               |
  |--- CmdFinishOTA ------------->|
  |<-- RespFinishOTA -------------|  (status: SUCCESS)
  |                               |
  |<-- Disconnect -----------------|  (device reboots)
```

### Status Codes (from constants.proto)
```protobuf
enum Status {
    Success = 0;
    InvalidSecScheme = 1;
    InvalidProto = 2;
    TooManySessions = 3;
    InvalidArgument = 4;
    InternalError = 5;
    CryptoError = 6;
    InvalidSession = 7;
}
```

### Block Size Considerations
- **MTU:** Negotiate maximum MTU (typically 512 bytes)
- **Block Size:** Use MTU - 20 bytes (for BLE overhead)
- **Typical:** 492 bytes per block
- **Total Blocks:** `ceil(file_size / block_size)`

---

## Security Considerations

### Pairing
- **First Connection:** User must enter PIN from device screen
- **Subsequent:** Automatic reconnection (bonded)
- **Bond Storage:** Android stores bond in system settings
- **Unbonding:** User can forget device in Bluetooth settings

### OTA Security
- **Device-Side Check:** Firmware only accepted from bonded devices
- **Transport Security:** BLE encryption (AES-128)
- **Signature Verification:** Not implemented (postponed)
- **Trust Model:** Device trusts firmware from bonded LoRaCueManager app

### Best Practices
- Verify device name before OTA (prevent wrong device update)
- Show device MAC address in confirmation dialog
- Warn user not to disconnect during update
- Disable phone sleep during OTA
- Keep screen on during transfer

---

## User Experience Flow

### Happy Path
1. User opens LoRaCueManager
2. User selects LoRaCue device from list
3. User taps "Update Firmware"
4. If not bonded:
   - App shows "Enter PIN from device"
   - User enters 6-digit PIN
   - Pairing completes
5. App shows firmware selection dialog
6. User selects firmware file
7. App shows confirmation: "Update device [name] to version X.Y.Z?"
8. User confirms
9. Progress screen shows:
   - "Preparing update..."
   - "Transferring firmware... 45%"
   - "Finalizing update..."
10. Success dialog: "Update complete! Device will reboot."
11. App returns to device list

### Error Scenarios
- **Wrong PIN:** "Pairing failed. Please check the PIN on your device."
- **Connection Lost:** "Connection lost. Retry update?"
- **Low Battery:** "Device battery low. Please charge before updating."
- **Transfer Failed:** "Update failed. Device is still functional. Retry?"

---

## Testing Checklist

### Functional
- [ ] Discover LoRaCue device
- [ ] Connect to device
- [ ] Pair with device (first time)
- [ ] Reconnect to bonded device
- [ ] Discover OTA service
- [ ] Select firmware file
- [ ] Start OTA transfer
- [ ] Monitor progress
- [ ] Complete OTA successfully
- [ ] Device reboots with new firmware

### Error Handling
- [ ] Device not found
- [ ] Connection timeout
- [ ] Wrong PIN entered
- [ ] OTA service not found
- [ ] File selection cancelled
- [ ] Transfer interrupted (user cancels)
- [ ] Transfer interrupted (connection lost)
- [ ] Device battery dies during OTA
- [ ] Invalid firmware file

### Edge Cases
- [ ] Very small firmware (<1KB)
- [ ] Very large firmware (>2MB)
- [ ] Multiple devices nearby
- [ ] Phone Bluetooth disabled
- [ ] Phone goes to sleep during transfer
- [ ] App backgrounded during transfer
- [ ] Phone call during transfer

---

## Dependencies

### Cargo.toml
```toml
[dependencies]
# BLE - Cross-platform Bluetooth Low Energy
btleplug = "0.11"

# Protocol Buffers
prost = "0.12"
bytes = "1.5"

# Async runtime
tokio = { version = "1.35", features = ["full"] }

# Error handling
thiserror = "1.0"
anyhow = "1.0"

# UUID
uuid = "1.6"

[build-dependencies]
prost-build = "0.12"
```

### build.rs
```rust
fn main() {
    prost_build::Config::new()
        .out_dir("src/proto")
        .compile_protos(
            &[
                "proto/ota_send_file.proto",
                "proto/constants.proto",
            ],
            &["proto/"],
        )
        .unwrap();
}
```

---

## Code Structure Suggestion

```
src/
├── ble/
│   ├── mod.rs                     # BLE module
│   ├── manager.rs                 # BLE connection management
│   ├── ota.rs                     # OTA-specific logic
│   └── scanner.rs                 # Device scanning
├── ota/
│   ├── mod.rs                     # OTA module
│   ├── state.rs                   # OTA state machine
│   ├── transfer.rs                # Transfer logic
│   └── firmware.rs                # Firmware file handling
├── proto/
│   ├── mod.rs                     # Generated protobuf code
│   ├── ota_send_file.rs           # Generated from .proto
│   └── constants.rs               # Generated from .proto
└── ui/
    ├── ota_view.rs                # OTA UI components
    └── progress.rs                # Progress display

proto/                             # Source .proto files
├── ota_send_file.proto            # Copied from firmware
└── constants.proto                # Copied from firmware
```

---

## Success Criteria

### Must Have
- ✅ Connect to LoRaCue device via BLE
- ✅ Pair with device using PIN
- ✅ Select firmware file
- ✅ Transfer firmware successfully
- ✅ Show progress during transfer
- ✅ Handle errors gracefully
- ✅ Device boots with new firmware

### Nice to Have
- 📦 Bundle firmware in app assets
- 🔄 Check for firmware updates from server
- 📊 Show transfer speed and ETA
- 🔔 Notification when update complete
- 📝 Log OTA attempts for debugging
- 🎨 Polished UI with animations

---

## Rust Implementation Example

### BLE Connection
```rust
use btleplug::api::{Central, Manager as _, Peripheral as _, ScanFilter};
use btleplug::platform::{Adapter, Manager, Peripheral};
use uuid::Uuid;

const OTA_SERVICE_UUID: Uuid = uuid::uuid!("00008018-0000-1000-8000-00805f9b34fb");

async fn connect_device(adapter: &Adapter, device_name: &str) -> Result<Peripheral, OtaError> {
    adapter.start_scan(ScanFilter::default()).await?;
    tokio::time::sleep(Duration::from_secs(5)).await;
    
    let peripherals = adapter.peripherals().await?;
    for peripheral in peripherals {
        if let Some(props) = peripheral.properties().await? {
            if props.local_name.as_deref() == Some(device_name) {
                peripheral.connect().await?;
                peripheral.discover_services().await?;
                return Ok(peripheral);
            }
        }
    }
    Err(OtaError::DeviceNotFound)
}
```

### OTA Transfer
```rust
use prost::Message;
use tokio::sync::mpsc;

async fn perform_ota(
    peripheral: &Peripheral,
    firmware: Vec<u8>,
    progress_tx: mpsc::Sender<OtaProgress>,
) -> Result<(), OtaError> {
    // Find characteristics
    let chars = peripheral.characteristics();
    let recv_fw_char = chars.iter()
        .find(|c| c.uuid == uuid::uuid!("00008020-0000-1000-8000-00805f9b34fb"))
        .ok_or(OtaError::ServiceNotFound)?;
    let cmd_char = chars.iter()
        .find(|c| c.uuid == uuid::uuid!("00008022-0000-1000-8000-00805f9b34fb"))
        .ok_or(OtaError::ServiceNotFound)?;
    
    // Step 1: Start OTA
    let start_ota = CmdStartOta {
        file_size: firmware.len() as u64,
        block_size: 512,
        partition_name: "ota_0".to_string(),
    };
    let payload = SendFilePayload {
        msg: SendFileMsgType::TypeCmdStartOta as i32,
        status: Status::Success as i32,
        payload: Some(send_file_payload::Payload::CmdStartOta(start_ota)),
    };
    let mut buf = Vec::new();
    payload.encode(&mut buf)?;
    peripheral.write(cmd_char, &buf, WriteType::WithResponse).await?;
    
    // Step 2: Send firmware blocks
    let block_size = 512;
    let total_blocks = (firmware.len() + block_size - 1) / block_size;
    
    for block_idx in 0..total_blocks {
        let start = block_idx * block_size;
        let end = std::cmp::min(start + block_size, firmware.len());
        let data = &firmware[start..end];
        
        let send_file = CmdSendFile {
            data: data.to_vec(),
        };
        let payload = SendFilePayload {
            msg: SendFileMsgType::TypeCmdSendFile as i32,
            status: Status::Success as i32,
            payload: Some(send_file_payload::Payload::CmdSendFile(send_file)),
        };
        let mut buf = Vec::new();
        payload.encode(&mut buf)?;
        peripheral.write(recv_fw_char, &buf, WriteType::WithResponse).await?;
        
        // Update progress
        let progress = OtaProgress {
            bytes_sent: end,
            total_bytes: firmware.len(),
            percentage: ((block_idx + 1) * 100 / total_blocks) as u8,
            speed_bps: 0.0, // Calculate from timing
        };
        progress_tx.send(progress).await?;
    }
    
    // Step 3: Finish OTA
    let finish_ota = CmdFinishOta {};
    let payload = SendFilePayload {
        msg: SendFileMsgType::TypeCmdFinishOta as i32,
        status: Status::Success as i32,
        payload: Some(send_file_payload::Payload::CmdFinishOta(finish_ota)),
    };
    let mut buf = Vec::new();
    payload.encode(&mut buf)?;
    peripheral.write(cmd_char, &buf, WriteType::WithResponse).await?;
    
    Ok(())
}
```

### Error Types
```rust
use thiserror::Error;

#[derive(Debug, Error)]
pub enum OtaError {
    #[error("Device not found")]
    DeviceNotFound,
    
    #[error("Connection failed: {0}")]
    ConnectionFailed(String),
    
    #[error("Pairing failed")]
    PairingFailed,
    
    #[error("OTA service not found")]
    ServiceNotFound,
    
    #[error("Transfer failed: {0}")]
    TransferFailed(String),
    
    #[error("BLE error: {0}")]
    BleError(#[from] btleplug::Error),
    
    #[error("Protobuf error: {0}")]
    ProtobufError(#[from] prost::DecodeError),
    
    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),
}
```

---

## Additional Resources

### Rust Crates
- **btleplug:** https://github.com/deviceplug/btleplug (Cross-platform BLE)
- **prost:** https://github.com/tokio-rs/prost (Protocol Buffers)
- **tokio:** https://tokio.rs (Async runtime)

### Documentation
- **btleplug Examples:** https://github.com/deviceplug/btleplug/tree/master/examples
- **prost Tutorial:** https://github.com/tokio-rs/prost#tutorial
- **Protocol Buffers:** https://developers.google.com/protocol-buffers

### LoRaCue Firmware
- **BLE OTA Component:** `~/Source/LoRaCue/components/ble_ota/`
- **Security Analysis:** `~/Source/LoRaCue/docs/BLE_SECURITY_ANALYSIS.md`
- **Pairing UX:** `~/Source/LoRaCue/docs/BLE_PAIRING_UX.md`

### Reference (Java, translate to Rust)
- **ESP BLE OTA Android:** https://github.com/EspressifApps/esp-ble-ota-android
- **Key File:** `app/src/main/java/com/espressif/bleota/BleOtaManager.java`

---

## Notes

### Why This Will Work
1. **Same Protocol:** Device uses espressif/ble_ota component
2. **Reference App:** ESP BLE OTA Android provides protocol reference
3. **Proven:** Protocol is maintained by Espressif
4. **Compatible:** Protocol buffers ensure wire-format compatibility
5. **btleplug:** Mature cross-platform BLE library for Rust

### What's Different from Reference
- **Language:** Rust instead of Java/Kotlin
- **BLE Library:** btleplug instead of Android BLE API
- **Security:** We require bonding, reference app may not
- **UI/UX:** Integrate into LoRaCueManager design

### Implementation Strategy
1. Use Java reference for protocol understanding
2. Implement in Rust using btleplug + prost
3. Add security checks (verify bonding)
4. Integrate with LoRaCueManager UI
5. Add LoRaCue-specific features

---

## Contact

For questions about the device-side implementation:
- **Firmware Repo:** https://github.com/LoRaCue/loracue
- **BLE OTA Fork:** https://github.com/LoRaCue/ble_ota
- **Documentation:** `~/Source/LoRaCue/docs/`
