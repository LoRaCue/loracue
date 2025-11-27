# LoRaCue Makefile with ESP-IDF Auto-Detection and Wokwi Simulator

.PHONY: all build clean fullclean rebuild flash flash-monitor monitor menuconfig size erase set-target format format-check lint test test-device test-build sim sim-run sim-debug chips web-dev web-build web-flash help check-idf assets ui_compact_assets fonts-1bpp fonts-4bpp fonts-8bpp fonts-all

# WASI Toolchain Detection (for Wokwi chips)
WASI_SYSROOT := $(shell find /opt/homebrew/Cellar/wasi-libc -name wasi-sysroot 2>/dev/null | head -1)
CLANG := $(shell test -f /opt/homebrew/opt/llvm/bin/clang && echo /opt/homebrew/opt/llvm/bin/clang || which clang 2>/dev/null)
WASM_LD := $(shell test -f /opt/homebrew/opt/llvm/bin/wasm-ld && echo /opt/homebrew/opt/llvm/bin/wasm-ld || which wasm-ld 2>/dev/null)

# ESP-IDF Detection Logic
IDF_PATH_CANDIDATES := \
	$(HOME)/esp-idf \
	$(HOME)/esp-idf-v5.5 \
	$(HOME)/esp/esp-idf \
	/opt/esp-idf \
	/usr/local/esp-idf

# Find ESP-IDF installation
define find_idf_path
$(foreach path,$(IDF_PATH_CANDIDATES),$(if $(wildcard $(path)/export.sh),$(path),))
endef

IDF_PATH_FOUND := $(word 1,$(call find_idf_path))

# Check if idf.py is already in PATH
IDF_PY_IN_PATH := $(shell which idf.py 2>/dev/null)

# Determine if we need to source export.sh
ifdef IDF_PY_IN_PATH
    IDF_SETUP := 
    IDF_VERSION := $(shell idf.py --version 2>/dev/null | grep -o 'v[0-9]\+\.[0-9]\+')
else ifdef IDF_PATH_FOUND
    IDF_SETUP := source $(IDF_PATH_FOUND)/export.sh >/dev/null 2>&1 &&
    IDF_VERSION := $(shell source $(IDF_PATH_FOUND)/export.sh >/dev/null 2>&1 && idf.py --version 2>/dev/null | grep -o 'v[0-9]\+\.[0-9]\+')
else
    IDF_SETUP := 
    IDF_VERSION := 
endif

# Wokwi CLI detection
WOKWI_CLI := $(shell which wokwi-cli 2>/dev/null)

# PNG to LVGL conversion for ui_compact
UI_COMPACT_ASSETS_DIR := components/ui_compact/assets
UI_COMPACT_GENERATED_DIR := components/ui_compact/generated
UI_COMPACT_PNGS := $(wildcard $(UI_COMPACT_ASSETS_DIR)/*.png)
UI_COMPACT_GENERATED := $(patsubst $(UI_COMPACT_ASSETS_DIR)/%.png,$(UI_COMPACT_GENERATED_DIR)/%.c,$(UI_COMPACT_PNGS))

check-png2lvgl:
ifneq ($(UI_COMPACT_PNGS),)
	@command -v png2lvgl >/dev/null 2>&1 || { \
		echo "❌ png2lvgl not found!"; \
		echo ""; \
		echo "Install png2lvgl:"; \
		echo "  git clone https://github.com/metaneutrons/png2lvgl.git"; \
		echo "  cd png2lvgl && make && sudo make install"; \
		echo ""; \
		echo "Or visit: https://github.com/metaneutrons/png2lvgl"; \
		exit 1; \
	}
endif

$(UI_COMPACT_GENERATED_DIR)/%.c: $(UI_COMPACT_ASSETS_DIR)/%.png
	@mkdir -p $(UI_COMPACT_GENERATED_DIR)
	@if [ ! -f "$@" ] || [ "$<" -nt "$@" ]; then \
		echo "🎨 Converting $< to LVGL C file..."; \
		png2lvgl --format indexed1 --overwrite -o $@ $<; \
	else \
		echo "✅ $@ is up to date"; \
	fi

ui_compact_assets: check-png2lvgl $(UI_COMPACT_GENERATED)

# Font generation for LVGL
FONT_OUTPUT_DIR := components/ui_lvgl/generated
FONT_SIZES := 8 10 12 14 16 18 20
FONT_RANGE := 0x20-0x7F
FONT_TEMP_DIR := /tmp/loracue-fonts

# Inter font from GitHub (official repository)
INTER_FONT_URL := https://github.com/rsms/inter/releases/download/v4.1/Inter-4.1.zip
INTER_FONT_LIGHT := $(FONT_TEMP_DIR)/Inter/extras/ttf/Inter-Light.ttf

# Pixolletta font from DaFont
PIXOLLETTA_FONT_URL := https://dl.dafont.com/dl/?f=pixolletta8px
PIXOLLETTA_FONT := $(FONT_TEMP_DIR)/Pixolletta8px.ttf

# Micro font from 1001fonts
MICRO_FONT_URL := https://www.1001fonts.com/download/micro.zip
MICRO_FONT := $(FONT_TEMP_DIR)/micro.ttf

check-lv-font-conv:
	@command -v npx >/dev/null 2>&1 || { \
		echo "❌ npx not found! Install Node.js first."; \
		exit 1; \
	}

# Download Inter font from GitHub
$(INTER_FONT_LIGHT):
	@echo "📥 Downloading Inter font from GitHub..."
	@mkdir -p $(FONT_TEMP_DIR)
	@curl -fL "$(INTER_FONT_URL)" -o $(FONT_TEMP_DIR)/inter.zip
	@unzip -q -o $(FONT_TEMP_DIR)/inter.zip -d $(FONT_TEMP_DIR)/Inter
	@test -f $(INTER_FONT_LIGHT) || { echo "❌ Inter-Light.ttf not found"; exit 1; }
	@echo "✅ Inter font downloaded"

# Download Pixolletta font from DaFont
$(PIXOLLETTA_FONT):
	@echo "📥 Downloading Pixolletta font from DaFont..."
	@mkdir -p $(FONT_TEMP_DIR)
	@curl -fL "$(PIXOLLETTA_FONT_URL)" -o $(FONT_TEMP_DIR)/pixolletta.zip
	@unzip -q -o $(FONT_TEMP_DIR)/pixolletta.zip -d $(FONT_TEMP_DIR)
	@test -f $(PIXOLLETTA_FONT) || { echo "❌ Pixolletta8px.ttf not found"; exit 1; }
	@echo "✅ Pixolletta font downloaded"

# Download Micro font from 1001fonts
$(MICRO_FONT):
	@echo "📥 Downloading Micro font from 1001fonts..."
	@mkdir -p $(FONT_TEMP_DIR)
	@curl -fL "$(MICRO_FONT_URL)" -o $(FONT_TEMP_DIR)/micro.zip
	@unzip -q -o $(FONT_TEMP_DIR)/micro.zip -d $(FONT_TEMP_DIR)/micro_extracted
	@find $(FONT_TEMP_DIR)/micro_extracted -name "*.ttf" -exec cp {} $(MICRO_FONT) \;
	@test -f $(MICRO_FONT) || { echo "❌ micro.ttf not found"; exit 1; }
	@echo "✅ Micro font downloaded"

# Generate 1bpp fonts for ui_compact (OLED displays) using Inter
fonts-1bpp: check-lv-font-conv $(INTER_FONT_LIGHT) $(PIXOLLETTA_FONT) $(MICRO_FONT)
	@mkdir -p $(FONT_OUTPUT_DIR)
	@if [ ! -f "$(FONT_OUTPUT_DIR)/lv_font_pixolletta_10.c" ]; then \
		echo "🔤 Generating 1bpp Inter fonts for ui_compact..."; \
		$(foreach size,$(FONT_SIZES), \
			echo "  - Inter light $(size)px (1bpp)"; \
			npx lv_font_conv --bpp 1 --format lvgl --font $(INTER_FONT_LIGHT) \
				--range $(FONT_RANGE) --size $(size) \
				--output $(FONT_OUTPUT_DIR)/lv_font_1bpp_inter_light_$(size).c;) \
		echo "🔤 Generating Pixolletta 10px (1bpp) for small screens..."; \
		npx lv_font_conv --bpp 1 --format lvgl --font $(PIXOLLETTA_FONT) \
			--range $(FONT_RANGE) --size 10 --no-compress \
			--output $(FONT_OUTPUT_DIR)/lv_font_pixolletta_10.c; \
		echo "🔤 Generating Pixolletta 20px (1bpp) for large value displays..."; \
		npx lv_font_conv --bpp 1 --format lvgl --font $(PIXOLLETTA_FONT) \
			--range $(FONT_RANGE) --size 20 --no-compress \
			--output $(FONT_OUTPUT_DIR)/lv_font_pixolletta_20.c; \
		echo "🔤 Generating Micro 5px (1bpp) for version text..."; \
		npx lv_font_conv --bpp 1 --format lvgl --font $(MICRO_FONT) \
			--range $(FONT_RANGE) --size 5 --no-compress \
			--output $(FONT_OUTPUT_DIR)/lv_font_micro_5.c; \
		echo "✅ 1bpp fonts generated"; \
	else \
		echo "✅ 1bpp fonts already exist, skipping generation"; \
	fi

# Generate 4bpp fonts for e-paper displays (16 grayscale)
fonts-4bpp: check-lv-font-conv $(INTER_FONT_LIGHT)
	@mkdir -p $(FONT_OUTPUT_DIR)
	@if [ ! -f "$(FONT_OUTPUT_DIR)/lv_font_4bpp_inter_light_8.c" ]; then \
		echo "🔤 Generating 4bpp Inter fonts for e-paper..."; \
		$(foreach size,$(FONT_SIZES), \
			echo "  - Inter light $(size)px (4bpp)"; \
			npx lv_font_conv --bpp 4 --format lvgl --font $(INTER_FONT_LIGHT) \
				--range $(FONT_RANGE) --size $(size) \
				--output $(FONT_OUTPUT_DIR)/lv_font_4bpp_inter_light_$(size).c;) \
		echo "✅ 4bpp fonts generated"; \
	else \
		echo "✅ 4bpp fonts already exist, skipping generation"; \
	fi

# Generate 8bpp fonts for color displays (256 colors)
fonts-8bpp: check-lv-font-conv $(INTER_FONT_LIGHT)
	@mkdir -p $(FONT_OUTPUT_DIR)
	@if [ ! -f "$(FONT_OUTPUT_DIR)/lv_font_8bpp_inter_light_8.c" ]; then \
		echo "🔤 Generating 8bpp Inter fonts for color displays..."; \
		$(foreach size,$(FONT_SIZES), \
			echo "  - Inter light $(size)px (8bpp)"; \
			npx lv_font_conv --bpp 8 --format lvgl --font $(INTER_FONT_LIGHT) \
				--range $(FONT_RANGE) --size $(size) \
				--output $(FONT_OUTPUT_DIR)/lv_font_8bpp_inter_light_$(size).c;) \
		echo "✅ 8bpp fonts generated"; \
	else \
		echo "✅ 8bpp fonts already exist, skipping generation"; \
	fi

# Generate all font variants
fonts-all: fonts-1bpp fonts-4bpp fonts-8bpp
	@rm -rf $(FONT_TEMP_DIR)
	@echo "✅ All fonts generated and temp files cleaned"

# Generate all assets (fonts + PNG icons)
assets: fonts-all ui_compact_assets
	@echo "✅ All assets generated"

# Default target
all: build

# Internal: Check ESP-IDF installation
check-idf:
ifndef IDF_PY_IN_PATH
ifndef IDF_PATH_FOUND
	@echo "❌ ESP-IDF not found!"
	@echo ""
	@echo "Install ESP-IDF v5.5:"
	@echo "  git clone -b v5.5 --recursive https://github.com/espressif/esp-idf.git ~/esp-idf-v5.5"
	@echo "  cd ~/esp-idf-v5.5 && ./install.sh esp32s3"
	@echo "  source ~/esp-idf-v5.5/export.sh"
	@false
endif
endif

# Build targets
# Usage: make build              - Build LC-Alpha (default)
#        make build MODEL=alpha  - Build LC-Alpha (Heltec V3, 1 button)
#        make build MODEL=alpha+ - Build LC-Alpha+ (Heltec V3, 2 buttons)
#        make build MODEL=gamma  - Build LC-Gamma (LilyGO T5, E-paper)
MODEL ?= alpha

ifeq ($(MODEL),alpha)
MODEL_NAME = LC-Alpha
BOARD_NAME = Heltec V3
BOARD_ID = heltec_v3
SDKCONFIG = sdkconfig.defaults;sdkconfig.heltec_v3;sdkconfig.model_alpha
else ifeq ($(MODEL),alpha+)
MODEL_NAME = LC-Alpha+
BOARD_NAME = Heltec V3
BOARD_ID = heltec_v3
SDKCONFIG = sdkconfig.defaults;sdkconfig.heltec_v3;sdkconfig.model_alpha_plus
else ifeq ($(MODEL),beta)
MODEL_NAME = LC-Beta
BOARD_NAME = LilyGO T3-S3
BOARD_ID = lilygo_t3
SDKCONFIG = sdkconfig.defaults;sdkconfig.lilygo_t3;sdkconfig.model_beta
else ifeq ($(MODEL),gamma)
MODEL_NAME = LC-Gamma
BOARD_NAME = LilyGO T5 Pro
BOARD_ID = lilygo_t5
SDKCONFIG = sdkconfig.defaults;sdkconfig.lilygo_t5;sdkconfig.model_gamma
else
$(error Invalid MODEL=$(MODEL). Use: alpha, alpha+, beta, or gamma)
endif

build: check-idf fonts-1bpp ui_compact_assets
	@echo "🔨 Building $(MODEL_NAME) ($(BOARD_NAME))..."
	@rm -f sdkconfig
	$(IDF_SETUP) idf.py -D SDKCONFIG_DEFAULTS="$(SDKCONFIG)" -D BOARD_ID="$(BOARD_ID)" build
	@echo "✅ $(MODEL_NAME) build complete"

clean:
	@echo "🧹 Cleaning build artifacts and sdkconfig..."
	@rm -f sdkconfig
	$(IDF_SETUP) idf.py clean

fullclean:
	@echo "🧹 Full clean (removing CMake cache, sdkconfig, and generated assets)..."
	@rm -f sdkconfig
	@rm -rf $(UI_COMPACT_GENERATED_DIR)/*.c
	@rm -rf $(FONT_OUTPUT_DIR)/*.c
	$(IDF_SETUP) idf.py fullclean

rebuild: fullclean build

# Flash targets with MODEL parameter
# Usage: make flash              - Flash LC-Alpha (default)
#        make flash MODEL=alpha  - Flash LC-Alpha
#        make flash MODEL=alpha+ - Flash LC-Alpha+
#        make flash MODEL=gamma  - Flash LC-Gamma

flash: check-idf
	@echo "📡 Flashing $(MODEL_NAME) firmware..."
	@rm -f sdkconfig
	$(IDF_SETUP) idf.py -D SDKCONFIG_DEFAULTS="$(SDKCONFIG)" -b 921600 flash

flash-only: check-idf
	@echo "📡 Flashing $(MODEL_NAME) firmware (no rebuild)..."
	@test -f build/loracue.bin || { echo "❌ Firmware not found. Run 'make build MODEL=$(MODEL)' first"; exit 1; }
	$(IDF_SETUP) idf.py -b 921600 flash

flash-monitor: check-idf
	@echo "📡 Flashing $(MODEL_NAME) firmware and starting monitor..."
	@rm -f sdkconfig
	$(IDF_SETUP) idf.py -D SDKCONFIG_DEFAULTS="$(SDKCONFIG)" -b 921600 flash monitor

monitor:
	@echo "📺 Starting serial monitor (Ctrl+] to exit)..."
	$(IDF_SETUP) idf.py monitor

erase: check-idf
	@echo "🗑️  Erasing flash memory..."
	$(IDF_SETUP) idf.py erase-flash

# Configuration
menuconfig: check-idf
	@echo "⚙️  Opening configuration menu..."
	$(IDF_SETUP) idf.py menuconfig

set-target: check-idf
	@echo "🎯 Setting target to ESP32-S3..."
	$(IDF_SETUP) idf.py set-target esp32s3

size: check-idf
	@echo "📊 Analyzing binary size..."
	$(IDF_SETUP) idf.py size

# Code quality
format:
	@echo "🎨 Formatting code..."
	@command -v clang-format >/dev/null 2>&1 || { echo "❌ clang-format not found"; exit 1; }
	@find main components -type f \( -name '*.c' -o -name '*.h' \) \
		! -path "*/sx126x/*" \
		! -path "*/u8g2/*" \
		-exec clang-format -i {} +
	@echo "✅ Code formatted"

format-check:
	@echo "🔍 Checking code formatting..."
	@command -v clang-format >/dev/null 2>&1 || { echo "❌ clang-format not found"; exit 1; }
	@UNFORMATTED=$$(find main components -type f \( -name '*.c' -o -name '*.h' \) \
		! -path "*/sx126x/*" \
		! -path "*/u8g2/*" \
		-exec clang-format --dry-run --Werror {} + 2>&1 | grep "warning:" || true); \
	if [ -n "$$UNFORMATTED" ]; then \
		echo "❌ Code formatting issues found. Fix with: make format"; \
		exit 1; \
	fi
	@echo "✅ Code formatting OK"

lint:
	@echo "🔍 Running static analysis..."
	@command -v cppcheck >/dev/null 2>&1 || { echo "❌ cppcheck not found"; exit 1; }
	@cppcheck --enable=warning,style,performance,portability \
		--suppress=missingIncludeSystem \
		--suppress=unmatchedSuppression \
		--inline-suppr \
		--error-exitcode=1 \
		--quiet \
		-i components/sx126x \
		-i components/u8g2 \
		-I main/include \
		-I components/*/include \
		main/ components/ 2>&1 | grep -v "Checking" || true
	@echo "✅ Static analysis passed"

# ============================================================================
# 🧪 Testing
# ============================================================================

test:
	@echo "🧪 Running host-based tests with REAL lora_protocol.c code..."
	@cd test/host_test && gcc -o test_runner test_lora_host.c \
		-I. \
		-I../../components/lora/include \
		-I/opt/homebrew/include \
		-L/opt/homebrew/lib \
		-lmbedcrypto && ./test_runner

test-device: check-idf
	@echo "🧪 Building and running LoRa protocol tests on device..."
	$(IDF_SETUP) idf.py -DTEST_COMPONENTS='test' build
	$(IDF_SETUP) idf.py -b 921600 flash monitor

test-build: check-idf
	@echo "🧪 Building device tests only..."
	$(IDF_SETUP) idf.py -DTEST_COMPONENTS='test' build

# ============================================================================
# 🎮 Wokwi Simulator
# ============================================================================

# Start SX1262 RF relay server
	@echo "📦 Installing relay server dependencies..."
	@cd wokwi/sx1262-rf-relay && npm install

# Wokwi simulator
WOKWI_BOARD ?= heltec_v3
WOKWI_DIR = wokwi/$(WOKWI_BOARD)

sim: check-idf build/wokwi/chips/uart.chip.wasm build/wokwi/chips/sx1262.chip.wasm
ifndef WOKWI_CLI
	@echo "❌ Wokwi CLI not found. Install: npm install -g wokwi-cli"
	@false
endif
	@$(MAKE) build BOARD=heltec
	@echo "✅ Wokwi simulation ready (using Heltec V3 firmware)"

# Build all custom Wokwi chips
chips: build/wokwi/chips/uart.chip.wasm build/wokwi/chips/sx1262.chip.wasm build/wokwi/chips/pca9535.chip.wasm build/wokwi/chips/pcf85063.chip.wasm build/wokwi/chips/bq27220.chip.wasm build/wokwi/chips/bq25896.chip.wasm build/wokwi/chips/tps65185.chip.wasm build/wokwi/chips/gt911.chip.wasm build/wokwi/chips/ed047tc1.chip.wasm build/wokwi/chips/i2console.chip.wasm
	@echo "✅ All custom chips compiled"

# Build custom UART bridge chip
build/wokwi/chips/uart.chip.wasm: wokwi/chips/uart.chip.c wokwi/chips/wokwi-api.h wokwi/chips/uart.chip.json
	@echo "🔧 Compiling custom UART bridge chip..."
	@mkdir -p build/wokwi/chips
	@$(CLANG) --target=wasm32-unknown-wasi \
		--sysroot $(WASI_SYSROOT) \
		-c -o build/wokwi/chips/uart.o wokwi/chips/uart.chip.c
	@$(WASM_LD) --no-entry --import-memory --export-table \
		-o build/wokwi/chips/uart.chip.wasm \
		build/wokwi/chips/uart.o \
		$(WASI_SYSROOT)/lib/wasm32-wasi/libc.a
	@cp wokwi/chips/uart.chip.json build/wokwi/chips/
	@rm build/wokwi/chips/uart.o
	@echo "✅ Custom chip compiled"

# Build custom SX1262 LoRa chip
build/wokwi/chips/sx1262.chip.wasm: wokwi/chips/sx1262.chip.c wokwi/chips/wokwi-api.h wokwi/chips/sx1262.chip.json wokwi/chips/sx1262.chip.svg
	@echo "🔧 Compiling custom SX1262 LoRa chip..."
	@mkdir -p build/wokwi/chips
	@$(CLANG) --target=wasm32-unknown-wasi \
		--sysroot $(WASI_SYSROOT) \
		-c -o build/wokwi/chips/sx1262.o wokwi/chips/sx1262.chip.c
	@$(WASM_LD) --no-entry --import-memory --export-table \
		-o build/wokwi/chips/sx1262.chip.wasm \
		build/wokwi/chips/sx1262.o \
		$(WASI_SYSROOT)/lib/wasm32-wasi/libc.a
	@cp wokwi/chips/sx1262.chip.json build/wokwi/chips/
	@cp wokwi/chips/sx1262.chip.svg build/wokwi/chips/
	@rm build/wokwi/chips/sx1262.o
	@echo "✅ SX1262 chip compiled"

# Build custom PCA9535 GPIO expander chip
build/wokwi/chips/pca9535.chip.wasm: wokwi/chips/pca9535.chip.c wokwi/chips/wokwi-api.h wokwi/chips/pca9535.chip.json
	@echo "🔧 Compiling custom PCA9535 chip..."
	@mkdir -p build/wokwi/chips
	@$(CLANG) --target=wasm32-unknown-wasi \
		--sysroot $(WASI_SYSROOT) \
		-c -o build/wokwi/chips/pca9535.o wokwi/chips/pca9535.chip.c
	@$(WASM_LD) --no-entry --import-memory --export-table \
		-o build/wokwi/chips/pca9535.chip.wasm \
		build/wokwi/chips/pca9535.o \
		$(WASI_SYSROOT)/lib/wasm32-wasi/libc.a
	@cp wokwi/chips/pca9535.chip.json build/wokwi/chips/
	@rm build/wokwi/chips/pca9535.o
	@echo "✅ PCA9535 chip compiled"

# Build custom PCF85063 RTC chip
build/wokwi/chips/pcf85063.chip.wasm: wokwi/chips/pcf85063.chip.c wokwi/chips/wokwi-api.h wokwi/chips/pcf85063.chip.json
	@echo "🔧 Compiling custom PCF85063 RTC chip..."
	@mkdir -p build/wokwi/chips
	@$(CLANG) --target=wasm32-unknown-wasi \
		--sysroot $(WASI_SYSROOT) \
		-c -o build/wokwi/chips/pcf85063.o wokwi/chips/pcf85063.chip.c
	@$(WASM_LD) --no-entry --import-memory --export-table \
		-o build/wokwi/chips/pcf85063.chip.wasm \
		build/wokwi/chips/pcf85063.o \
		$(WASI_SYSROOT)/lib/wasm32-wasi/libc.a
	@cp wokwi/chips/pcf85063.chip.json build/wokwi/chips/
	@rm build/wokwi/chips/pcf85063.o
	@echo "✅ PCF85063 chip compiled"

# Build custom BQ27220 fuel gauge chip
build/wokwi/chips/bq27220.chip.wasm: wokwi/chips/bq27220.chip.c wokwi/chips/wokwi-api.h wokwi/chips/bq27220.chip.json
	@echo "🔧 Compiling custom BQ27220 fuel gauge chip..."
	@mkdir -p build/wokwi/chips
	@$(CLANG) --target=wasm32-unknown-wasi \
		--sysroot $(WASI_SYSROOT) \
		-c -o build/wokwi/chips/bq27220.o wokwi/chips/bq27220.chip.c
	@$(WASM_LD) --no-entry --import-memory --export-table \
		-o build/wokwi/chips/bq27220.chip.wasm \
		build/wokwi/chips/bq27220.o \
		$(WASI_SYSROOT)/lib/wasm32-wasi/libc.a
	@cp wokwi/chips/bq27220.chip.json build/wokwi/chips/
	@rm build/wokwi/chips/bq27220.o
	@echo "✅ BQ27220 chip compiled"

# Build custom BQ25896 charger chip
build/wokwi/chips/bq25896.chip.wasm: wokwi/chips/bq25896.chip.c wokwi/chips/wokwi-api.h wokwi/chips/bq25896.chip.json
	@echo "🔧 Compiling custom BQ25896 charger chip..."
	@mkdir -p build/wokwi/chips
	@$(CLANG) --target=wasm32-unknown-wasi \
		--sysroot $(WASI_SYSROOT) \
		-c -o build/wokwi/chips/bq25896.o wokwi/chips/bq25896.chip.c
	@$(WASM_LD) --no-entry --import-memory --export-table \
		-o build/wokwi/chips/bq25896.chip.wasm \
		build/wokwi/chips/bq25896.o \
		$(WASI_SYSROOT)/lib/wasm32-wasi/libc.a
	@cp wokwi/chips/bq25896.chip.json build/wokwi/chips/
	@rm build/wokwi/chips/bq25896.o
	@echo "✅ BQ25896 chip compiled"

# Build custom TPS65185 e-paper PMIC chip
build/wokwi/chips/tps65185.chip.wasm: wokwi/chips/tps65185.chip.c wokwi/chips/wokwi-api.h wokwi/chips/tps65185.chip.json
	@echo "🔧 Compiling custom TPS65185 PMIC chip..."
	@mkdir -p build/wokwi/chips
	@$(CLANG) --target=wasm32-unknown-wasi \
		--sysroot $(WASI_SYSROOT) \
		-c -o build/wokwi/chips/tps65185.o wokwi/chips/tps65185.chip.c
	@$(WASM_LD) --no-entry --import-memory --export-table \
		-o build/wokwi/chips/tps65185.chip.wasm \
		build/wokwi/chips/tps65185.o \
		$(WASI_SYSROOT)/lib/wasm32-wasi/libc.a
	@cp wokwi/chips/tps65185.chip.json build/wokwi/chips/
	@rm build/wokwi/chips/tps65185.o
	@echo "✅ TPS65185 chip compiled"

# Build custom GT911 touch controller chip
build/wokwi/chips/gt911.chip.wasm: wokwi/chips/gt911.chip.c wokwi/chips/wokwi-api.h wokwi/chips/gt911.chip.json
	@echo "🔧 Compiling custom GT911 touch chip..."
	@mkdir -p build/wokwi/chips
	@$(CLANG) --target=wasm32-unknown-wasi \
		--sysroot $(WASI_SYSROOT) \
		-c -o build/wokwi/chips/gt911.o wokwi/chips/gt911.chip.c
	@$(WASM_LD) --no-entry --import-memory --export-table \
		-o build/wokwi/chips/gt911.chip.wasm \
		build/wokwi/chips/gt911.o \
		$(WASI_SYSROOT)/lib/wasm32-wasi/libc.a
	@cp wokwi/chips/gt911.chip.json build/wokwi/chips/
	@rm build/wokwi/chips/gt911.o
	@echo "✅ GT911 chip compiled"

# Build custom ED047TC1 e-paper display chip
build/wokwi/chips/ed047tc1.chip.wasm: wokwi/chips/ed047tc1.chip.c wokwi/chips/wokwi-api.h wokwi/chips/ed047tc1.chip.json
	@echo "🔧 Compiling custom ED047TC1 e-paper display chip..."
	@mkdir -p build/wokwi/chips
	@$(CLANG) --target=wasm32-unknown-wasi \
		--sysroot $(WASI_SYSROOT) \
		-c -o build/wokwi/chips/ed047tc1.o wokwi/chips/ed047tc1.chip.c
	@$(WASM_LD) --no-entry --import-memory --export-table \
		-o build/wokwi/chips/ed047tc1.chip.wasm \
		build/wokwi/chips/ed047tc1.o \
		$(WASI_SYSROOT)/lib/wasm32-wasi/libc.a
	@cp wokwi/chips/ed047tc1.chip.json build/wokwi/chips/
	@rm build/wokwi/chips/ed047tc1.o
	@echo "✅ ED047TC1 chip compiled"

# Build custom I2Console chip
build/wokwi/chips/i2console.chip.wasm: wokwi/chips/i2console.chip.c wokwi/chips/wokwi-api.h wokwi/chips/i2console.chip.json
	@echo "🔧 Compiling custom I2Console chip..."
	@mkdir -p build/wokwi/chips
	@$(CLANG) --target=wasm32-unknown-wasi \
		--sysroot $(WASI_SYSROOT) \
		-c -o build/wokwi/chips/i2console.o wokwi/chips/i2console.chip.c
	@$(WASM_LD) --no-entry --import-memory --export-table \
		-o build/wokwi/chips/i2console.chip.wasm \
		build/wokwi/chips/i2console.o \
		$(WASI_SYSROOT)/lib/wasm32-wasi/libc.a
	@cp wokwi/chips/i2console.chip.json build/wokwi/chips/
	@rm build/wokwi/chips/i2console.o
	@echo "✅ I2Console chip compiled"

sim-run: sim
	@if [ ! -d $(WOKWI_DIR) ]; then \
		echo "❌ Board $(WOKWI_BOARD) not found"; \
		exit 1; \
	fi
	@echo "🚀 Starting Wokwi simulation for $(WOKWI_BOARD)..."
	@cd $(WOKWI_DIR) && wokwi-cli .
	@echo "💡 UART0 commands: telnet localhost 4000 (RFC2217)"
	@echo "💡 Serial log: wokwi.log"
	@echo "💡 Press Ctrl+C to stop"
	@echo ""
	wokwi-cli --timeout 0 --interactive --serial-log-file wokwi.log

sim-debug: sim
	@echo "🐛 Starting debug simulation with interactive serial..."
	@echo "💡 UART0 commands: telnet localhost 4000 (RFC2217)"
	@echo "💡 Serial log: wokwi.log"
	@echo "💡 Press Ctrl+C to stop"
	@echo ""
	wokwi-cli --timeout 0 --interactive --serial-log-file wokwi.log

build/wokwi_sim.bin:
	@$(MAKE) sim

# Web interface
web-dev:
	@echo "🌐 Starting web interface development server..."
	@cd web-interface && npm run dev

web-build:
	@echo "🏗️  Building web interface..."
	@cd web-interface && npm install && npm run build
	@echo "📦 Creating LittleFS image..."
	@command -v mklittlefs >/dev/null 2>&1 || command -v /usr/local/bin/mklittlefs >/dev/null 2>&1 || { echo "❌ mklittlefs not found"; exit 1; }
	@mkdir -p build
	@if command -v mklittlefs >/dev/null 2>&1; then \
		mklittlefs -c web-interface/out -b 4096 -p 256 -s 0x1C0000 build/webui-littlefs.bin; \
	else \
		/usr/local/bin/mklittlefs -c web-interface/out -b 4096 -p 256 -s 0x1C0000 build/webui-littlefs.bin; \
	fi
	@echo "✅ Web interface built: build/webui-littlefs.bin"

web-flash: check-idf
	@if [ ! -f build/webui-littlefs.bin ]; then \
		echo "📦 Building web interface..."; \
		$(MAKE) web-build; \
	fi
	@echo "⚡ Flashing LittleFS partition..."
	$(IDF_SETUP) esptool.py --chip esp32s3 write_flash 0x640000 build/webui-littlefs.bin
	@echo "✅ Web interface flashed"

# Help
help:
	@echo "🚀 LoRaCue Build System"
	@echo ""
	@echo "📦 Build:"
	@echo "  make build              - Build LC-Alpha (default)"
	@echo "  make build MODEL=alpha  - Build LC-Alpha (Heltec V3, 1 button)"
	@echo "  make build MODEL=alpha+ - Build LC-Alpha+ (Heltec V3, 2 buttons)"
	@echo "  make build MODEL=gamma  - Build LC-Gamma (LilyGO T5, E-paper)"
	@echo "  make rebuild            - Clean and rebuild"
	@echo "  make clean              - Clean build artifacts"
	@echo "  make fullclean          - Full clean (CMake cache + sdkconfig)"
	@echo ""
	@echo "📡 Flash:"
	@echo "  make flash              - Flash LC-Alpha (default)"
	@echo "  make flash MODEL=alpha  - Flash LC-Alpha"
	@echo "  make flash MODEL=alpha+ - Flash LC-Alpha+"
	@echo "  make flash MODEL=gamma  - Flash LC-Gamma"
	@echo "  make flash-monitor MODEL=alpha - Flash and monitor LC-Alpha"
	@echo "  make monitor            - Serial monitor only"
	@echo "  make erase              - Erase entire flash"
	@echo ""
	@echo "⚙️  Config:"
	@echo "  make menuconfig    - Open ESP-IDF configuration"
	@echo "  make set-target    - Set target to ESP32-S3 (run once)"
	@echo "  make size          - Show binary size analysis"
	@echo ""
	@echo "🎮 Simulator:"
	@echo "  make sim           - Build for Wokwi"
	@echo "  make sim-run       - Run Wokwi simulation (WOKWI_BOARD=heltec_v3|lilygo_t5)"
	@echo "  make sim-debug     - Interactive simulation"
	@echo ""
	@echo "🌐 Web Interface:"
	@echo "  make web-dev       - Start dev server (localhost:3000)"
	@echo "  make web-build     - Build production web UI"
	@echo "  make web-flash     - Flash web UI to device"
	@echo ""
	@echo "🧪 Testing:"
	@echo "  make test          - Build and run protocol tests"
	@echo "  make test-build    - Build tests only"
	@echo ""
	@echo "🎨 Code Quality:"
	@echo "  make format        - Format all code"
	@echo "  make format-check  - Check formatting"
	@echo "  make lint          - Run static analysis"
	@echo ""
	@echo "🔤 Font Generation:"
	@echo "  make fonts-1bpp    - Generate 1bpp fonts (OLED)"
	@echo "  make fonts-4bpp    - Generate 4bpp fonts (e-paper)"
	@echo "  make fonts-8bpp    - Generate 8bpp fonts (color)"
	@echo "  make fonts-all     - Generate all font variants"
	@echo ""
	@echo "💡 Quick Start:"
	@echo "  source ~/esp-idf-v5.5/export.sh  # Setup ESP-IDF"
	@echo "  make set-target                   # First time only"
	@echo "  make build MODEL=alpha            # Build LC-Alpha"
	@echo "  make flash-monitor MODEL=alpha    # Flash and monitor"
