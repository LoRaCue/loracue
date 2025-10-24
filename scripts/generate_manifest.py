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
    if len(sys.argv) != 10:
        print(f"Usage: {sys.argv[0]} <version> <release_type> <commit_sha> <tag_name> "
              f"<board_id> <firmware_bin> <bootloader_bin> <partition_bin> <private_key>")
        sys.exit(1)
    
    version = sys.argv[1]
    release_type = sys.argv[2]
    commit_sha = sys.argv[3]
    tag_name = sys.argv[4]
    board_id = sys.argv[5]
    firmware_bin = Path(sys.argv[6])
    bootloader_bin = Path(sys.argv[7])
    partition_bin = Path(sys.argv[8])
    private_key = Path(sys.argv[9])
    
    base_url = f"https://github.com/LoRaCue/loracue/releases/download/{tag_name}"
    
    manifest = {
        "schema_version": "1.0.0",
        "version": version,
        "release_type": release_type,
        "published_at": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z'),
        "commit_sha": commit_sha,
        "tag_name": tag_name,
        "boards": [
            {
                "board_id": board_id,
                "display_name": "Heltec LoRa V3",
                "target": "esp32s3",
                "description": "ESP32-S3 with SX1262 LoRa (868/915MHz) and SSD1306 OLED",
                "files": {
                    "firmware": {
                        "name": firmware_bin.name,
                        "size": firmware_bin.stat().st_size,
                        "sha256": calculate_sha256(firmware_bin),
                        "signature": Path(str(firmware_bin) + '.sig').read_text().strip(),
                        "url": f"{base_url}/{firmware_bin.name}"
                    },
                    "bootloader": {
                        "name": bootloader_bin.name,
                        "size": bootloader_bin.stat().st_size,
                        "sha256": calculate_sha256(bootloader_bin),
                        "signature": Path(str(bootloader_bin) + '.sig').read_text().strip(),
                        "url": f"{base_url}/{bootloader_bin.name}"
                    },
                    "partition_table": {
                        "name": partition_bin.name,
                        "size": partition_bin.stat().st_size,
                        "sha256": calculate_sha256(partition_bin),
                        "signature": Path(str(partition_bin) + '.sig').read_text().strip(),
                        "url": f"{base_url}/{partition_bin.name}"
                    }
                },
                "flash_config": {
                    "flash_mode": "dio",
                    "flash_freq": "80m",
                    "flash_size": "8MB",
                    "offsets": {
                        "bootloader": "0x0",
                        "partition_table": "0x8000",
                        "firmware": "0x10000"
                    }
                }
            }
        ],
        "changelog": {
            "features": [],
            "fixes": [],
            "breaking_changes": [],
            "performance": []
        },
        "min_manager_version": "1.0.0"
    }
    
    manifest_json = json.dumps(manifest, indent=2, sort_keys=True)
    signature = sign_data(manifest_json, private_key)
    
    manifest["signature"] = signature
    
    output_file = Path("manifest.json")
    with open(output_file, 'w') as f:
        json.dump(manifest, f, indent=2)
    
    print(f"Manifest written to: {output_file}")


if __name__ == '__main__':
    main()
