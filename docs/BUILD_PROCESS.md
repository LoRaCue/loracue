# LoRaCue Build Process & Model Architecture

## Product Models

LoRaCue has **3 models**, each with a specific hardware configuration:

| Model | Board | Hardware | Description |
|-------|-------|----------|-------------|
| **LC-Alpha** | Heltec V3 | 1 onboard button | Prototype with single button |
| **LC-Alpha+** | Heltec V3 | 2 separate buttons | Enhanced prototype with prev/next buttons |
| **LC-Gamma** | LilyGO T5 Pro | E-paper + touch | Professional with 4.7" display |

**Key Design:** Model implicitly defines the board. No separate board selection needed.

---

## Kconfig Configuration

**File:** `Kconfig.projbuild`

```kconfig
menu "LoRaCue Configuration"

    choice LORACUE_MODEL
        prompt "Product Model"
        default MODEL_LC_ALPHA
        help
            Select the LoRaCue product model.
            Each model has a specific hardware configuration.
        
        config MODEL_LC_ALPHA
            bool "LC-Alpha (Heltec V3, 1 button)"
            select BOARD_HELTEC_V3
            select UI_MINI
            help
                Prototype model with Heltec LoRa V3 board.
                Uses single onboard button (GPIO0).
        
        config MODEL_LC_ALPHA_PLUS
            bool "LC-Alpha+ (Heltec V3, 2 buttons)"
            select BOARD_HELTEC_V3
            select UI_MINI
            help
                Enhanced prototype with Heltec LoRa V3 board.
                Uses two separate external buttons (prev/next).
        
        config MODEL_LC_GAMMA
            bool "LC-Gamma (LilyGO T5 Pro, E-paper)"
            select BOARD_LILYGO_T5
            select UI_RICH
            help
                Professional model with LilyGO T5 4.7" E-paper display.
                Features touch interface and rich UI.
    endchoice

    config LORACUE_MODEL_NAME
        string
        default "LC-Alpha" if MODEL_LC_ALPHA
        default "LC-Alpha+" if MODEL_LC_ALPHA_PLUS
        default "LC-Gamma" if MODEL_LC_GAMMA

    # Board selection (implicit, set by model)
    config BOARD_HELTEC_V3
        bool
    
    config BOARD_LILYGO_T5
        bool

    config LORACUE_BOARD_ID
        string
        default "heltec_v3" if BOARD_HELTEC_V3
        default "lilygo_t5" if BOARD_LILYGO_T5

    # UI selection (implicit, set by model)
    config UI_MINI
        bool
    
    config UI_RICH
        bool

endmenu
```

---

## Makefile

```makefile
# Usage: make build              (defaults to alpha)
#        make build MODEL=alpha+
#        make build MODEL=gamma
MODEL ?= alpha

ifeq ($(MODEL),alpha)
MODEL_NAME = LC-Alpha
BOARD_NAME = Heltec V3
SDKCONFIG = sdkconfig.defaults sdkconfig.heltec_v3 sdkconfig.model_alpha
else ifeq ($(MODEL),alpha+)
MODEL_NAME = LC-Alpha+
BOARD_NAME = Heltec V3
SDKCONFIG = sdkconfig.defaults sdkconfig.heltec_v3 sdkconfig.model_alpha_plus
else ifeq ($(MODEL),gamma)
MODEL_NAME = LC-Gamma
BOARD_NAME = LilyGO T5 Pro
SDKCONFIG = sdkconfig.defaults sdkconfig.lilygo_t5 sdkconfig.model_gamma
else
$(error Invalid MODEL=$(MODEL). Use: alpha, alpha+, or gamma)
endif

build:
	@echo "🔨 Building $(MODEL_NAME) ($(BOARD_NAME))..."
	@rm -f sdkconfig
	@cat $(SDKCONFIG) > sdkconfig
	idf.py build
```

---

## GitHub Actions Build Matrix

**File:** `.github/workflows/release.yml`

```yaml
build-matrix:
  strategy:
    fail-fast: false
    matrix:
      model:
        - name: "alpha"
          display_name: "LC-Alpha"
          board: "heltec_v3"
          board_name: "Heltec LoRa V3"
          target: "esp32s3"
          description: "Prototype with single onboard button"
          flash_size: "8MB"
          sdkconfig: "sdkconfig.defaults sdkconfig.heltec_v3 sdkconfig.model_alpha"
          
        - name: "alpha-plus"
          display_name: "LC-Alpha+"
          board: "heltec_v3"
          board_name: "Heltec LoRa V3"
          target: "esp32s3"
          description: "Enhanced prototype with two separate buttons"
          flash_size: "8MB"
          sdkconfig: "sdkconfig.defaults sdkconfig.heltec_v3 sdkconfig.model_alpha_plus"
          
        - name: "gamma"
          display_name: "LC-Gamma"
          board: "lilygo_t5"
          board_name: "LilyGO T5 Pro"
          target: "esp32s3"
          description: "Professional with 4.7\" E-paper display and touch"
          flash_size: "16MB"
          sdkconfig: "sdkconfig.defaults sdkconfig.lilygo_t5 sdkconfig.model_gamma"
  
  name: Build ${{ matrix.model.display_name }}
  
  steps:
    - name: Build firmware
      run: |
        idf.py -D SDKCONFIG_DEFAULTS="${{ matrix.model.sdkconfig }}" build
```

---

## Release Artifacts

### Package Structure

```
release/
├── loracue-lc-alpha-v1.0.0.zip
├── loracue-lc-alpha-plus-v1.0.0.zip
├── loracue-lc-gamma-v1.0.0.zip
└── manifests.json
```

### Firmware Package Contents

```
loracue-lc-alpha/
├── firmware.bin
├── bootloader.bin
├── partition-table.bin
├── ota_data_initial.bin
├── manifest.json
├── README.md
└── checksums.txt
```

---

## Manifest Files

### manifest.json (Single Model)

```json
{
  "model": "LC-Alpha",
  "board_id": "heltec_v3",
  "board_name": "Heltec LoRa V3",
  "version": "1.0.0",
  "build_date": "2025-11-06",
  "commit": "abc1234",
  "target": "esp32s3",
  "flash_size": "8MB",
  "firmware": {
    "file": "firmware.bin",
    "size": 1357376,
    "sha256": "a1b2c3d4...",
    "offset": "0x30000"
  },
  "bootloader": {
    "file": "bootloader.bin",
    "size": 21088,
    "sha256": "e5f6g7h8...",
    "offset": "0x0"
  },
  "partition_table": {
    "file": "partition-table.bin",
    "size": 3072,
    "sha256": "i9j0k1l2...",
    "offset": "0x8000"
  },
  "ota_data": {
    "file": "ota_data_initial.bin",
    "size": 8192,
    "sha256": "m3n4o5p6...",
    "offset": "0x630000"
  },
  "esptool_args": [
    "--chip", "esp32s3",
    "--baud", "460800",
    "write_flash",
    "--flash_mode", "dio",
    "--flash_size", "8MB",
    "--flash_freq", "80m",
    "0x0", "bootloader.bin",
    "0x8000", "partition-table.bin",
    "0x30000", "firmware.bin",
    "0x630000", "ota_data_initial.bin"
  ]
}
```

### manifests.json (All Models)

```json
[
  {
    "model": "LC-Alpha",
    "board_id": "heltec_v3",
    "version": "1.0.0",
    "download_url": "https://github.com/.../loracue-lc-alpha-v1.0.0.zip",
    ...
  },
  {
    "model": "LC-Alpha+",
    "board_id": "heltec_v3",
    "version": "1.0.0",
    "download_url": "https://github.com/.../loracue-lc-alpha-plus-v1.0.0.zip",
    ...
  },
  {
    "model": "LC-Gamma",
    "board_id": "lilygo_t5",
    "version": "1.0.0",
    "download_url": "https://github.com/.../loracue-lc-gamma-v1.0.0.zip",
    ...
  }
]
```

---

## Device Identification

### device:info Response

```json
{
  "board_id": "heltec_v3",
  "model": "LC-Alpha+",
  "version": "1.0.0",
  "mac": "aa:bb:cc:dd:ee:ff"
}
```

### Firmware Update Matching

LoRaCue Manager matches by **model** (board_id is implicit):

```python
device_model = "LC-Alpha+"  # From device:info

# Find matching firmware
for manifest in manifests:
    if manifest["model"] == device_model:
        download_url = manifest["download_url"]
        break
```

---

## File Structure

```
LoRaCue/
├── sdkconfig.defaults          # Common settings
├── sdkconfig.heltec_v3         # Heltec V3 board settings
├── sdkconfig.lilygo_t5         # LilyGO T5 board settings
├── sdkconfig.model_alpha       # LC-Alpha model settings
├── sdkconfig.model_alpha_plus  # LC-Alpha+ model settings
├── sdkconfig.model_gamma       # LC-Gamma model settings
└── Kconfig.projbuild           # Model selection
```

---

## Model-Specific Features

### LC-Alpha (sdkconfig.model_alpha)
```
CONFIG_MODEL_LC_ALPHA=y
CONFIG_BUTTON_MODE_SINGLE=y
CONFIG_BUTTON_GPIO=0
```

### LC-Alpha+ (sdkconfig.model_alpha_plus)
```
CONFIG_MODEL_LC_ALPHA_PLUS=y
CONFIG_BUTTON_MODE_DUAL=y
CONFIG_BUTTON_PREV_GPIO=12
CONFIG_BUTTON_NEXT_GPIO=13
```

### LC-Gamma (sdkconfig.model_gamma)
```
CONFIG_MODEL_LC_GAMMA=y
CONFIG_TOUCH_ENABLED=y
CONFIG_EINK_DISPLAY=y
```

---

## Build Commands

```bash
# Local development (defaults to alpha)
make build              # LC-Alpha (default)
make build MODEL=alpha  # LC-Alpha (explicit)
make build MODEL=alpha+ # LC-Alpha+
make build MODEL=gamma  # LC-Gamma

# Flash
make flash              # LC-Alpha (default)
make flash MODEL=alpha+ # LC-Alpha+

# CI/CD
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.heltec_v3;sdkconfig.model_alpha" build
```

---

## Summary

**3 Models = 3 Builds:**
- LC-Alpha (Heltec V3, 1 button)
- LC-Alpha+ (Heltec V3, 2 buttons)
- LC-Gamma (LilyGO T5 Pro, E-paper)

**Key Benefits:**
✅ Simple: Model selection only  
✅ Clear: Each model has specific hardware  
✅ Safe: Can't build invalid combinations  
✅ Scalable: Easy to add new models  
✅ Efficient: 3 builds per release (not 9)


### Build Matrix (Board-Based)

Currently, firmware is built per **board** (hardware platform):

```yaml
matrix:
  board:
    - name: "heltec_v3"
      display_name: "Heltec LoRa V3"
      target: "esp32s3"
      sdkconfig: "sdkconfig.heltec_v3"
    - name: "lilygo_t5"
      display_name: "LilyGO T5"
      target: "esp32s3"
      sdkconfig: "sdkconfig.lilygo_t5"
```

**Problem:** No distinction between **hardware** (board) and **product** (model).

---

## Proposed: Model + Board Architecture

### Concept

- **Board** = Hardware platform (Heltec V3, LilyGO T5, custom PCB)
- **Model** = Product variant (LC-Alpha, LC-One, LC-Pro)
- **Build** = Board × Model combination

### Example Combinations

| Model | Board | Use Case |
|-------|-------|----------|
| LC-Alpha | Heltec V3 | Prototype/development |
| LC-One | Heltec V3 | Production v1 (using dev board) |
| LC-One | Custom PCB | Production v1 (final hardware) |
| LC-Pro | LilyGO T5 | Professional with e-paper |

---

## Implementation Plan

### 1. Kconfig Configuration

**File:** `Kconfig.projbuild`

```kconfig
menu "LoRaCue Configuration"

    choice LORACUE_MODEL
        prompt "Product Model"
        default MODEL_LC_ALPHA
        help
            Select the product model/variant.
        
        config MODEL_LC_ALPHA
            bool "LC-Alpha (Prototype)"
        
        config MODEL_LC_ONE
            bool "LC-One (Production)"
        
        config MODEL_LC_PRO
            bool "LC-Pro (Professional)"
    endchoice

    config LORACUE_MODEL_NAME
        string
        default "LC-Alpha" if MODEL_LC_ALPHA
        default "LC-One" if MODEL_LC_ONE
        default "LC-Pro" if MODEL_LC_PRO

    choice LORACUE_BOARD
        prompt "Target Board"
        default BOARD_HELTEC_V3
        
        config BOARD_HELTEC_V3
            bool "Heltec WiFi LoRa 32 V3"
            select UI_MINI
        
        config BOARD_LILYGO_T5
            bool "LilyGO T5 4.7\" E-Paper"
            select UI_RICH
        
        config BOARD_CUSTOM_PCB
            bool "Custom LoRaCue PCB"
            select UI_MINI
    endchoice

    config LORACUE_BOARD_ID
        string
        default "heltec_v3" if BOARD_HELTEC_V3
        default "lilygo_t5" if BOARD_LILYGO_T5
        default "custom_pcb" if BOARD_CUSTOM_PCB

endmenu
```

### 2. Makefile Updates

```makefile
# Usage: make build BOARD=heltec MODEL=alpha
#        make build BOARD=custom MODEL=one
BOARD ?= heltec
MODEL ?= alpha

# Board mapping
ifeq ($(BOARD),heltec)
BOARD_TARGET = heltec_v3
BOARD_NAME = Heltec V3
SDKCONFIG_BOARD = sdkconfig.heltec_v3
else ifeq ($(BOARD),lilygo)
BOARD_TARGET = lilygo_t5
BOARD_NAME = LilyGO T5
SDKCONFIG_BOARD = sdkconfig.lilygo_t5
else ifeq ($(BOARD),custom)
BOARD_TARGET = custom_pcb
BOARD_NAME = Custom PCB
SDKCONFIG_BOARD = sdkconfig.custom_pcb
endif

# Model mapping
ifeq ($(MODEL),alpha)
MODEL_NAME = LC-Alpha
SDKCONFIG_MODEL = sdkconfig.model_alpha
else ifeq ($(MODEL),one)
MODEL_NAME = LC-One
SDKCONFIG_MODEL = sdkconfig.model_one
else ifeq ($(MODEL),pro)
MODEL_NAME = LC-Pro
SDKCONFIG_MODEL = sdkconfig.model_pro
endif

build:
 @echo "🔨 Building $(MODEL_NAME) for $(BOARD_NAME)..."
 @rm -f sdkconfig
 @cat sdkconfig.defaults $(SDKCONFIG_BOARD) $(SDKCONFIG_MODEL) > sdkconfig.tmp
 @mv sdkconfig.tmp sdkconfig
 idf.py build
```

### 3. GitHub Actions Build Matrix

**File:** `.github/workflows/release.yml`

```yaml
build-matrix:
  strategy:
    matrix:
      include:
        # LC-Alpha (Prototype) - Heltec V3 only
        - model: "alpha"
          model_name: "LC-Alpha"
          board: "heltec_v3"
          board_name: "Heltec LoRa V3"
          target: "esp32s3"
          sdkconfig: "sdkconfig.defaults sdkconfig.heltec_v3 sdkconfig.model_alpha"
          
        # LC-One (Production) - Multiple boards
        - model: "one"
          model_name: "LC-One"
          board: "heltec_v3"
          board_name: "Heltec LoRa V3"
          target: "esp32s3"
          sdkconfig: "sdkconfig.defaults sdkconfig.heltec_v3 sdkconfig.model_one"
          
        - model: "one"
          model_name: "LC-One"
          board: "custom_pcb"
          board_name: "Custom PCB"
          target: "esp32s3"
          sdkconfig: "sdkconfig.defaults sdkconfig.custom_pcb sdkconfig.model_one"
          
        # LC-Pro (Professional) - LilyGO T5
        - model: "pro"
          model_name: "LC-Pro"
          board: "lilygo_t5"
          board_name: "LilyGO T5"
          target: "esp32s3"
          sdkconfig: "sdkconfig.defaults sdkconfig.lilygo_t5 sdkconfig.model_pro"
  
  name: Build ${{ matrix.model_name }} (${{ matrix.board_name }})
```

---

## Release Artifacts

### Directory Structure

```
release/
├── loracue-lc-alpha-heltec_v3-v1.0.0.zip
├── loracue-lc-one-heltec_v3-v1.0.0.zip
├── loracue-lc-one-custom_pcb-v1.0.0.zip
├── loracue-lc-pro-lilygo_t5-v1.0.0.zip
└── manifests.json
```

### Firmware Package Contents

Each `.zip` file contains:

```
loracue-lc-one-heltec_v3/
├── firmware.bin           # Main application binary
├── bootloader.bin         # ESP32 bootloader
├── partition-table.bin    # Partition layout
├── ota_data_initial.bin   # OTA data partition
├── manifest.json          # Single board manifest
├── README.md              # Installation instructions
└── checksums.txt          # SHA256 checksums
```

---

## Manifest Files

### manifest.json (Single Board)

**Purpose:** Metadata for ONE firmware build (model + board combination)

**Location:** Inside each firmware `.zip` package

**Structure:**

```json
{
  "model": "LC-One",
  "board_id": "heltec_v3",
  "board_name": "Heltec LoRa V3",
  "version": "1.0.0",
  "build_date": "2025-11-06",
  "commit": "abc1234",
  "target": "esp32s3",
  "flash_size": "8MB",
  "firmware": {
    "file": "firmware.bin",
    "size": 1357376,
    "sha256": "a1b2c3d4...",
    "offset": "0x30000"
  },
  "bootloader": {
    "file": "bootloader.bin",
    "size": 21088,
    "sha256": "e5f6g7h8...",
    "offset": "0x0"
  },
  "partition_table": {
    "file": "partition-table.bin",
    "size": 3072,
    "sha256": "i9j0k1l2...",
    "offset": "0x8000"
  },
  "ota_data": {
    "file": "ota_data_initial.bin",
    "size": 8192,
    "sha256": "m3n4o5p6...",
    "offset": "0x630000"
  },
  "esptool_args": [
    "--chip", "esp32s3",
    "--baud", "460800",
    "write_flash",
    "--flash_mode", "dio",
    "--flash_size", "8MB",
    "--flash_freq", "80m",
    "0x0", "bootloader.bin",
    "0x8000", "partition-table.bin",
    "0x30000", "firmware.bin",
    "0x630000", "ota_data_initial.bin"
  ]
}
```

**Used by:**

- LoRaCue Manager (desktop app) for flashing
- Automated testing tools
- Manufacturing tools

---

### manifests.json (All Boards)

**Purpose:** Combined metadata for ALL firmware builds in a release

**Location:** Root of release assets (alongside `.zip` files)

**Structure:**

```json
[
  {
    "model": "LC-Alpha",
    "board_id": "heltec_v3",
    "version": "1.0.0",
    "download_url": "https://github.com/.../loracue-lc-alpha-heltec_v3-v1.0.0.zip",
    ...
  },
  {
    "model": "LC-One",
    "board_id": "heltec_v3",
    "version": "1.0.0",
    "download_url": "https://github.com/.../loracue-lc-one-heltec_v3-v1.0.0.zip",
    ...
  },
  {
    "model": "LC-One",
    "board_id": "custom_pcb",
    "version": "1.0.0",
    "download_url": "https://github.com/.../loracue-lc-one-custom_pcb-v1.0.0.zip",
    ...
  },
  {
    "model": "LC-Pro",
    "board_id": "lilygo_t5",
    "version": "1.0.0",
    "download_url": "https://github.com/.../loracue-lc-pro-lilygo_t5-v1.0.0.zip",
    ...
  }
]
```

**Used by:**

- LoRaCue Manager for update checking
- Web-based firmware selector
- CI/CD validation
- Release notes generation

**How it's created:**

```python
# In release.yml after all builds complete
manifests = []
for manifest_file in glob.glob("firmware-packages/*-manifest-*/manifest.json"):
    manifest = json.load(open(manifest_file))
    manifest["download_url"] = f"https://github.com/.../loracue-{manifest['model'].lower()}-{manifest['board_id']}-v{version}.zip"
    manifests.append(manifest)

with open("manifests.json", 'w') as f:
    json.dump(manifests, f, indent=2)
```

---

## Device Identification

### At Runtime

**device:info JSON-RPC response:**

```json
{
  "board_id": "heltec_v3",      // From bsp_get_board_id()
  "model": "LC-One",             // From CONFIG_LORACUE_MODEL_NAME
  "version": "1.0.0",
  "mac": "aa:bb:cc:dd:ee:ff"
}
```

### Firmware Update Matching

LoRaCue Manager checks compatibility:

```python
# Device reports
device_board_id = "heltec_v3"
device_model = "LC-One"

# Find matching firmware in manifests.json
for manifest in manifests:
    if manifest["board_id"] == device_board_id and \
       manifest["model"] == device_model:
        # Compatible firmware found
        download_url = manifest["download_url"]
        break
```

---

## Migration Path

### Phase 1: Add Model Support (Non-Breaking)

1. Add `LORACUE_MODEL` to Kconfig
2. Update `device:info` to include model field
3. Keep single board builds (alpha only)

### Phase 2: Multi-Model Builds

1. Add model-specific sdkconfig files
2. Update build matrix to include model dimension
3. Generate separate packages per model+board

### Phase 3: Custom PCB Support

1. Add `BOARD_CUSTOM_PCB` to Kconfig
2. Create BSP implementation for custom hardware
3. Add to build matrix

---

## Benefits

✅ **Clear separation:** Hardware (board) vs Product (model)  
✅ **Flexible manufacturing:** Same model on different boards  
✅ **Feature differentiation:** Model-specific features via Kconfig  
✅ **Proper identification:** Device reports both board and model  
✅ **Update safety:** Firmware updates match both board AND model  
✅ **Scalability:** Easy to add new models or boards  

---

## Example Build Commands

```bash
# Development
make build BOARD=heltec MODEL=alpha

# Production
make build BOARD=custom MODEL=one

# Professional
make build BOARD=lilygo MODEL=pro

# CI/CD
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.heltec_v3;sdkconfig.model_one" build
```

---

## File Structure

```
LoRaCue/
├── sdkconfig.defaults          # Common settings
├── sdkconfig.heltec_v3         # Board-specific
├── sdkconfig.lilygo_t5         # Board-specific
├── sdkconfig.custom_pcb        # Board-specific
├── sdkconfig.model_alpha       # Model-specific
├── sdkconfig.model_one         # Model-specific
├── sdkconfig.model_pro         # Model-specific
└── Kconfig.projbuild           # Configuration options
```

---

## Summary

**Current:** Board-only builds (heltec_v3, lilygo_t5)  
**Proposed:** Model × Board matrix (LC-Alpha/heltec_v3, LC-One/custom_pcb, etc.)  

**Key Changes:**

1. Add model dimension to build system
2. Generate separate packages per model+board combination
3. Update manifest.json to include model field
4. Create manifests.json with all combinations
5. Device reports both board_id and model at runtime
