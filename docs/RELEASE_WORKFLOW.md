# LoRaCue Release Workflow

Complete documentation for the secure firmware release and distribution system.

## 🏗️ Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     GitHub Actions (CI/CD)                       │
│  ┌────────────┐  ┌──────────────┐  ┌─────────────────────┐    │
│  │   Build    │→ │ Sign with    │→ │ Generate Manifest   │    │
│  │  Firmware  │  │ RSA Private  │  │   (manifest.json)   │    │
│  └────────────┘  └──────────────┘  └─────────────────────┘    │
│         ↓                                      ↓                 │
│  ┌────────────────────────────────────────────────────────┐    │
│  │         Upload to GitHub Release Assets                 │    │
│  │  • heltec_v3.bin + .sig + .sha256                      │    │
│  │  • bootloader.bin + .sig + .sha256                     │    │
│  │  • partition-table.bin + .sig + .sha256                │    │
│  │  • manifest.json + .sig                                │    │
│  └────────────────────────────────────────────────────────┘    │
│         ↓                                                        │
│  ┌────────────────────────────────────────────────────────┐    │
│  │    Trigger Webhook to release.loracue.com Repo          │    │
│  └────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│         LoRaCue/release.loracue.com (Private Repo)               │
│  ┌────────────────────────────────────────────────────────┐    │
│  │  GitHub Actions: Update releases.json                  │    │
│  │  1. Fetch all GitHub releases via API                  │    │
│  │  2. Download manifest.json for each release            │    │
│  │  3. Aggregate into releases.json                       │    │
│  │  4. Sign releases.json with RSA private key            │    │
│  │  5. Commit to repo                                     │    │
│  └────────────────────────────────────────────────────────┘    │
│         ↓                                                        │
│  ┌────────────────────────────────────────────────────────┐    │
│  │           Vercel Auto-Deploy                            │    │
│  │  • Serves releases.json at release.loracue.com          │    │
│  │  • CDN distribution (global edge network)              │    │
│  │  • HTTPS with custom domain                            │    │
│  └────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    LoRaCue Manager (Client)                      │
│  ┌────────────────────────────────────────────────────────┐    │
│  │  1. Fetch https://release.loracue.com/releases.json     │    │
│  │  2. Verify signature with embedded public key          │    │
│  │  3. Display available versions (stable/prerelease)     │    │
│  │  4. User selects version                               │    │
│  │  5. Download manifest.json from GitHub Release         │    │
│  │  6. Verify manifest signature                          │    │
│  │  7. Download firmware files from GitHub Release        │    │
│  │  8. Verify file signatures                             │    │
│  │  9. Flash to device                                    │    │
│  └────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

## 🔐 Security Model

### RSA Key Pair

**Generation (One-time Setup):**
```bash
# Generate 4096-bit RSA private key
openssl genrsa -out firmware_private.pem 4096

# Extract public key
openssl rsa -in firmware_private.pem -pubout -out firmware_public.pem

# Store private key in GitHub Secrets (both repos)
# Commit public key to both repos
```

**Key Storage:**
- **Private Key**: GitHub Actions Secret `FIRMWARE_SIGNING_KEY` (both repos)
- **Public Key**: 
  - `keys/firmware_public.pem` in LoRaCue/loracue repo
  - `keys/firmware_public.pem` in LoRaCue/release.loracue.com repo
  - Embedded in LoRaCue Manager binary

**Signature Algorithm:**
- RSA-4096 with SHA-256
- PKCS#1 v1.5 padding
- Base64-encoded signatures in JSON

### Trust Chain

```
Root of Trust: RSA Public Key (embedded in Manager)
    ↓
releases.json signature verification
    ↓
manifest.json signature verification
    ↓
Individual file signature verification
    ↓
Flash to device
```

## 📋 File Formats

### releases.json

**Location:** `https://release.loracue.com/releases.json`

**Structure:**
```json
{
  "schema_version": "1.0.0",
  "generated_at": "2025-10-22T16:07:00Z",
  "latest_stable": "0.1.0",
  "latest_prerelease": "0.2.0-alpha.201",
  "releases": [
    {
      "version": "0.2.0-alpha.201",
      "release_type": "prerelease",
      "published_at": "2025-10-22T15:59:00Z",
      "commit_sha": "6458caf",
      "tag_name": "v0.2.0-alpha.201",
      "manifest_url": "https://github.com/LoRaCue/loracue/releases/download/v0.2.0-alpha.201/manifest.json",
      "manifest_sha256": "abc123...",
      "manifest_signature": "def456...",
      "supported_boards": ["heltec_v3"],
      "changelog_summary": "Bug fixes and performance improvements"
    },
    {
      "version": "0.1.0",
      "release_type": "release",
      "published_at": "2025-10-15T10:00:00Z",
      "commit_sha": "abc1234",
      "tag_name": "v0.1.0",
      "manifest_url": "https://github.com/LoRaCue/loracue/releases/download/v0.1.0/manifest.json",
      "manifest_sha256": "ghi789...",
      "manifest_signature": "jkl012...",
      "supported_boards": ["heltec_v3"],
      "changelog_summary": "Initial stable release"
    }
  ],
  "signature": "mno345..."
}
```

**Signature Calculation:**
```bash
# Sign the entire JSON (excluding signature field)
echo -n '{"schema_version":"1.0.0",...}' | \
  openssl dgst -sha256 -sign firmware_private.pem | \
  base64 -w 0
```

### manifest.json

**Location:** GitHub Release Assets (per version)

**Structure:**
```json
{
  "schema_version": "1.0.0",
  "version": "0.2.0-alpha.201",
  "release_type": "prerelease",
  "published_at": "2025-10-22T15:59:00Z",
  "commit_sha": "6458caf",
  "tag_name": "v0.2.0-alpha.201",
  "boards": [
    {
      "board_id": "heltec_v3",
      "display_name": "Heltec LoRa V3",
      "target": "esp32s3",
      "description": "ESP32-S3 with SX1262 LoRa (868/915MHz) and SSD1306 OLED",
      "files": {
        "firmware": {
          "name": "heltec_v3.bin",
          "size": 1615520,
          "sha256": "abc123...",
          "signature": "def456...",
          "url": "https://github.com/LoRaCue/loracue/releases/download/v0.2.0-alpha.201/heltec_v3.bin"
        },
        "bootloader": {
          "name": "bootloader.bin",
          "size": 28672,
          "sha256": "ghi789...",
          "signature": "jkl012...",
          "url": "https://github.com/LoRaCue/loracue/releases/download/v0.2.0-alpha.201/bootloader.bin"
        },
        "partition_table": {
          "name": "partition-table.bin",
          "size": 3072,
          "sha256": "mno345...",
          "signature": "pqr678...",
          "url": "https://github.com/LoRaCue/loracue/releases/download/v0.2.0-alpha.201/partition-table.bin"
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
    "features": [
      "Event-driven ACK handling",
      "Keycode-to-name lookup table"
    ],
    "fixes": [
      "Duplicate keyboard transmission bug",
      "Device name display on boot"
    ],
    "breaking_changes": [],
    "performance": [
      "HMAC context reuse (~1ms)",
      "Interrupt-driven TX (~0.5-1ms)"
    ]
  },
  "min_manager_version": "1.0.0",
  "signature": "stu901..."
}
```

## 🔄 Workflow Steps

### 1. Firmware Build & Release (LoRaCue/loracue)

**Trigger:** Push tag `v*` or manual workflow dispatch

**Jobs:**

#### Job: version
- Calculate version with GitVersion
- Create/validate tag

#### Job: build-matrix
- Build firmware for each board
- Extract firmware manifest (board ID, version)
- Calculate SHA-256 checksums
- **Sign binaries with RSA private key**
- Generate manifest.json
- **Sign manifest.json**
- Upload artifacts

#### Job: generate-changelog
- Parse commits since last tag
- Generate structured changelog

#### Job: create-release
- Download all artifacts
- Create GitHub Release
- Upload binaries + signatures + checksums + manifest.json
- **Trigger webhook to release.loracue.com repo**

**New Files to Upload:**
```
heltec_v3.bin
heltec_v3.bin.sig          # NEW
heltec_v3.bin.sha256
bootloader.bin
bootloader.bin.sig         # NEW
bootloader.bin.sha256
partition-table.bin
partition-table.bin.sig    # NEW
partition-table.bin.sha256
manifest.json              # NEW
manifest.json.sig          # NEW
README.md
```

### 2. Release Index Update (LoRaCue/release.loracue.com)

**Trigger:** Repository dispatch webhook from loracue repo

**Jobs:**

#### Job: update-releases-index
1. Fetch all releases from LoRaCue/loracue via GitHub API
2. For each release:
   - Download manifest.json
   - Verify manifest signature
   - Extract metadata (version, boards, changelog summary)
3. Generate releases.json with aggregated data
4. Sign releases.json with RSA private key
5. Commit to repo (triggers Vercel deploy)

**Repository Structure:**
```
LoRaCue/release.loracue.com/
├── .github/
│   └── workflows/
│       └── update-index.yml
├── keys/
│   └── firmware_public.pem
├── scripts/
│   └── generate_releases_index.py
├── public/
│   └── releases.json          # Generated file
├── vercel.json
└── README.md
```

**vercel.json:**
```json
{
  "version": 2,
  "public": true,
  "headers": [
    {
      "source": "/releases.json",
      "headers": [
        {
          "key": "Cache-Control",
          "value": "public, max-age=300, s-maxage=300"
        },
        {
          "key": "Access-Control-Allow-Origin",
          "value": "*"
        }
      ]
    }
  ]
}
```

### 3. Vercel Deployment

**Setup:**
1. Connect LoRaCue/release.loracue.com to Vercel
2. Configure custom domain: `release.loracue.com`
3. Enable automatic deployments on push to main

**Deployment:**
- Triggered automatically on commit to main
- Serves `public/releases.json` at root
- Global CDN distribution
- HTTPS by default

## 🔧 Implementation Tasks

### Phase 1: Core Infrastructure ✅ COMPLETED

**LoRaCue/loracue Repository:**
- [x] Generate RSA key pair
- [ ] **MANUAL**: Store private key in GitHub Secrets (`FIRMWARE_SIGNING_KEY`)
- [x] Commit public key to `keys/firmware_public.pem`
- [x] Create `scripts/sign_firmware.py`
- [x] Create `scripts/generate_manifest.py`
- [x] Update `.github/workflows/release.yml`:
  - [x] Add signing step after build
  - [x] Add manifest generation step
  - [x] Add repository dispatch trigger
- [ ] **MANUAL**: Create GitHub Secret `RELEASE_REPO_TOKEN` for webhook

**LoRaCue/release.loracue.com Repository:**
- [ ] **MANUAL**: Create private repository on GitHub
- [ ] **MANUAL**: Push files from `/tmp/release.loracue.com/` to repository
- [ ] **MANUAL**: Add RSA private key to GitHub Secrets (`FIRMWARE_SIGNING_KEY`)
- [x] Public key ready at `keys/firmware_public.pem`
- [x] Created `scripts/generate_releases_index.py`
- [x] Created `.github/workflows/update-index.yml`
- [x] Created `vercel.json` configuration
- [ ] **MANUAL**: Connect to Vercel (see `/tmp/release.loracue.com/SETUP.md`)
- [ ] **MANUAL**: Configure custom domain `release.loracue.com`

**Implementation Files Ready:**
- ✅ All code and scripts implemented
- ✅ Workflows configured
- ✅ Documentation complete
- ⏳ Awaiting manual GitHub/Vercel setup steps

**Setup Instructions:**
See `/tmp/release.loracue.com/SETUP.md` for complete step-by-step guide.

### Phase 2: LoRaCue Manager Integration

- [ ] Embed public key in Manager binary
- [ ] Implement releases.json fetching
- [ ] Implement signature verification
- [ ] Implement manifest.json fetching
- [ ] Implement file download with progress
- [ ] Implement board ID matching
- [ ] Implement flashing logic

### Phase 3: Testing & Documentation

- [ ] Test complete workflow end-to-end
- [ ] Document key rotation procedure
- [ ] Create troubleshooting guide
- [ ] Add monitoring/alerting for failed releases

## 🔒 Security Considerations

### Key Management

**Private Key Protection:**
- Never commit private key to repository
- Store only in GitHub Actions Secrets
- Rotate key annually or on compromise
- Use separate keys for production/staging

**Key Rotation Procedure:**
1. Generate new key pair
2. Update GitHub Secrets in both repos
3. Commit new public key
4. Release new Manager version with new public key
5. Keep old key for 6 months (backward compatibility)
6. Deprecate old key after grace period

### Attack Mitigation

**Man-in-the-Middle:**
- ✅ HTTPS for all downloads
- ✅ Signature verification at every level
- ✅ Pinned public key in Manager

**Replay Attacks:**
- ✅ Version monotonicity checks
- ✅ Timestamp validation
- ✅ Board ID matching

**Compromised Release Server:**
- ✅ Signatures prevent tampering
- ✅ Manager validates against embedded public key
- ✅ GitHub Release assets are immutable

**Supply Chain Attacks:**
- ✅ Signed commits (optional)
- ✅ Reproducible builds (future)
- ✅ SBOM generation (future)

## 📊 Monitoring & Metrics

**Track:**
- Release success/failure rate
- Signature verification failures
- Download counts per version
- Manager version distribution
- Board model distribution

**Alerts:**
- Failed release builds
- Signature verification errors
- Vercel deployment failures
- Unusual download patterns

## 🚀 Rollout Plan

**Week 1:**
- Setup infrastructure (repos, keys, Vercel)
- Implement signing in release workflow
- Test with pre-release builds

**Week 2:**
- Implement releases.json generation
- Deploy to Vercel
- Verify CDN distribution

**Week 3:**
- Integrate with LoRaCue Manager
- Internal testing
- Documentation

**Week 4:**
- Beta release to select users
- Monitor metrics
- Fix issues

**Week 5:**
- Public release
- Announce new update system
- Deprecate old manual flashing docs

## 📚 References

- [Secure Boot Best Practices](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/security/secure-boot-v2.html)
- [Firmware Update Security](https://www.rfc-editor.org/rfc/rfc9019.html)
- [NIST Guidelines for Firmware Updates](https://csrc.nist.gov/publications/detail/sp/800-147b/final)
- [Vercel Documentation](https://vercel.com/docs)
- [GitHub Actions Security](https://docs.github.com/en/actions/security-guides)
