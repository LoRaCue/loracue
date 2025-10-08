# LoRa Protocol V2 Design

## Overview
Future-proof protocol design for extensible HID device support.

## Protocol

### Payload Structure (7 bytes)
```c
typedef struct __attribute__((packed)) {
    uint8_t version_type;      // [7:4]=protocol_ver (0-15), [3:0]=hid_type (0-15)
    uint8_t hid_report[6];     // HID report data (device-specific)
} lora_payload_t;
```

### Protocol Versions
- `0x0` - Reserved
- `0x1` - Current/MVP (keyboard only)
- `0x2-F` - Future

### HID Device Types
- `0x0` - None/System command
- `0x1` - Keyboard
- `0x2` - Mouse
- `0x3` - Media keys
- `0x4` - Consumer control
- `0x5` - Gamepad
- `0x6-F` - Reserved

### Commands
- `0x01` - CMD_HID_REPORT (with payload defining device type and data)
- `0x80` - CMD_ACK
- `0x81` - CMD_NACK
- `0xF0` - CMD_PING
- `0xFF` - CMD_RESET

### HID Report Formats

#### Keyboard (type=0x1, 6 bytes)
```c
struct {
    uint8_t modifiers;     // Bit 0=Ctrl, 1=Shift, 2=Alt, 3=GUI
    uint8_t reserved;      // Always 0
    uint8_t keycode[4];    // Up to 4 simultaneous keys
}
```

#### Mouse (type=0x2, 6 bytes)
```c
struct {
    uint8_t buttons;       // Bit 0=Left, 1=Right, 2=Middle
    uint8_t reserved;
    int16_t x;             // X movement (-32768 to 32767)
    int16_t y;             // Y movement
}
```

#### Media Keys (type=0x3, 6 bytes)
```c
struct {
    uint8_t key;           // 0x01=Play/Pause, 0x02=Next, 0x03=Prev, 0x04=Mute, etc.
    uint8_t reserved[5];
}
```

### Example Packets

**Next Slide (Right Arrow):**
```
Command: 0x01 (CMD_HID_REPORT)
version_type: 0x11  (version=1, type=1=keyboard)
hid_report:   [0x00, 0x00, 0x4F, 0x00, 0x00, 0x00]
               └mods └res  └Right└──────────────┘
```

**Mouse Click:**
```
Command: 0x01 (CMD_HID_REPORT)
version_type: 0x12  (version=1, type=2=mouse)
hid_report:   [0x01, 0x00, 0x00, 0x00, 0x00, 0x00]
               └left └res  └─x=0─┘ └─y=0─┘
```

### Macros
```c
#define LORA_VERSION(vt)  (((vt) >> 4) & 0x0F)
#define LORA_HID_TYPE(vt) ((vt) & 0x0F)
#define LORA_MAKE_VT(v,t) ((((v) & 0x0F) << 4) | ((t) & 0x0F))
```

### Benefits
- ✅ Single command type for all HID devices
- ✅ Protocol version in every packet
- ✅ Extensible to 16 HID device types
- ✅ 6 bytes for HID data (enough for most HID reports)
- ✅ No backward compatibility baggage
- ✅ Clean, future-proof design

### Migration Path
1. Implement new protocol structures
2. Update send/receive functions
3. Update button handler to use new API
4. Update PC mode to parse new payload
5. Test with all HID device types

## Implementation Status
- [ ] Protocol structures defined
- [ ] Send functions updated
- [ ] Receive functions updated
- [ ] Button handler updated
- [ ] PC mode updated
- [ ] Testing complete

**Status:** Design documented, implementation pending
**Target:** Post-MVP enhancement
