#!/usr/bin/env python3
"""Convert PNG to LVGL 4-bit grayscale C array"""

import sys
import os

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow not installed. Run: pip3 install Pillow")
    sys.exit(1)

if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} <input.png> <output.c>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]
name = os.path.splitext(os.path.basename(output_file))[0]

# Load and convert to grayscale
img = Image.open(input_file).convert('L')
w, h = img.size
pixels = list(img.getdata())

# Convert to 4-bit (16 levels)
pixels_4bit = [p >> 4 for p in pixels]

# Pack 2 pixels per byte
data = bytearray()
for i in range(0, len(pixels_4bit), 2):
    if i + 1 < len(pixels_4bit):
        data.append((pixels_4bit[i] << 4) | pixels_4bit[i+1])
    else:
        data.append(pixels_4bit[i] << 4)

# Write C file
with open(output_file, 'w') as f:
    f.write('#include "lvgl.h"\n\n')
    f.write(f'const uint8_t {name}_map[] = {{\n')
    for i in range(0, len(data), 16):
        line = ', '.join(f'0x{b:02x}' for b in data[i:i+16])
        f.write(f'    {line},\n')
    f.write('};\n\n')
    f.write(f'const lv_img_dsc_t {name} = {{\n')
    f.write('    .header.cf = LV_IMG_CF_INDEXED_4BIT,\n')
    f.write(f'    .header.w = {w},\n')
    f.write(f'    .header.h = {h},\n')
    f.write(f'    .data_size = {len(data)},\n')
    f.write(f'    .data = {name}_map,\n')
    f.write('};\n')

# Write header
with open(output_file.replace('.c', '.h'), 'w') as f:
    f.write('#pragma once\n#include "lvgl.h"\n\n')
    f.write(f'extern const lv_img_dsc_t {name};\n')

print(f"✓ {w}x{h} → {len(data)} bytes ({output_file})")
