#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FONT_DIR="$SCRIPT_DIR/../../assets/fonts/sf-pro/font/woff2"
OUTPUT_DIR="$SCRIPT_DIR/fonts"
TEMP_DIR="$OUTPUT_DIR/temp"

# Check if lv_font_conv is available
if ! command -v lv_font_conv &> /dev/null; then
    echo "lv_font_conv not found. Installing..."
    npm install -g lv_font_conv
fi

# Check if woff2_decompress is available
if ! command -v woff2_decompress &> /dev/null; then
    echo "woff2_decompress not found. Please install woff2 tools:"
    echo "  macOS: brew install woff2"
    echo "  Linux: sudo apt-get install woff2"
    exit 1
fi

# Create directories
mkdir -p "$OUTPUT_DIR"
mkdir -p "$TEMP_DIR"

echo "Converting woff2 to ttf..."

# Convert woff2 to ttf
woff2_decompress "$FONT_DIR/sf-pro-display_regular.woff2"
woff2_decompress "$FONT_DIR/sf-pro-display_bold.woff2"

# Move to temp directory
mv "$FONT_DIR/sf-pro-display_regular.ttf" "$TEMP_DIR/"
mv "$FONT_DIR/sf-pro-display_bold.ttf" "$TEMP_DIR/"

echo "Generating SF Pro fonts for LVGL..."

# Generate SF Pro Display Regular 16px
npx lv_font_conv \
    --font "$TEMP_DIR/sf-pro-display_regular.ttf" \
    -r 0x20-0x7E \
    --size 16 \
    --format lvgl \
    --bpp 4 \
    --no-compress \
    -o "$OUTPUT_DIR/sf_pro_16.c"

# Generate SF Pro Display Regular 24px
npx lv_font_conv \
    --font "$TEMP_DIR/sf-pro-display_regular.ttf" \
    -r 0x20-0x7E \
    --size 24 \
    --format lvgl \
    --bpp 4 \
    --no-compress \
    -o "$OUTPUT_DIR/sf_pro_24.c"

# Generate SF Pro Display Bold 24px
npx lv_font_conv \
    --font "$TEMP_DIR/sf-pro-display_bold.ttf" \
    -r 0x20-0x7E \
    --size 24 \
    --format lvgl \
    --bpp 4 \
    --no-compress \
    -o "$OUTPUT_DIR/sf_pro_bold_24.c"

# Clean up temp files
rm -rf "$TEMP_DIR"

echo "Font generation complete!"
