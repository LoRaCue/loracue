#!/bin/bash
set -e

VERSION="$1"
BOARD_NAME="$2"
TARGET="$3"
DESCRIPTION="$4"
COMMIT="$5"
BOARD_ID="$6"
FW_VERSION="$7"
SHA256="$8"
BINARY_NAME="$9"
OUTPUT_FILE="${10}"

BUILD_DATE=$(date -u +"%Y-%m-%d %H:%M:%S UTC")

cat > "$OUTPUT_FILE" << 'EOF'
# LoRaCue Firmware v__VERSION__

**Board**: __BOARD_NAME__  
**Target**: __TARGET__  
**Description**: __DESCRIPTION__  
**Build Date**: __BUILD_DATE__  
**Commit**: __COMMIT__

## 📋 Package Contents

- `__BINARY_NAME__` - Main firmware binary
- `bootloader.bin` - ESP32 bootloader
- `partition-table.bin` - Partition table
- `manifest.json` - Firmware manifest with metadata
- `*.sha256` - SHA256 checksums for verification
- `*.sig` - RSA-4096 signatures for authenticity verification

## 🔐 Firmware Verification

**Board ID**: `__BOARD_ID__`  
**Firmware Version**: `__FW_VERSION__`  
**SHA256**: `__SHA256__`

All binaries are signed with RSA-4096 for authenticity verification.

## 📥 Installation

Use **[LoRaCue Manager](https://github.com/LoRaCue/loracue-manager)** to flash this firmware.

LoRaCue Manager provides:
- ✅ Automatic device detection
- ✅ One-click firmware updates  
- ✅ Integrity verification
- ✅ Cross-platform support (Windows, macOS, Linux)

## ✅ Post-Flash Verification

After flashing, the device should:
1. Display LoRaCue logo on OLED
2. Status LED breathing effect
3. Buttons make LED solid when pressed
4. Serial output shows firmware version

## 🔄 OTA Updates

This firmware supports OTA updates via:
- USB CDC commands
- WiFi web interface

**Compatibility**: Only accepts firmware with matching board ID (`__BOARD_ID__`)

## 📚 Documentation

- **Firmware Repository**: https://github.com/LoRaCue/loracue
- **Manager Tool**: https://github.com/LoRaCue/loracue-manager
- **Issues**: https://github.com/LoRaCue/loracue/issues
- **Wiki**: https://github.com/LoRaCue/loracue/wiki

## 🏙️ Made with ❤️ in Hannover 🇩🇪
EOF

# Replace placeholders
sed -i.bak "s|__VERSION__|$VERSION|g" "$OUTPUT_FILE"
sed -i.bak "s|__BOARD_NAME__|$BOARD_NAME|g" "$OUTPUT_FILE"
sed -i.bak "s|__TARGET__|$TARGET|g" "$OUTPUT_FILE"
sed -i.bak "s|__DESCRIPTION__|$DESCRIPTION|g" "$OUTPUT_FILE"
sed -i.bak "s|__BUILD_DATE__|$BUILD_DATE|g" "$OUTPUT_FILE"
sed -i.bak "s|__COMMIT__|$COMMIT|g" "$OUTPUT_FILE"
sed -i.bak "s|__BINARY_NAME__|$BINARY_NAME|g" "$OUTPUT_FILE"
sed -i.bak "s|__BOARD_ID__|$BOARD_ID|g" "$OUTPUT_FILE"
sed -i.bak "s|__FW_VERSION__|$FW_VERSION|g" "$OUTPUT_FILE"
sed -i.bak "s|__SHA256__|$SHA256|g" "$OUTPUT_FILE"
rm -f "$OUTPUT_FILE.bak"
