# Release Workflow Verification

## ✅ Updated Release Workflow

The GitHub Actions release workflow now correctly builds firmware for **both** boards:

### Build Matrix

```yaml
matrix:
  board:
    - name: "heltec_v3"
      display_name: "Heltec LoRa V3"
      sdkconfig: "sdkconfig.heltec_v3"
      flash_size: "8MB"
      
    - name: "lilygo_t5"
      display_name: "LilyGO T5 4.7\" E-Paper"
      sdkconfig: "sdkconfig.lilygo_t5"
      flash_size: "16MB"
```

## 📦 Generated Artifacts

For **each board**, the workflow generates:

### 1. Firmware Binaries
- `{board}.bin` - Main firmware
- `bootloader.bin` - ESP32 bootloader
- `partition-table.bin` - Partition table

### 2. Checksums
- `{board}.bin.sha256`
- `bootloader.bin.sha256`
- `partition-table.bin.sha256`

### 3. Signatures (RSA-4096)
- `{board}.bin.sig`
- `bootloader.bin.sig`
- `partition-table.bin.sig`

### 4. Manifest
- `manifest.json` - Contains:
  - Board ID validation
  - Firmware version
  - SHA256 checksums
  - RSA signature
  - Build metadata

### 5. Release Package
- `loracue-{board}-v{version}.zip` containing all above files
- `README.md` with installation instructions

## 🔐 Security Features

### Signing Process
1. All binaries signed with RSA-4096 private key (from GitHub Secrets)
2. Signatures stored as `.sig` files
3. Manifest includes signature for verification

### Board ID Validation
- Firmware contains embedded board ID at offset 0x20
- Manifest validates board ID matches expected value
- Prevents flashing wrong firmware to wrong board

### Integrity Verification
- SHA256 checksums for all binaries
- Checksums included in manifest
- LoRaCue Manager verifies before flashing

## 📋 Release Assets

Each release includes:

```
loracue-heltec_v3-v1.0.0.zip
├── heltec_v3.bin
├── heltec_v3.bin.sha256
├── heltec_v3.bin.sig
├── bootloader.bin
├── bootloader.bin.sha256
├── bootloader.bin.sig
├── partition-table.bin
├── partition-table.bin.sha256
├── partition-table.bin.sig
├── manifest.json
└── README.md

loracue-lilygo_t5-v1.0.0.zip
├── lilygo_t5.bin
├── lilygo_t5.bin.sha256
├── lilygo_t5.bin.sig
├── bootloader.bin
├── bootloader.bin.sha256
├── bootloader.bin.sig
├── partition-table.bin
├── partition-table.bin.sha256
├── partition-table.bin.sig
├── manifest.json
└── README.md
```

## 🚀 Build Process

### 1. Version Calculation
- Uses GitVersion for semantic versioning
- Supports manual tags
- Validates tag format

### 2. Matrix Build
- Parallel builds for both boards
- Uses board-specific sdkconfig files
- Independent failure handling

### 3. Firmware Validation
- Extracts ESP-IDF app descriptor
- Validates board ID matches matrix
- Verifies firmware version

### 4. Signing & Packaging
- Signs all binaries with private key
- Generates checksums
- Creates manifest with metadata
- Packages into ZIP files

### 5. Release Creation
- Creates GitHub release
- Uploads all firmware packages
- Generates changelog from commits
- Includes breaking changes and known issues

## 🔄 Workflow Triggers

### Automatic (Tag Push)
```bash
git tag v1.0.0
git push origin v1.0.0
```

### Manual (Workflow Dispatch)
- Go to Actions → Release LoRaCue Firmware → Run workflow
- Optional: Set pre-release flag
- Optional: Override version tag

## ✅ Verification Checklist

Before release, verify:

- [ ] Both board sdkconfig files exist
- [ ] Board names match in matrix and sdkconfig
- [ ] Signing key secret is configured
- [ ] Build succeeds for both boards
- [ ] Manifest contains correct board IDs
- [ ] ZIP files contain all required files
- [ ] Signatures are valid
- [ ] Checksums match binaries

## 📊 Release Notes

Release notes automatically include:

- **Features** (feat: commits)
- **Bug Fixes** (fix: commits)
- **Performance** (perf: commits)
- **Documentation** (docs: commits)
- **Breaking Changes** (BREAKING CHANGE: or !)
- **Known Issues** (from KNOWN_ISSUES.md)

## 🔗 Integration

The workflow triggers:
- **release.loracue.com** index update
- Notifies LoRaCue Manager of new release
- Updates firmware catalog

## 🛠️ Troubleshooting

### Build Fails for One Board
- Check sdkconfig file exists
- Verify board-specific components compile
- Check CMakeLists.txt board selection logic

### Signing Fails
- Verify `FIRMWARE_SIGNING_KEY` secret is set
- Check private key format (PEM)
- Ensure key has correct permissions

### Manifest Validation Fails
- Check board ID in firmware matches matrix
- Verify binary name matches board name
- Ensure app descriptor is at offset 0x20

## 📚 Related Documentation

- [Board Support](BOARDS.md)
- [Build System](../Makefile)
- [Signing Scripts](../scripts/)
