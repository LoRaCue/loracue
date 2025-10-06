#!/usr/bin/env python3
"""Check firmware manifest in ESP-IDF binary"""

import struct
import sys
import os

def check_firmware(bin_path):
    if not os.path.exists(bin_path):
        print(f"✗ File not found: {bin_path}")
        return False
    
    with open(bin_path, 'rb') as f:
        # ESP-IDF app descriptor at offset 0x20
        f.seek(0x20)
        data = f.read(256)
        
        # Parse esp_app_desc_t structure (offsets from start of structure)
        # uint32_t magic_word;        // 0
        # uint32_t secure_version;    // 4
        # uint32_t reserv1[2];        // 8
        # char version[32];           // 16
        # char project_name[32];      // 48
        # char time[16];              // 80
        # char date[16];              // 96
        # char idf_ver[32];           // 112
        
        magic = struct.unpack('<I', data[0:4])[0]
        secure_version = struct.unpack('<I', data[4:8])[0]
        version = data[16:48].decode('utf-8', errors='ignore').rstrip('\x00')
        project_name = data[48:80].decode('utf-8', errors='ignore').rstrip('\x00')
        time = data[80:96].decode('utf-8', errors='ignore').rstrip('\x00')
        date = data[96:112].decode('utf-8', errors='ignore').rstrip('\x00')
        idf_ver = data[112:144].decode('utf-8', errors='ignore').rstrip('\x00')
        
        # Validate
        valid = magic == 0xABCD5432
        
        print(f"📦 Firmware: {os.path.basename(bin_path)}")
        print(f"   Magic:        0x{magic:08X} {'✓' if valid else '✗ INVALID'}")
        print(f"   Board ID:     {project_name}")
        print(f"   Version:      {version}")
        print(f"   Secure Ver:   {secure_version}")
        print(f"   Build Date:   {date}")
        print(f"   Build Time:   {time}")
        print(f"   IDF Version:  {idf_ver}")
        
        return valid

if __name__ == '__main__':
    bin_file = sys.argv[1] if len(sys.argv) > 1 else 'build/heltec_v3.bin'
    sys.exit(0 if check_firmware(bin_file) else 1)
