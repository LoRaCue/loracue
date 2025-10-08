# LoRa Protocol V2 - Current Implementation

## Overview
Production protocol with slot-based routing and extensible HID device support.

## Payload Structure (7 bytes)

```c
typedef struct __attribute__((packed)) {
    uint8_t version_slot;      // [7:4]=protocol_ver, [3:0]=slot_id (1-16)
    uint8_t type_flags;        // [7:4]=hid_type, [3:0]=flags/reserved
    uint8_t hid_report[5];     // HID report data (device-specific)
} lora_payload_v2_t;
```

### Byte 0: version_slot
- **Bits [7:4]**: Protocol version (0-15)
  - `0x1` - Current version
  - `0x2-F` - Future versions
- **Bits [3:0]**: Slot ID (1-16)
  - `0x1` - Default slot
  - `0x2-F` - Slots 2-15
  - `0x0` - Reserved (slot 16 encoded as 0)

### Byte 1: type_flags
- **Bits [7:4]**: HID device type (0-15)
  - `0x0` - None/System command
  - `0x1` - Keyboard
  - `0x2` - Mouse
  - `0x3` - Media keys
  - `0x4-F` - Reserved
- **Bits [3:0]**: Flags (reserved for future use)
  - Currently always `0x0`

### Bytes 2-6: hid_report (5 bytes)
Device-specific HID report data.

## HID Report Formats

### Keyboard (type=0x1, 5 bytes)
```c
struct {
    uint8_t modifiers;     // Bit 0=Ctrl, 1=Shift, 2=Alt, 3=GUI
    uint8_t keycode[4];    // Up to 4 simultaneous keys
}
```

**Modifier bits:**
- Bit 0: Left Ctrl (0x01)
- Bit 1: Left Shift (0x02)
- Bit 2: Left Alt (0x04)
- Bit 3: Left GUI/Win/Cmd (0x08)
- Bit 4: Right Ctrl (0x10)
- Bit 5: Right Shift (0x20)
- Bit 6: Right Alt (0x40)
- Bit 7: Right GUI (0x80)

### Mouse (type=0x2, 5 bytes) - Future
```c
struct {
    uint8_t buttons;       // Bit 0=Left, 1=Right, 2=Middle
    int16_t x;             // X movement (-32768 to 32767)
    int16_t y;             // Y movement
}
```

### Media Keys (type=0x3, 5 bytes) - Future
```c
struct {
    uint8_t key;           // Media key code
    uint8_t reserved[4];
}
```

## Macros

```c
// Protocol version
#define LORA_PROTOCOL_VERSION 0x01

// Byte 0: version_slot
#define LORA_VERSION(vs)      (((vs) >> 4) & 0x0F)
#define LORA_SLOT(vs)         ((vs) & 0x0F)
#define LORA_MAKE_VS(v, s)    ((((v) & 0x0F) << 4) | ((s) & 0x0F))

// Byte 1: type_flags
#define LORA_HID_TYPE(tf)     (((tf) >> 4) & 0x0F)
#define LORA_FLAGS(tf)        ((tf) & 0x0F)
#define LORA_MAKE_TF(t, f)    ((((t) & 0x0F) << 4) | ((f) & 0x0F))
```

## Example Payloads

### Example 1: Next Slide (Slot 3)

**Byte-by-Byte Breakdown:**

| Byte | Hex  | Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 | Field | Description |
|------|------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------------|
| 0    | `13` | 0 | 0 | 0 | 1 | 0 | 0 | 1 | 1 | version_slot | Protocol v1 (0001) + Slot 3 (0011) |
| 1    | `10` | 0 | 0 | 0 | 1 | 0 | 0 | 0 | 0 | type_flags | HID type=Keyboard (0001) + Flags=0 (0000) |
| 2    | `00` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | modifiers | No modifiers |
| 3    | `4E` | 0 | 1 | 0 | 0 | 1 | 1 | 1 | 0 | keycode[0] | PAGE_DOWN (0x4E) |
| 4    | `00` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | keycode[1] | Empty |
| 5    | `00` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | keycode[2] | Empty |
| 6    | `00` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | keycode[3] | Empty |

**Complete payload:** `13 10 00 4E 00 00 00` (7 bytes)

**Nibble breakdown:**
- Byte 0 `0x13`: High nibble `0001` (version 1) + Low nibble `0011` (slot 3)
- Byte 1 `0x10`: High nibble `0001` (keyboard) + Low nibble `0000` (no flags)

### Example 2: Previous Slide (Slot 1, default)
```
Hex: 11 10 00 4B 00 00 00

Byte 0: 0x11  [0001 0001]  version=1, slot=1
Byte 1: 0x10  [0001 0000]  hid_type=keyboard(1), flags=0
Byte 2: 0x00               modifiers=none
Byte 3: 0x4B               keycode[0]=PAGE_UP
Byte 4-6: 0x00             empty
```

### Example 3: Ctrl+Shift+F5 (Slot 1)
```
Hex: 11 10 03 3E 00 00 00

Byte 0: 0x11  [0001 0001]  version=1, slot=1
Byte 1: 0x10  [0001 0000]  hid_type=keyboard(1), flags=0
Byte 2: 0x03  [0000 0011]  modifiers=Ctrl(0x01)+Shift(0x02)
Byte 3: 0x3E               keycode[0]=F5
Byte 4-6: 0x00             empty
```

### Example 4: Black Screen (Slot 16)
```
Hex: 10 10 00 05 00 00 00

Byte 0: 0x10  [0001 0000]  version=1, slot=16 (0x0 encodes slot 16)
Byte 1: 0x10  [0001 0000]  hid_type=keyboard(1), flags=0
Byte 2: 0x00               modifiers=none
Byte 3: 0x05               keycode[0]='B' key
Byte 4-6: 0x00             empty
```

## Complete Packet Structure (22 bytes)

```
┌─────────────────────────────────────────────────────────────┐
│ Device ID (2 bytes, unencrypted)                            │
├─────────────────────────────────────────────────────────────┤
│ Encrypted Block (16 bytes, AES-256-ECB):                    │
│   - Sequence Number (2 bytes)                               │
│   - Command (1 byte): 0x01=CMD_HID_REPORT                   │
│   - Payload Length (1 byte): 0x07                           │
│   - Payload (7 bytes): version_slot + type_flags + report   │
│   - Padding (5 bytes): zeros for AES block alignment        │
├─────────────────────────────────────────────────────────────┤
│ MAC (4 bytes): HMAC-SHA256 truncated                        │
└─────────────────────────────────────────────────────────────┘
```

**Note:** AES-256 requires 16-byte blocks, so packet size is fixed at 22 bytes regardless of payload content. Variable-length payloads are not possible without breaking encryption.

## Commands

- `0x01` - CMD_HID_REPORT (with V2 payload)
- `0x80` - CMD_ACK

## Benefits

✅ **Slot-based routing**: One PC module controls up to 16 computers  
✅ **Nibble-packed**: Efficient use of every bit  
✅ **Extensible**: 16 protocol versions × 16 HID types × 16 flags  
✅ **Future-proof**: 4 flag bits reserved for extensions  
✅ **Compact**: 5 bytes for HID data (sufficient for keyboard/mouse)  
✅ **Secure**: AES-256 encryption + HMAC-SHA256 MAC  
✅ **Fixed-size**: 22-byte packets for reliable LoRa transmission  

## Implementation Status

✅ Protocol structures defined  
✅ Slot system implemented  
✅ Nibble-packed type_flags  
✅ Send functions updated  
✅ Receive functions updated  
✅ Button handler updated  
✅ PC mode updated  
✅ NVS persistence  
✅ UI for slot selection  

**Status:** Production-ready  
**Version:** V2 with slots (protocol_version=1)
