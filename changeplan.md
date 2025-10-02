# LoRaCue Change Plan

## Overview

Implementation plan for USB HID keyboard emulation and USB CDC communication protocol for elegant device management.

## 1. USB HID Implementation

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
├── usb_hid_pc.c         # PC mode mappings
├── usb_hid_presenter.c  # Presenter mode mappings
└── usb_hid_common.c     # Shared HID functionality
```

## 2. USB CDC Communication Protocol

### Rationale

**Elegance over Security:** USB CDC pairing eliminates the frustration of complex operations on a 128x64 OLED display. Users can perform device management through a professional desktop application instead of navigating tiny menus with 2 buttons.

### Protocol Commands (JSON Format)

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

```paintext
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
→ {"channel":5,"sf":7,"bandwidth":125,"power":14,"frequency":868100000}

SET_LORA_CONFIG {"channel":8,"sf":9,"bandwidth":250,"power":20}
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
→ OK Ready for 512KB firmware
```

## 3. Data Storage Decisions

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

## 4. Component Architecture

### Separation of Concerns

```
components/
├── usb_hid/                 # Keyboard emulation only
├── usb_cdc/                 # Communication protocol
│   ├── usb_cdc_protocol.c   # Command parsing/responses
│   ├── usb_cdc_pairing.c    # Device pairing logic
│   └── usb_cdc_firmware.c   # Firmware update handling
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

## 6. Desktop Companion Application

### Features

- Device discovery via USB CDC
- Rich pairing interface (no tiny OLED needed)
- Firmware update with progress bars
- Device management (rename, unpair, etc.)
- LoRa configuration with validation
- Bulk operations for multiple devices
- Professional IT-friendly interface

## 7. Implementation Priority

1. **USB HID keyboard emulation** - Core presentation functionality
2. **USB CDC protocol foundation** - Basic commands (PING, GET_VERSION, etc.)
3. **Device pairing via CDC** - Elegant pairing experience
4. **LoRa configuration via CDC** - Professional configuration management
5. **Firmware update protocol** - Secure update mechanism
6. **Desktop application** - User-friendly management interface
7. **WiFi API integration** - Extend existing web interface

## 8. Key Benefits

**User Experience:**

- Complex operations via desktop app (elegant)
- Simple operations via OLED (quick access)
- Professional device management
- No tiny-screen frustration

**Technical:**

- Clean separation of concerns
- Extensible protocol design
- Consistent across USB CDC and WiFi
- Minimal NVS storage requirements
- Security-conscious design

## 9. Success Criteria

- [ ] USB HID works in both PC and Presenter modes
- [ ] Desktop app can discover and pair devices elegantly
- [ ] LoRa configuration possible without OLED navigation
- [ ] Firmware updates work over both USB CDC and WiFi
- [ ] All settings persist correctly in NVS
- [ ] Professional user experience for IT departments
