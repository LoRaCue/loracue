#!/usr/bin/env python3
"""Generate manifest.json for firmware release."""

import sys
import json
import hashlib
import base64
from pathlib import Path
from datetime import datetime, timezone
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ed25519
from cryptography.hazmat.backends import default_backend


def calculate_sha256(file_path: Path) -> str:
    """Calculate SHA-256 hash of a file."""
    sha256 = hashlib.sha256()
    with open(file_path, 'rb') as f:
        for chunk in iter(lambda: f.read(8192), b''):
            sha256.update(chunk)
    return sha256.hexdigest()


def sign_data(data: str, private_key_path: Path) -> str:
    """Sign data with Ed25519 and return base64-encoded signature."""
    with open(private_key_path, 'rb') as f:
        private_key = serialization.load_pem_private_key(
            f.read(),
            password=None,
            backend=default_backend()
        )
    
    if not isinstance(private_key, ed25519.Ed25519PrivateKey):
        raise ValueError(f"Expected Ed25519 key, got {type(private_key).__name__}")
    
    data_bytes = data.encode('utf-8')
    signature = private_key.sign(data_bytes)
    
    return base64.b64encode(signature).decode('ascii')


# Flash partition layout
PARTITIONS = [
    {"name": "bootloader", "offset": "0x0", "file": "bootloader.bin"},
    {"name": "partition_table", "offset": "0x8000", "file": "partition-table.bin"},
    {"name": "firmware", "offset": "0x10000", "file": "firmware.bin"},
    {"name": "webui", "offset": "0x640000", "file": "webui-littlefs.bin"},
]

# Board configurations
BOARDS = {
    "heltec_v3": {"display_name": "Heltec LoRa V3", "flash_size": "8MB"},
    "lilygo_t5": {"display_name": "LilyGO T5 Pro", "flash_size": "16MB"},
}


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
    
    # Map CLI args to partition files
    binary_files = {
        "firmware.bin": Path(sys.argv[7]),
        "bootloader.bin": Path(sys.argv[8]),
        "partition-table.bin": Path(sys.argv[9]),
        "webui-littlefs.bin": Path(sys.argv[10]),
    }
    private_key = Path(sys.argv[11])
    
    # Get board configuration
    board_data = BOARDS.get(board_id, {"display_name": board_id, "flash_size": "8MB"})
    
    # Build manifest dynamically from partition layout
    manifest = {
        "model": model,
        "board_id": board_id,
        "board_name": board_data["display_name"],
        "version": version,
        "build_date": datetime.now(timezone.utc).strftime("%Y-%m-%d"),
        "commit": commit_sha[:7],
        "target": "esp32s3",
        "flash_size": board_data["flash_size"],
    }
    
    # Add partition entries
    esptool_args = ["--chip", "esp32s3", "--baud", "460800", "write_flash",
                    "--flash_mode", "dio", "--flash_size", board_data["flash_size"], "--flash_freq", "80m"]
    
    for partition in PARTITIONS:
        file_path = binary_files[partition["file"]]
        manifest[partition["name"]] = {
            "file": partition["file"],
            "size": file_path.stat().st_size,
            "sha256": calculate_sha256(file_path),
            "offset": partition["offset"]
        }
        esptool_args.extend([partition["offset"], partition["file"]])
    
    manifest["esptool_args"] = esptool_args
    
    # Write manifest.json
    manifest_file = Path("manifest.json")
    with open(manifest_file, 'w') as f:
        json.dump(manifest, f, indent=2)
    
    # Sign manifest
    with open(manifest_file, 'r') as f:
        signature = sign_data(f.read(), private_key)
    
    # Write signature
    signature_file = Path("manifest.json.sig")
    with open(signature_file, 'w') as f:
        f.write(signature)
    
    print(f"✓ Manifest generated: {model} ({board_id})")
    print(f"  Version: {version}")
    print(f"  Board: {board_data['display_name']}")
    print(f"  Firmware: {binary_files['firmware.bin'].stat().st_size} bytes")
    print(f"  Manifest: {manifest_file}")
    print(f"  Signature: {signature_file}")


if __name__ == '__main__':
    main()
