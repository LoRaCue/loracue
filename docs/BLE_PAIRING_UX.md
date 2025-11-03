# BLE Pairing User Experience

## Overview

LoRaCue uses **BLE Secure Connections with passkey pairing** for secure device authentication. The pairing process is designed to be simple and secure.

---

## Pairing Flow

### 1. Initial Connection Attempt

**User Action:** Opens LoRaCueManager app and selects device

**Device Behavior:**
- Continues normal operation
- Waits for pairing request

---

### 2. Passkey Display

**Trigger:** Phone initiates pairing

**Device Display:**
```
┌──────────────────────────────────────┐
│  Status Bar (Battery, BLE, etc.)     │
├──────────────────────────────────────┤
│                                      │
│     ┌────────────────────────┐      │
│     │ ┌──┐                   │      │
│     │ │🔵│  Bluetooth        │      │
│     │ │  │  Connection       │      │
│     │ └──┘                   │      │
│     │      PIN: 123456       │      │
│     └────────────────────────┘      │
│                                      │
├──────────────────────────────────────┤
│  Bottom Bar (Mode, etc.)             │
└──────────────────────────────────────┘
```

**Overlay Details:**
- **Centered popup** over current screen
- **Large Bluetooth icon** (12x24px)
- **6-digit PIN** in bold font
- **Double border** for emphasis
- **White background** with black text

**Console Output:**
```
I (12345) ble_config: === Passkey Action Event ===
I (12346) ble_config: ===========================================
I (12347) ble_config:   PAIRING PASSKEY: 123456
I (12348) ble_config: ===========================================
```

---

### 3. User Enters PIN

**User Action:** Enters 6-digit PIN on phone

**Device Behavior:**
- Overlay remains visible
- Waits for phone to complete pairing

---

### 4. Pairing Complete

**Trigger:** Encryption established

**Device Behavior:**
- Overlay disappears automatically
- Returns to normal screen
- Connection is now bonded

**Console Output:**
```
I (12500) ble_config: Encryption change: status=0
```

---

## Security Configuration

### Pairing Method
- **I/O Capability:** `BLE_SM_IO_CAP_DISP_ONLY` (Display Only)
- **Device displays** 6-digit PIN
- **User enters** PIN on phone
- **MITM Protection:** Enabled
- **Secure Connections:** Enabled (ECDH)

### Bonding
- **Enabled:** Keys stored in NVS
- **Max Bonds:** 3 devices
- **Persistence:** Survives reboots
- **Reconnection:** Automatic without re-pairing

---

## Implementation Details

### Passkey Generation
```c
// Random 6-digit number (000000-999999)
pkey.passkey = esp_random() % 1000000;
```

### State Management
```c
typedef struct {
    uint16_t conn_handle;
    bool connected;
    bool pairing_active;    // TRUE during pairing
    uint32_t passkey;       // 6-digit PIN
    // ...
} ble_conn_state_t;
```

### Display Trigger
```c
// In presenter_main_screen.c and pc_mode_screen.c
uint32_t passkey;
if (bluetooth_config_get_passkey(&passkey)) {
    ui_pairing_overlay_draw(passkey);  // Draw overlay
}
```

### Lifecycle
1. **Passkey Action Event** → Store passkey, set `pairing_active = true`
2. **UI Refresh** → Check `pairing_active`, display overlay if true
3. **Encryption Change Event** → Clear passkey, set `pairing_active = false`
4. **UI Refresh** → Overlay no longer displayed

---

## User Experience Goals

### ✅ Achieved
- **Non-intrusive:** Overlay doesn't block critical information
- **Clear:** Large, readable 6-digit PIN
- **Automatic:** Appears and disappears without user action
- **Secure:** MITM protection with passkey verification
- **Persistent:** Bonding survives reboots

### Design Principles
- **Minimal interaction:** User only enters PIN once
- **Visual clarity:** High contrast, large fonts
- **Context preservation:** Overlay doesn't change screen
- **Automatic cleanup:** Overlay disappears when done

---

## Reconnection Behavior

### First Connection (Not Bonded)
1. User selects device in app
2. Device displays passkey overlay
3. User enters PIN
4. Pairing completes
5. Device stores bond

### Subsequent Connections (Bonded)
1. User selects device in app
2. **No passkey displayed** (already bonded)
3. Connection established immediately
4. Encrypted communication starts

---

## Error Handling

### Pairing Timeout
- **Behavior:** Overlay disappears after ~30 seconds
- **User Action:** Retry connection from app

### Wrong PIN Entered
- **Behavior:** Pairing fails, overlay disappears
- **User Action:** Retry connection, new PIN generated

### Connection Lost During Pairing
- **Behavior:** Overlay disappears immediately
- **User Action:** Retry connection from app

---

## Testing Checklist

### Visual Testing
- [ ] Overlay appears centered on screen
- [ ] PIN is clearly readable (6 digits, leading zeros)
- [ ] Bluetooth icon displays correctly
- [ ] Overlay has double border
- [ ] Text is black on white background

### Functional Testing
- [ ] Overlay appears when pairing starts
- [ ] Overlay disappears when pairing completes
- [ ] Overlay disappears if pairing fails
- [ ] Correct PIN displayed (matches console log)
- [ ] Overlay doesn't interfere with status bar

### Security Testing
- [ ] Different PIN each pairing attempt
- [ ] PIN not stored after pairing complete
- [ ] Bonded devices reconnect without PIN
- [ ] Non-bonded devices require PIN

---

## Future Enhancements

### Potential Improvements
- **Timeout indicator:** Show countdown timer
- **Pairing status:** "Waiting for PIN..." / "Verifying..."
- **Success animation:** Brief checkmark before overlay disappears
- **Sound feedback:** Beep when pairing completes (if speaker available)
- **Multi-language:** Translate "Bluetooth Connection" text

### Not Planned
- ❌ QR code pairing (no camera on device)
- ❌ NFC pairing (no NFC hardware)
- ❌ PIN entry on device (no keyboard)

---

## Code References

### Key Files
- `components/bluetooth_config/bluetooth_config.c` - Passkey generation and storage
- `components/ui_mini/ui_pairing_overlay.c` - Overlay rendering
- `components/ui_mini/screens/presenter_main_screen.c` - Overlay integration
- `components/ui_mini/screens/pc_mode_screen.c` - Overlay integration

### Key Functions
- `bluetooth_config_get_passkey()` - Get current passkey if pairing active
- `ui_pairing_overlay_draw()` - Render passkey overlay
- `ble_gap_event_handler()` - Handle BLE events (passkey, encryption)

---

## Comparison with Other Devices

### Similar to:
- **Bluetooth Keyboards:** Display PIN, user enters on host
- **Bluetooth Speakers:** Display PIN for initial pairing
- **Smart Watches:** Display PIN for phone pairing

### Different from:
- **Just Works:** No PIN required (less secure)
- **Numeric Comparison:** Both devices show same number (requires display on both)
- **Out of Band:** Uses NFC or QR code (not available)

---

## Accessibility

### Visual
- **High Contrast:** Black text on white background
- **Large Font:** 10pt bold for PIN
- **Clear Layout:** Icon + text, well-spaced

### Cognitive
- **Simple:** Only one action required (enter PIN)
- **Familiar:** Standard Bluetooth pairing flow
- **Forgiving:** Can retry if wrong PIN entered

### Physical
- **No device interaction:** PIN entry on phone only
- **Automatic:** Overlay appears/disappears automatically
