#!/usr/bin/env python3
"""Test Ed25519 firmware signing and verification."""

import hashlib
from pathlib import Path
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend

def test_signing():
    """Test Ed25519 signing with the generated keys."""
    keys_dir = Path(__file__).parent.parent / "keys"
    
    # Load keys
    with open(keys_dir / "firmware_private_ed25519.pem", 'rb') as f:
        private_key = serialization.load_pem_private_key(f.read(), password=None, backend=default_backend())
    
    with open(keys_dir / "firmware_public_ed25519.pem", 'rb') as f:
        public_key = serialization.load_pem_public_key(f.read(), backend=default_backend())
    
    # Test data (simulate firmware hash)
    test_data = b"Test firmware data"
    file_hash = hashlib.sha256(test_data).digest()
    
    # Sign
    signature = private_key.sign(file_hash)
    print(f"✓ Signature created: {len(signature)} bytes")
    print(f"  Hex: {signature.hex()[:32]}...")
    
    # Verify
    try:
        public_key.verify(signature, file_hash)
        print("✓ Signature verification passed!")
        return True
    except Exception as e:
        print(f"✗ Signature verification failed: {e}")
        return False

if __name__ == "__main__":
    print("Testing Ed25519 firmware signing...")
    print()
    success = test_signing()
    print()
    if success:
        print("✅ Ed25519 signing test passed!")
        exit(0)
    else:
        print("❌ Ed25519 signing test failed!")
        exit(1)
