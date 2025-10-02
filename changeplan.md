# LoRaCue Change Plan

## Overview

Implementation plan for USB HID keyboard emulation and USB CDC communication protocol for elegant device management.

## 1. USB HID Implementation ✅ COMPLETED

### Button Mapping (2 buttons with short/long press)

**PC Mode (Receiver):**

- Button A Short: Right Arrow (Next slide)
- Button A Long: F5 (Start slideshow)
- Button B Short: Left Arrow (Previous slide)
- Button B Long: Escape (Exit presentation)

**Presenter Mode (Remote):**

- Button A Short: Right Arrow (Next slide)
- Button A Long: Space (Start/Stop presentation)
- Button B Short: Left Arrow (Previous slide)
- Button B Long: B key (Blank/Unblank screen)

**Reserved:**

- Both Long: Menu (reserved for system)

### Component Structure

```plaintext
components/usb_hid/
├── usb_hid.c               # USB HID keyboard functionality
└── include/usb_hid.h       # HID interface
```

## 2. USB CDC Communication Protocol ✅ COMPLETED

### Rationale

**Elegance over Security:** USB CDC pairing eliminates the frustration of complex operations on a 128x64 OLED display. Users can perform device management through a professional desktop application instead of navigating tiny menus with 2 buttons.

### Protocol Commands (Text Format)

**Connection Management:**

```plaintext
PING                          → PONG
```

**Device Information:**

```plaintext
GET_VERSION                   → Git version string
GET_DEVICE_NAME              → Current device name
SET_DEVICE_NAME <name>       → Set device name
```

**LoRa Configuration:**

```plaintext
GET_LORA_CONFIG              → JSON with LoRa settings
SET_LORA_CONFIG <json>       → Update LoRa settings
```

**Device Pairing:**

```
PAIR_DEVICE <json>           → Pair with LoRa device
GET_PAIRED_DEVICES           → JSON array of paired devices
```

**Firmware Update:**

```
FW_UPDATE_START <json>       → Begin firmware update
FW_UPDATE_DATA <chunk> <data> → Send firmware chunk
FW_UPDATE_VERIFY             → Verify firmware
FW_UPDATE_COMMIT             → Apply firmware
```

### JSON Examples

**LoRa Configuration:**

```json
GET_LORA_CONFIG
→ {"frequency":915000000,"spreading_factor":7,"bandwidth":500,"coding_rate":5,"tx_power":14}

SET_LORA_CONFIG {"frequency":868100000,"spreading_factor":9,"bandwidth":250,"tx_power":20}
→ OK LoRa configuration updated
```

**Device Pairing:**

```json
PAIR_DEVICE {"name":"Hall-Remote","mac":"aa:bb:cc:dd:ee:ff","key":"a1b2c3d4..."}
→ OK Device paired successfully

GET_PAIRED_DEVICES
→ [{"name":"Hall-Remote","mac":"aa:bb:cc:dd:ee:ff"}]
```

**Firmware Update:**

```json
FW_UPDATE_START {"size":524288,"checksum":"sha256:abc123...","version":"v1.2.3"}
→ OK Ready for firmware data

FW_UPDATE_DATA 1 48656c6c6f576f726c64...
→ OK Data written
```

## 3. Data Storage Decisions ✅ COMPLETED

### Device Registry (NVS)

**Minimal pairing essentials only:**

```c
typedef struct {
    char device_name[32];           // "Hall-Remote"
    uint8_t mac_address[6];         // Device MAC for identification
    uint8_t aes_key[32];           // AES-256 key for LoRa encryption
} paired_device_t;
```

**Excluded from NVS:**

- `device_type` - Already stored in local device_config
- `last_seen` - No RTC, meaningless after reboot
- `last_sequence` - Security risk and flash wear

### Runtime State (RAM only)

```c
typedef struct {
    uint32_t tx_sequence;          // Outgoing sequence counter
    uint32_t rx_sequence;          // Expected incoming sequence
    uint32_t session_start_ms;     // When device came online
    uint32_t last_message_ms;      // Last message received
    bool is_online;                // Current connection status
} device_runtime_state_t;
```

## 4. Component Architecture ✅ COMPLETED

### Separation of Concerns

```
components/
├── usb_hid/                 # Keyboard emulation only
├── usb_cdc/                 # Communication protocol
│   ├── usb_cdc.c            # Command parsing/responses
│   └── include/usb_cdc.h    # CDC interface
├── lora_comm/               # LoRa communication logic
├── device_registry/         # Paired devices (name, MAC, AES key)
├── lora_config/             # LoRa settings (channel, SF, power)
├── wifi_config/             # WiFi settings (SSID, password, IP)
└── device_config/           # Local device settings (mode, brightness)
```

### WiFi Integration

**Extend existing config_wifi_server:**

- Add REST API endpoints for firmware updates
- Reuse existing WiFi infrastructure
- Same protocol commands over HTTP
- No separate "unified" interface needed

## 5. UI Enhancements

### LoRa Config Screen TODO

**Current:** 3 static presets (Short/Medium/Long Range)

**Planned:** Show actual parameter values

```
LoRa Settings:
> Short Range (SF7, CH5, 14dBm)     ← Current selection
  Medium Range (SF8, CH6, 17dBm)
  Long Range (SF9, CH7, 20dBm)
  Custom (SF9, CH8, 20dBm)          ← Appears when modified via CDC/WiFi
```

**Benefits:**

- Transparency: Always show actual parameters
- Flexibility: Support both preset and custom configurations
- Professional: Technical details for advanced users

## 6. Desktop Companion Application 🚧 IN PROGRESS

### Technology Stack: Tauri

**Why Tauri:**
- **Small footprint** (~10MB vs 100MB+ Electron)
- **Native performance** (Rust backend)
- **Modern web frontend** (React/Vue/Svelte)
- **Cross-platform** (Windows, macOS, Linux)
- **Secure by default** (no Node.js runtime)
- **Perfect for device tools** (native system access)

### Architecture

```
desktop-app/
├── src-tauri/              # Rust backend
│   ├── src/
│   │   ├── main.rs         # Tauri app entry
│   │   ├── serial.rs       # USB CDC communication
│   │   ├── device.rs       # Device management
│   │   └── firmware.rs     # OTA update logic
│   └── Cargo.toml          # Rust dependencies
├── src/                    # Frontend (React/TypeScript)
│   ├── components/
│   │   ├── DeviceList.tsx  # Connected devices
│   │   ├── PairingWizard.tsx # Device pairing flow
│   │   ├── LoRaConfig.tsx  # LoRa configuration
│   │   └── FirmwareUpdate.tsx # OTA updates
│   ├── hooks/
│   │   ├── useSerial.ts    # Serial communication
│   │   └── useDevice.ts    # Device state management
│   └── App.tsx             # Main application
└── package.json            # Frontend dependencies
```

### Core Features

**Device Discovery & Management:**
- Auto-detect LoRaCue devices via USB
- Show device info (name, version, LoRa config)
- Rename devices with validation
- Unpair devices with confirmation

**Professional Pairing Interface:**
- QR code scanning for device credentials
- Manual entry with MAC/key validation
- Bulk pairing for multiple devices
- Export/import pairing configurations

**LoRa Configuration:**
- Visual frequency/power sliders
- Preset configurations (Short/Medium/Long Range)
- Custom parameter validation
- Real-time configuration preview

**Firmware Updates:**
- Drag & drop firmware files
- Progress bars with ETA
- Automatic version detection
- Rollback capability
- Batch updates for multiple devices

### Implementation Plan

**Phase 1: Core Infrastructure**
```bash
# Initialize Tauri project
npm create tauri-app@latest loracue-manager
cd loracue-manager

# Add dependencies
cargo add serialport tokio serde
npm install @tanstack/react-query lucide-react
```

**Phase 2: Serial Communication**
```rust
// src-tauri/src/serial.rs
use serialport::{SerialPort, SerialPortType};
use tauri::command;

#[command]
pub async fn discover_devices() -> Result<Vec<Device>, String> {
    // Scan for LoRaCue devices on USB CDC ports
}

#[command] 
pub async fn send_command(port: String, cmd: String) -> Result<String, String> {
    // Send CDC command and return response
}
```

**Phase 3: Device Management UI**
```tsx
// src/components/DeviceList.tsx
export function DeviceList() {
  const { devices, isLoading } = useDevices();
  
  return (
    <div className="grid gap-4">
      {devices.map(device => (
        <DeviceCard key={device.port} device={device} />
      ))}
    </div>
  );
}
```

**Phase 4: Advanced Features**
- Firmware update with progress tracking
- Configuration backup/restore
- Multi-device operations
- Professional logging/diagnostics

### User Experience Goals

**IT Department Friendly:**
- No installation required (portable executable)
- Professional interface with technical details
- Bulk operations for device management
- Configuration export for backup/deployment
- Detailed logging for troubleshooting

**End User Friendly:**
- Simple device discovery
- Wizard-driven pairing process
- One-click firmware updates
- Visual configuration tools
- Clear status indicators

## 7. Implementation Priority ✅ UPDATED

1. ✅ **USB HID keyboard emulation** - Core presentation functionality
2. ✅ **USB CDC protocol foundation** - Basic commands (PING, GET_VERSION, etc.)
3. ✅ **Device pairing via CDC** - Elegant pairing experience
4. ✅ **LoRa configuration via CDC** - Professional configuration management
5. ✅ **Firmware update protocol** - Secure update mechanism
6. 🚧 **Desktop application (Tauri)** - User-friendly management interface
7. 🔄 **WiFi API integration** - Extend existing web interface
8. 🔄 **Production testing** - Hardware validation and optimization

## 8. Key Benefits ✅ ACHIEVED

**User Experience:**

- ✅ Complex operations via desktop app (elegant)
- ✅ Simple operations via OLED (quick access)
- 🚧 Professional device management
- ✅ No tiny-screen frustration

**Technical:**

- ✅ Clean separation of concerns
- ✅ Extensible protocol design
- ✅ Consistent across USB CDC and WiFi
- ✅ Minimal NVS storage requirements
- ✅ Security-conscious design

## 9. Success Criteria

- ✅ USB HID works in both PC and Presenter modes
- ✅ LoRa communication with dependency injection architecture
- ✅ USB CDC protocol with all planned commands
- 🚧 Desktop app can discover and pair devices elegantly
- 🚧 LoRa configuration possible without OLED navigation
- ✅ Firmware updates work over USB CDC
- ✅ All settings persist correctly in NVS
- 🔄 Professional user experience for IT departments

## 10. Next Steps: Desktop Application Development

### Immediate Tasks

1. **Initialize Tauri Project**
   ```bash
   npm create tauri-app@latest loracue-manager --template react-ts
   cd loracue-manager
   ```

2. **Add Serial Communication**
   - Rust serialport crate for USB CDC
   - Device discovery and enumeration
   - Command/response handling

3. **Build Core UI Components**
   - Device list with status indicators
   - Configuration panels for LoRa settings
   - Pairing wizard with validation

4. **Implement Firmware Updates**
   - File selection and validation
   - Progress tracking with chunked uploads
   - Error handling and recovery

### Development Environment

**Prerequisites:**
- Rust (latest stable)
- Node.js 18+ with npm/yarn
- Tauri CLI: `cargo install tauri-cli`

**Development Workflow:**
```bash
# Start development server
cargo tauri dev

# Build for production
cargo tauri build

# Cross-platform builds
cargo tauri build --target x86_64-pc-windows-msvc
cargo tauri build --target x86_64-apple-darwin
cargo tauri build --target x86_64-unknown-linux-gnu
```

This provides a complete roadmap for the professional desktop companion application that will make LoRaCue device management elegant and IT-friendly.
