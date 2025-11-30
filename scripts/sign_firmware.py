#!/usr/bin/env python3
"""Sign firmware files with Ed25519 private key."""

import sys
import hashlib
from pathlib import Path
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ed25519
from cryptography.hazmat.backends import default_backend


def sign_file(file_path: Path, private_key_path: Path) -> str:
    """Sign a file's SHA256 hash and return hex-encoded signature."""
    with open(private_key_path, 'rb') as f:
        private_key = serialization.load_pem_private_key(
            f.read(),
            password=None,
            backend=default_backend()
        )
    
    if not isinstance(private_key, ed25519.Ed25519PrivateKey):
        raise ValueError(f"Expected Ed25519 key, got {type(private_key).__name__}")
    
    # Calculate SHA256 hash of the file
    with open(file_path, 'rb') as f:
        data = f.read()
    
    file_hash = hashlib.sha256(data).digest()
    
    # Ed25519 signs the hash directly
    signature = private_key.sign(file_hash)
    
    return signature.hex()


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
