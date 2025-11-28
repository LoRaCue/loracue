# OTA Engine

Secure firmware update engine with SHA256 integrity verification and Ed25519 signature verification framework.

## Security Features

### ✅ SHA256 Integrity Verification (Fully Implemented)
- Validates firmware hasn't been corrupted during transfer
- Incremental hash calculation during `ota_engine_write()`
- Verification on `ota_engine_finish()` before committing update
- Rejects firmware if hash mismatch detected

### ⚠️ Ed25519 Signature Verification (Framework Ready)
- **Status**: Format validation implemented, cryptographic verification pending
- **Current**: Validates signature format (128 hex chars) and rejects all-zero signatures
- **Missing**: Actual Ed25519 cryptographic verification

## Why Signature Verification is Incomplete

ESP-IDF's mbedtls **does not include Ed25519 support**. To complete signature verification, add one of these libraries:

### Option 1: libsodium (Recommended)
```yaml
# main/idf_component.yml
dependencies:
  espressif/libsodium: "^1.0.18"
```

**Implementation:**
```c
// In ota_engine_finish()
if (crypto_sign_verify_detached(signature_bin, hash_bin, 32, public_key) != 0) {
    ESP_LOGE(TAG, "Ed25519 signature verification FAILED!");
    return ESP_ERR_INVALID_RESPONSE;
}
```

**Pros:**
- Battle-tested (used by Signal, WhatsApp, Tor)
- Simple API (one function call)
- Constant-time operations (timing attack resistant)
- ~50KB flash footprint

**Cons:**
- ESP-IDF component has build issues (as of v1.0.20)
- Requires manual patching or older version

### Option 2: TweetNaCl
Minimal Ed25519 implementation (~100 lines of C code).

**Pros:**
- Tiny footprint (~15KB)
- No dependencies
- Easy to integrate

**Cons:**
- Less audited than libsodium
- Manual integration required

### Option 3: micro-ecc
Already included in ESP-IDF for ECDSA.

**Pros:**
- Already in ESP-IDF
- Small footprint (~20KB)

**Cons:**
- Complex API
- Requires Ed25519 curve configuration

## Current Security Level

**With SHA256 only:**
- ✅ Protects against accidental corruption
- ✅ Protects against transmission errors
- ❌ Does NOT protect against malicious firmware
- ❌ Does NOT verify firmware authenticity

**With SHA256 + Ed25519 (when completed):**
- ✅ Full integrity protection
- ✅ Authenticity verification
- ✅ Protection against malicious firmware
- ✅ Production-ready security

## Usage

```c
// Set expected SHA256 hash
ota_engine_set_expected_sha256("abc123...");

// Set expected Ed25519 signature (128 hex chars)
ota_engine_verify_signature("def456...");

// Start OTA
ota_engine_start(firmware_size);

// Write firmware data
ota_engine_write(data, len);

// Finish and verify
esp_err_t ret = ota_engine_finish();
if (ret == ESP_ERR_INVALID_CRC) {
    // SHA256 mismatch
} else if (ret == ESP_ERR_INVALID_RESPONSE) {
    // Signature verification failed
}
```

## Testing

```bash
# Test Ed25519 signing (Python)
python3 scripts/test_ed25519_signing.py

# Sign firmware
python3 scripts/sign_firmware.py build/loracue.bin keys/firmware_private_ed25519.pem
```

## Production Deployment

1. Add libsodium or TweetNaCl to project
2. Replace placeholder verification in `ota_engine_finish()`
3. Test with signed firmware
4. Deploy with confidence

## References

- [libsodium documentation](https://doc.libsodium.org/)
- [Ed25519 specification](https://ed25519.cr.yp.to/)
- [TweetNaCl](https://tweetnacl.cr.yp.to/)
