# USB Composite Device Configuration

LoRaCue presents as a USB composite device with multiple interfaces:

## Development Builds (default)

When `LORACUE_RELEASE` is **not** defined:

- **HID Keyboard** - Presentation control (arrow keys, etc.)
- **CDC-ACM Port 0** - Application interface (configuration, pairing, commands)
- **CDC-ACM Port 1** - Debug console (ESP_LOG* output) + Firmware flashing

### Port Assignment

| Interface | Port | Purpose | Used By | DTR/RTS |
|-----------|------|---------|---------|---------|
| HID | - | Keyboard emulation | OS | N/A |
| CDC-ACM 0 | /dev/ttyACM0 (Linux)<br>COMx (Windows) | Config/pairing commands | usb_cdc.c | No |
| CDC-ACM 1 | /dev/ttyACM1 (Linux)<br>COMx+1 (Windows) | Debug logs + Flashing | usb_console.c + esptool | Yes |

### Functionality

**CDC Port 0 (Commands):**
- Handles: `ping`, `pair`, `config`, etc.
- Implementation: `components/usb_cdc/usb_cdc.c`
- Protocol: Line-based text commands
- No DTR/RTS control

**CDC Port 1 (Debug + Flash):**
- Handles: ESP_LOG* output (dual with UART)
- Implementation: `components/usb_cdc/usb_console.c`
- DTR/RTS control: Enables bootloader reset for `make flash`
- Firmware flashing: esptool.py can use this port

### Usage

```bash
# Linux - Monitor debug console
screen /dev/ttyACM1 115200

# Linux - Send config commands
echo "ping" > /dev/ttyACM0
cat /dev/ttyACM0

# Flash firmware via USB CDC port 1
make flash BOARD=heltec  # Auto-detects and uses CDC port 1

# Windows - Use two separate COM ports in terminal software
# COM3 = Commands, COM4 = Debug/Flash
```

## Release Builds

When `LORACUE_RELEASE` is defined:

- **HID Keyboard** - Presentation control
- **CDC-ACM Port 0** - Application interface only

Debug console is **disabled** in release builds. Use UART for debugging if needed.

## Implementation Details

### Descriptors
- **File**: `components/usb_hid/usb_descriptors.c`
- **Config**: `CONFIG_TINYUSB_CDC_COUNT=2` in sdkconfig

### Endpoints

| Interface | EP IN | EP OUT | EP Notify | String ID |
|-----------|-------|--------|-----------|-----------|
| CDC-ACM 0 | 0x82 | 0x02 | 0x81 | 4 |
| CDC-ACM 1 | 0x85 | 0x03 | 0x84 | 6 |
| HID | 0x83 | - | - | 5 |

### DTR/RTS Bootloader Reset

CDC Port 1 implements DTR/RTS control for automatic bootloader entry:
- **DTR=low, RTS=high** → Enter bootloader mode
- **DTR=high, RTS=low** → Normal reset
- Implementation: `tud_cdc_line_state_cb()` in `usb_console.c`

This allows `esptool.py` (used by `make flash`) to automatically reset the ESP32 into bootloader mode without pressing buttons.

## Building

```bash
# Development build (dual CDC)
make build BOARD=heltec

# Release build (single CDC)
make build BOARD=heltec EXTRA_CFLAGS="-DLORACUE_RELEASE"
```

## Verification

After flashing, check USB enumeration:

```bash
# Linux
lsusb -v | grep -A 10 "LoRaCue"
ls -l /dev/ttyACM*

# macOS
system_profiler SPUSBDataType | grep -A 10 "LoRaCue"
ls -l /dev/cu.usbmodem*

# Windows
mode  # Lists COM ports
```

You should see:
- 1 HID device (keyboard)
- 2 CDC-ACM serial ports (in dev builds)
- 1 CDC-ACM serial port (in release builds)

