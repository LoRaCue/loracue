# LoRaCue User Interface Specification

## Overview

LoRaCue uses a **dual Heltec V3 system** with identical hardware serving different roles:
- **STAGE Remote**: Handheld wireless controller (battery powered)
- **PC Receiver**: USB-connected receiver with compound USB device (Serial + HID)

Both devices feature **128x64 OLED displays** with **two-button navigation** and **identical UI framework**.

## System Architecture

### Hardware Configuration
- **Remote Heltec V3**: Battery powered, sends LoRa commands
- **PC Receiver Heltec V3**: USB powered, receives LoRa + acts as USB HID device
- **USB Interface**: Compound device (Serial for stage detection + HID for keyboard/mouse)
- **No PC Software Required**: Everything runs on the Heltec V3 receiver

### Communication Flow
```
Button Press → LoRa → PC Receiver → USB HID Event → Presentation Software
```

## Boot Screen Design

### Boot Screen Layout (2 second display)
```
┌─────────────────────────────────────┐
│                                     │
│                                     │
│            LoRaCue                  │ ← 14pt Helvetica Bold, centered
│                                     │
│         Enterprise Remote           │ ← 8pt Helvetica Regular, centered
│                                     │
│                                     │
│                                     │
│ v1.2.3              Initializing...│ ← 6pt Helvetica Regular, corners
└─────────────────────────────────────┘
```

### Typography Specifications
- **"LoRaCue"**: u8g2_font_helvB14_tr, centered at y=32 (vertical center)
- **"Enterprise Remote"**: u8g2_font_helvR08_tr, centered at y=45
- **Version**: u8g2_font_helvR06_tr, bottom-left at (2, 60)
- **Status**: u8g2_font_helvR06_tr, bottom-right at (85, 60)

### Positioning Calculations (128x64 display)
```c
// Main title "LoRaCue" (14pt font ≈ 16px height)
u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
int title_width = u8g2_GetStrWidth(&u8g2, "LoRaCue");
u8g2_DrawStr(&u8g2, (128 - title_width) / 2, 32, "LoRaCue");

// Subtitle "Enterprise Remote" (8pt font ≈ 10px height)  
u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
int subtitle_width = u8g2_GetStrWidth(&u8g2, "Enterprise Remote");
u8g2_DrawStr(&u8g2, (128 - subtitle_width) / 2, 45, "Enterprise Remote");

// Version and status (6pt font ≈ 8px height)
u8g2_SetFont(&u8g2, u8g2_font_helvR06_tr);
u8g2_DrawStr(&u8g2, 2, 60, "v1.2.3");
u8g2_DrawStr(&u8g2, 85, 60, "Initializing...");
```

### Boot Sequence Behavior
1. **Display boot screen** for exactly 2 seconds
2. **Auto-transition** to main screen (no user input required)
3. **Clean appearance** - no borders, minimal elements
4. **Professional branding** - Consistent with enterprise identity

## STAGE Remote UI

### Main Screen Layout
```
┌─────────────────────────────────────┐
│ LoRaCue          [🔌] [📶] [●●●○] 75%│ 
├─────────────────────────────────────┤
│                                     │
│         PRESENTATION READY          │
│                                     │
│     [◄ PREV]        [NEXT ►]        │
│                                     │
├─────────────────────────────────────┤
│ RC-A4B2                 [◄+►] Menu  │
└─────────────────────────────────────┘
```

### Status Bar Icons
- **[🔌]** - USB connection status (connected/disconnected)
- **[📶]** - LoRa signal strength / **[📵]** - No connection
- **[●●●○] 75%** - Battery level with visual bar + percentage

### Navigation
- **PREV Button**: Send previous slide command
- **NEXT Button**: Send next slide command  
- **BOTH Buttons (Long Press)**: Enter menu system
- **Auto-return**: Return to main screen after 30s menu inactivity

## PC Receiver UI

### Main Screen Layout
```
┌─────────────────────────────────────┐
│ LoRaCue PC       [🔌] [📶] [●●●○] 75%│
├─────────────────────────────────────┤
│ 14:23:15 RC-A4B2 NEXT               │ ← Event log with timestamps
│ 14:23:12 RC-A4B2 PREV               │
│ 14:23:08 RC-A4B2 NEXT               │
│ 14:22:45 RC-A4B2 PREV               │
│ 14:22:41 RC-A4B2 NEXT               │
│ 14:22:38 RC-A4B2 PREV               │
├─────────────────────────────────────┤
│ PC-1F3A                 [◄+►] Menu  │ ← PC device ID + menu hint
└─────────────────────────────────────┘
```

### Event Log Features
- **Real-time updates**: New events appear at top
- **Auto-scroll**: Oldest entries disappear from bottom
- **Timestamp format**: HH:MM:SS
- **Device identification**: Shows which remote sent command
- **Command display**: PREV/NEXT events clearly shown

## Menu System

### Main Menu Structure
```
MENU SCREEN
├── Device Mode (PC/STAGE)
├── Battery Status
├── LoRa Settings  
├── Configuration Mode
├── Device Info
└── System Info
```

### Device Mode Selection
```
┌─────────────────────────────────────┐
│ ► PC Mode (USB Receiver)            │ ← Selected mode
│   STAGE Mode (Remote Control)       │
│                                     │
│ Current: STAGE Mode                 │
│ Restart required after change       │
│                                     │
│ [◄] Back         [●] Select         │
└─────────────────────────────────────┘
```

**Purpose**: Same hardware can operate as either PC receiver or handheld remote.

### Battery Status
```
┌─────────────────────────────────────┐
│ Battery Level: 75%                  │
│ Voltage: 3.8V                       │
│ Status: Discharging                 │
│ Estimated: 18h 24m remaining        │
│                                     │
│ Charging: Connect USB-C             │
│                                     │
│ [◄] Back                           │
└─────────────────────────────────────┘
```

### LoRa Settings (Venue-Based Presets)
```
┌─────────────────────────────────────┐
│ ► Conference Room (50m)             │
│   SF7, 14dBm, 500kHz - Fast        │
│                                     │
│   Auditorium (200m)                 │
│   SF8, 17dBm, 250kHz - Balanced    │
│                                     │
│   Stadium (500m)                    │
│   SF10, 20dBm, 125kHz - Max Range  │
│                                     │
│ [◄] Back    [▼▲] Select    [●] Set  │
└─────────────────────────────────────┘
```

**Presets**:
- **Conference Room**: Fast response, good battery life
- **Auditorium**: Balanced range and speed
- **Stadium**: Maximum range, slower response

### Configuration Mode
```
┌─────────────────────────────────────┐
│ ► Configuration Mode: Off           │
│   Start Config Mode                 │
│                                     │
│ Creates WiFi hotspot for:           │
│ • Firmware updates                  │
│ • Device settings                   │
│                                     │
│ [◄] Back         [●] Start          │
└─────────────────────────────────────┘
```

### Configuration Mode Active
```
┌─────────────────────────────────────┐
│ CONFIGURATION MODE ACTIVE           │
│ ████████████████████████████████    │
│ ██  ██████    ██████  ████  ██      │
│ ██  ██  ██  ██  ██  ██  ██  ██      │
│ ██  ██████  ██████  ██████  ██      │
│ ████████████████████████████████    │
│ Network: LoRaCue-A4B2               │
│ [◄+►] Exit Config Mode             │
└─────────────────────────────────────┘
```

**Alternative Text Mode** (for devices that can't scan QR):
```
┌─────────────────────────────────────┐
│ CONFIGURATION MODE ACTIVE           │
│                                     │
│ Network: LoRaCue-A4B2               │
│ Password: K7mP9nQ2                  │
│                                     │
│ Web: http://192.168.4.1             │
│                                     │
│ [◄+►] Exit Config Mode             │
└─────────────────────────────────────┘
```

### QR Code Implementation
- **Content**: `WIFI:T:WPA;S:LoRaCue-A4B2;P:K7mP9nQ2;H:false;;`
- **Size**: QR Version 2 (25x25 modules)
- **Scaling**: 2x2 pixels (50x50 total) for optimal scanability
- **Fallback**: Button press toggles between QR code and text mode
- **Positioning**: Centered in display with network name below

**Features**:
- **Unique AP Name**: `LoRaCue-{DeviceID}`
- **Hardware-based Password**: 8 chars (a-zA-Z0-9) from ESP32 serial
- **Web Interface**: Firmware updates, device configuration
- **Auto-timeout**: Disables after 15 minutes inactivity

### WiFi Password Generation Algorithm
```c
// Get ESP32 MAC address (6 bytes)
uint8_t mac[6];
esp_read_mac(mac, ESP_MAC_WIFI_STA);

// Use CRC32 hash of MAC address
uint32_t hash = esp_crc32_le(0, mac, 6);

// Convert to base62 (a-zA-Z0-9) string
char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
char password[9] = {0};
for (int i = 0; i < 8; i++) {
    password[i] = charset[hash % 62];
    hash /= 62;
}
// Result: "K7mP9nQ2" (8 characters, reproducible)
```

**Algorithm Benefits**:
- ✅ **Hardware-unique**: Based on ESP32 MAC address
- ✅ **Reproducible**: Same device always generates same password
- ✅ **No storage**: Generated on-demand from hardware
- ✅ **Secure**: CRC32 provides good distribution across character set
- ✅ **User-friendly**: Only alphanumeric characters, 8 char length

### Device Info
```
┌─────────────────────────────────────┐
│ Device Name: RC-A4B2                │
│ Mode: STAGE Remote                  │
│ LoRa Channel: 868.1 MHz            │
│ Device ID: A4B2                     │
│                                     │
│                                     │
│ [◄] Back                           │
└─────────────────────────────────────┘
```

### System Info
```
┌─────────────────────────────────────┐
│ Firmware: v1.2.3                   │
│ Hardware: Heltec LoRa V3           │
│ ESP-IDF: v5.5.0                    │
│ Uptime: 2h 34m                     │
│ Free RAM: 234KB                    │
│ Flash: 6.2MB free                  │
│ [◄] Back                           │
└─────────────────────────────────────┘
```

## Navigation Patterns

### Two-Button Navigation
- **PREV Button**: Navigate up/back, decrease values
- **NEXT Button**: Navigate down/forward, increase values  
- **BOTH Buttons**: Select/confirm, enter/exit menus
- **Long Press BOTH**: Access main menu from any screen

### Menu Navigation Indicators
- **►** - Currently selected item
- **[◄] Back** - Return to previous screen
- **[▼▲] Navigate** - Move between options
- **[●] Select** - Confirm selection
- **[◄+►] Action** - Special combined button action

## Technical Implementation

### Screen State Machine
```c
typedef enum {
    SCREEN_BOOT,
    SCREEN_MAIN,
    SCREEN_MENU,
    SCREEN_DEVICE_MODE,
    SCREEN_BATTERY,
    SCREEN_LORA_SETTINGS,
    SCREEN_CONFIG_MODE,
    SCREEN_CONFIG_ACTIVE,
    SCREEN_DEVICE_INFO,
    SCREEN_SYSTEM_INFO,
    SCREEN_LOW_BATTERY,
    SCREEN_CONNECTION_LOST
} oled_screen_t;
```

### Font Strategy
- **Large Font (u8g2_font_helvB14_tr)**: Main status text, titles (14pt Helvetica Bold)
- **Medium Font (u8g2_font_helvB10_tr)**: Menu items, values (10pt Helvetica Bold)  
- **Small Font (u8g2_font_helvR08_tr)**: Status bar, details (8pt Helvetica Regular)
- **Tiny Font (u8g2_font_helvR06_tr)**: Timestamps, fine details (6pt Helvetica Regular)
- **Custom Icons**: 8x8 bitmap icons for status indicators

**Font Family: Helvetica**
- ✅ **Highly readable**: Designed for clarity at small sizes
- ✅ **Professional appearance**: Clean, modern sans-serif
- ✅ **Multiple weights**: Regular and Bold variants available
- ✅ **Consistent spacing**: Uniform character width and height
- ✅ **u8g2 optimized**: Pre-rendered bitmap fonts for fast display

### Memory Optimization
- **Static layouts**: Pre-rendered screen templates
- **Dynamic content**: Update only changed elements
- **Single frame buffer**: 128x64 = 1KB RAM
- **Partial updates**: Redraw only dirty regions

### Power Management Integration
- **Screen timeout**: Dim after 30s, off after 2min
- **Wake on button**: Any button press activates display
- **Sleep integration**: Display off during deep sleep
- **Battery awareness**: Show low battery warnings

## Web Configuration Interface

### Available via Configuration Mode AP
- **Firmware Update**: Upload .bin files
- **Device Settings**: Name, LoRa presets, timeouts
- **System Information**: Logs, diagnostics, factory reset
- **Export/Import**: Backup and restore device configuration

### Security Features
- **Unique credentials** per device (hardware-based)
- **Temporary access** (auto-disable after timeout)
- **Local network only** (no internet routing)
- **HTTPS optional** (self-signed certificate)

## Design Principles

### Professional Appearance
- **Consistent layout**: Top status bar, bottom navigation
- **Clear hierarchy**: Important info prominently displayed
- **Minimal clutter**: Only essential information shown
- **Enterprise polish**: Professional iconography and spacing

### User Experience
- **Intuitive navigation**: Two-button system easy to learn
- **Immediate feedback**: Visual confirmation of all actions
- **Context awareness**: Different behaviors per screen
- **Error recovery**: Clear error messages with solutions

### Accessibility
- **High contrast**: Black text on white background
- **Large fonts**: Readable in presentation environments
- **Simple navigation**: Minimal button combinations
- **Audio feedback**: Optional buzzer for button presses

## Implementation Status

### Current State
- ✅ **OLED Display**: Working with u8g2-hal-esp-idf
- ✅ **Basic UI Framework**: Screen management and navigation
- ✅ **Status Icons**: Battery, LoRa, USB indicators
- ⏳ **Menu System**: To be implemented
- ⏳ **Configuration Mode**: Web interface development
- ⏳ **Event Logging**: PC receiver log display

### Next Steps
1. Implement complete menu navigation system
2. Add LoRa settings configuration screens
3. Develop web-based configuration interface
4. Create PC receiver event logging display
5. Add power management integration
6. Implement device mode switching

This UI specification provides a **professional, enterprise-grade interface** suitable for presentation environments while maintaining **simplicity and reliability** for critical presentation control scenarios.
