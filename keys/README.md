# Firmware Signing Keys

## Ed25519 Keys (Current)

**Public Key:** `firmware_public_ed25519.pem` (committed to repo)
- Embedded in firmware as `firmware_public_ed25519.bin` (32 bytes)
- Used to verify OTA firmware signatures

**Private Key:** `firmware_private_ed25519.pem` (gitignored, stored in GitHub Secrets)
- Used in CI/CD to sign firmware releases
- **Never commit this file!**

### Key Generation

```bash
# Generate new Ed25519 key pair
openssl genpkey -algorithm ED25519 -out firmware_private_ed25519.pem
openssl pkey -in firmware_private_ed25519.pem -pubout -out firmware_public_ed25519.pem

# Extract raw public key for embedding (32 bytes)
openssl pkey -in firmware_public_ed25519.pem -pubin -outform DER | tail -c 32 > firmware_public_ed25519.bin
```

### Signing Firmware

```bash
# Sign a firmware binary
python3 scripts/sign_firmware.py build/loracue.bin keys/firmware_private_ed25519.pem

# Output: 128 hex characters (64-byte signature)
```

### Testing

```bash
# Verify signing works
python3 scripts/test_ed25519_signing.py
```

## GitHub Secrets

Store the private key in GitHub repository secrets:

1. Go to repository Settings → Secrets and variables → Actions
2. Add secret: `FIRMWARE_SIGNING_KEY`
3. Paste contents of `firmware_private_ed25519.pem`

The CI/CD workflow will use this to sign releases.

## Security Notes

- **Private keys are never committed to the repository**
- Public key is embedded in firmware at build time
- Signatures are verified during OTA updates
- Ed25519 provides 128-bit security (equivalent to RSA-3072)
- Signature size: 64 bytes
- Verification time: ~0.5ms
