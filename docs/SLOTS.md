# LoRaCue Slot System

## Overview

Slots enable one PC module to control multiple computers simultaneously. Each USB-C port on the PC module is assigned a slot ID (1-16), allowing presenters to target specific PCs.

## Architecture

### Payload Structure (7 bytes)
```
Byte 0: version_slot [7:4]=protocol_ver, [3:0]=slot_id
Bytes 1-6: HID report
  - Byte 1: hid_type (keyboard=1, mouse=2, media=3)
  - Byte 2: modifiers (Ctrl/Shift/Alt/GUI)
  - Bytes 3-6: keycode[4] (up to 4 simultaneous keys)
```

### Slot ID Range
- **1-16**: Valid slot IDs (4 bits)
- **1**: Default slot (all modules start with slot 1)
- **0**: Reserved/invalid

## Use Cases

### Single PC (Default)
- Presenter: slot=1 (default)
- PC module: receives on slot 1, forwards to USB port 1

### Multi-PC Setup
- Presenter A: slot=1 → PC module USB port 1 → Computer A
- Presenter B: slot=2 → PC module USB port 2 → Computer B
- Presenter C: slot=3 → PC module USB port 3 → Computer C
- Presenter D: slot=4 → PC module USB port 4 → Computer D

### Conference Room
- Main presenter: slot=1 → Main projector PC
- Backup presenter: slot=1 → Same PC (redundancy)
- Side screen: slot=2 → Secondary display PC

## Configuration

### Presenter Module
```c
// Set slot via UI or config
lora_protocol_send_keyboard_reliable(slot_id, modifiers, keycode, 1000, 3);
```

### PC Module
```c
// Extract slot from received packet
uint8_t slot_id = LORA_SLOT(payload->version_slot);

// Route to appropriate USB port based on slot_id
usb_hid_send_key_to_port(slot_id, keycode);
```

## Hardware Design

### PC Module with 4 Microcontrollers
```
ESP32-S3 (LoRa receiver)
├── USB-C Port 1 → ATtiny85 (Slot 1) → Computer A
├── USB-C Port 2 → ATtiny85 (Slot 2) → Computer B
├── USB-C Port 3 → ATtiny85 (Slot 3) → Computer C
└── USB-C Port 4 → ATtiny85 (Slot 4) → Computer D
```

Communication: UART/I2C from ESP32 to each ATtiny85 with HID commands.

## Protocol Compatibility

- **Backward compatible**: Old modules default to slot 1
- **Forward compatible**: New modules can ignore slot if single-PC
- **No breaking changes**: Slot replaces unused hid_type bits in version_type byte
