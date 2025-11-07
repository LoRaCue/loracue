#!/usr/bin/env python3
"""Generate manifest.json for firmware release."""

import sys
import json
import hashlib
import base64
from pathlib import Path
from datetime import datetime, timezone
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.backends import default_backend


def calculate_sha256(file_path: Path) -> str:
    """Calculate SHA-256 hash of a file."""
    sha256 = hashlib.sha256()
    with open(file_path, 'rb') as f:
        for chunk in iter(lambda: f.read(8192), b''):
            sha256.update(chunk)
    return sha256.hexdigest()


def sign_data(data: str, private_key_path: Path) -> str:
    """Sign data and return base64-encoded signature."""
    with open(private_key_path, 'rb') as f:
        private_key = serialization.load_pem_private_key(
            f.read(),
            password=None,
            backend=default_backend()
        )
    
    signature = private_key.sign(
        data.encode('utf-8'),
        padding.PKCS1v15(),
        hashes.SHA256()
    )
    
    return base64.b64encode(signature).decode('ascii')


def main():
    if len(sys.argv) != 12:
        print(f"Usage: {sys.argv[0]} <version> <release_type> <commit_sha> <tag_name> "
              f"<model> <board_id> <firmware_bin> <bootloader_bin> <partition_bin> <webui_bin> <private_key>")
        sys.exit(1)
    
    version = sys.argv[1]
    release_type = sys.argv[2]
    commit_sha = sys.argv[3]
    tag_name = sys.argv[4]
    model = sys.argv[5]
    board_id = sys.argv[6]
    firmware_bin = Path(sys.argv[7])
    bootloader_bin = Path(sys.argv[8])
    partition_bin = Path(sys.argv[9])
    webui_bin = Path(sys.argv[10])
    private_key = Path(sys.argv[11])
    
    # Determine board display name and flash size from board_id
    board_info = {
        "heltec_v3": {
            "display_name": "Heltec LoRa V3",
            "flash_size": "8MB"
        },
        "lilygo_t5": {
            "display_name": "LilyGO T5 Pro",
            "flash_size": "16MB"
        }
    }
    
    board_data = board_info.get(board_id, {
        "display_name": board_id,
        "flash_size": "8MB"
    })
    
    manifest = {
        "model": model,
        "board_id": board_id,
        "board_name": board_data["display_name"],
        "version": version,
        "build_date": datetime.now(timezone.utc).strftime("%Y-%m-%d"),
        "commit": commit_sha[:7],
        "target": "esp32s3",
        "flash_size": board_data["flash_size"],
        "firmware": {
            "file": "firmware.bin",
            "size": firmware_bin.stat().st_size,
            "sha256": calculate_sha256(firmware_bin),
            "offset": "0x10000"
        },
        "bootloader": {
            "file": "bootloader.bin",
            "size": bootloader_bin.stat().st_size,
            "sha256": calculate_sha256(bootloader_bin),
            "offset": "0x0"
        },
        "partition_table": {
            "file": "partition-table.bin",
            "size": partition_bin.stat().st_size,
            "sha256": calculate_sha256(partition_bin),
            "offset": "0x8000"
        },
        "webui": {
            "file": "webui-littlefs.bin",
            "size": webui_bin.stat().st_size,
            "sha256": calculate_sha256(webui_bin),
            "offset": "0x310000"
        },
        "esptool_args": [
            "--chip", "esp32s3",
            "--baud", "460800",
            "write_flash",
            "--flash_mode", "dio",
            "--flash_size", board_data["flash_size"],
            "--flash_freq", "80m",
            "0x0", "bootloader.bin",
            "0x8000", "partition-table.bin",
            "0x10000", "firmware.bin",
            "0x310000", "webui-littlefs.bin"
        ]
    }
    
    # Write manifest.json (without signature)
    manifest_file = Path("manifest.json")
    with open(manifest_file, 'w') as f:
        json.dump(manifest, f, indent=2)
    
    # Sign the manifest file
    with open(manifest_file, 'r') as f:
        manifest_content = f.read()
    
    signature = sign_data(manifest_content, private_key)
    
    # Write signature to separate file
    signature_file = Path("manifest.json.sig")
    with open(signature_file, 'w') as f:
        f.write(signature)
    
    print(f"✓ Manifest generated: {model} ({board_id})")
    print(f"  Version: {version}")
    print(f"  Board: {board_data['display_name']}")
    print(f"  Firmware: {firmware_bin.stat().st_size} bytes")
    print(f"  Manifest: {manifest_file}")
    print(f"  Signature: {signature_file}")


if __name__ == '__main__':
    main()
