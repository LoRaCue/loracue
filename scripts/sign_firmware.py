#!/usr/bin/env python3
"""Sign firmware files with RSA-4096 private key."""

import sys
import base64
from pathlib import Path
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.backends import default_backend


def sign_file(file_path: Path, private_key_path: Path) -> str:
    """Sign a file and return base64-encoded signature."""
    with open(private_key_path, 'rb') as f:
        private_key = serialization.load_pem_private_key(
            f.read(),
            password=None,
            backend=default_backend()
        )
    
    with open(file_path, 'rb') as f:
        data = f.read()
    
    signature = private_key.sign(
        data,
        padding.PKCS1v15(),
        hashes.SHA256()
    )
    
    return base64.b64encode(signature).decode('ascii')


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <file_to_sign> <private_key_pem>")
        sys.exit(1)
    
    file_path = Path(sys.argv[1])
    private_key_path = Path(sys.argv[2])
    
    if not file_path.exists():
        print(f"Error: File not found: {file_path}")
        sys.exit(1)
    
    if not private_key_path.exists():
        print(f"Error: Private key not found: {private_key_path}")
        sys.exit(1)
    
    signature = sign_file(file_path, private_key_path)
    
    sig_file = file_path.with_suffix(file_path.suffix + '.sig')
    with open(sig_file, 'w') as f:
        f.write(signature)
    
    print(f"Signature written to: {sig_file}")


if __name__ == '__main__':
    main()
