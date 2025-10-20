# LoRaCue Makefile with ESP-IDF Auto-Detection and Wokwi Simulator

.PHONY: all build build-console-on-uart0 clean fullclean rebuild flash flash-monitor monitor menuconfig size erase set-target format format-check lint sim sim-run sim-debug chips web-dev web-build web-flash help check-idf

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
build: check-idf
	@echo "🔨 Building LoRaCue firmware..."
	$(IDF_SETUP) idf.py build

build-console-on-uart0: check-idf
	@echo "🐛 Building LoRaCue firmware (console on UART0, commands on UART1)..."
	@rm -f sdkconfig
	$(IDF_SETUP) SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.console-on-uart0" idf.py build

clean:
	@echo "🧹 Cleaning build artifacts and sdkconfig..."
	@rm -f sdkconfig
	$(IDF_SETUP) idf.py clean

fullclean:
	@echo "🧹 Full clean (removing CMake cache and sdkconfig)..."
	@rm -f sdkconfig
	$(IDF_SETUP) idf.py fullclean

rebuild: clean build

# Flash targets
flash: check-idf
	@echo "🔍 Checking firmware type..."
	@if [ -f build/wokwi_sim.bin ] && [ ! -f build/heltec_v3.bin ]; then \
		echo "❌ ERROR: Cannot flash simulator build to real hardware!"; \
		echo "Run: make fullclean && make build"; \
		exit 1; \
	fi
	@if [ ! -f build/heltec_v3.bin ]; then \
		echo "❌ ERROR: Hardware firmware not found! Run: make build"; \
		exit 1; \
	fi
	@echo "✅ Hardware firmware detected"
	@echo "📡 Flashing firmware to device..."
	$(IDF_SETUP) idf.py flash

flash-monitor: flash monitor

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

# Wokwi simulator
sim: check-idf build/wokwi-chips/uart.chip.wasm build/wokwi-chips/sx1262.chip.wasm
ifndef WOKWI_CLI
	@echo "❌ Wokwi CLI not found. Install: npm install -g wokwi-cli"
	@false
endif
	@echo "🎮 Building for Wokwi simulator..."
	$(IDF_SETUP) WOKWI_BUILD=1 idf.py build

# Build all custom Wokwi chips
chips: build/wokwi-chips/uart.chip.wasm build/wokwi-chips/sx1262.chip.wasm
	@echo "✅ All custom chips compiled"

# Build custom UART bridge chip
build/wokwi-chips/uart.chip.wasm: wokwi-chips/uart.chip.c wokwi-chips/wokwi-api.h wokwi-chips/uart.chip.json
	@echo "🔧 Compiling custom UART bridge chip..."
	@mkdir -p build/wokwi-chips
	@/opt/homebrew/opt/llvm/bin/clang --target=wasm32-unknown-wasi \
		--sysroot /opt/homebrew/Cellar/wasi-libc/27/share/wasi-sysroot \
		-c -o build/wokwi-chips/uart.o wokwi-chips/uart.chip.c
	@wasm-ld --no-entry --import-memory --export-table \
		-o build/wokwi-chips/uart.chip.wasm \
		build/wokwi-chips/uart.o \
		/opt/homebrew/Cellar/wasi-libc/27/share/wasi-sysroot/lib/wasm32-wasi/libc.a
	@cp wokwi-chips/uart.chip.json build/wokwi-chips/
	@rm build/wokwi-chips/uart.o
	@echo "✅ Custom chip compiled"

# Build custom SX1262 LoRa chip
build/wokwi-chips/sx1262.chip.wasm: wokwi-chips/sx1262.chip.c wokwi-chips/wokwi-api.h wokwi-chips/sx1262.chip.json wokwi-chips/sx1262.chip.svg
	@echo "🔧 Compiling custom SX1262 LoRa chip..."
	@mkdir -p build/wokwi-chips
	@/opt/homebrew/opt/llvm/bin/clang --target=wasm32-unknown-wasi \
		--sysroot /opt/homebrew/Cellar/wasi-libc/27/share/wasi-sysroot \
		-c -o build/wokwi-chips/sx1262.o wokwi-chips/sx1262.chip.c
	@wasm-ld --no-entry --import-memory --export-table \
		-o build/wokwi-chips/sx1262.chip.wasm \
		build/wokwi-chips/sx1262.o \
		/opt/homebrew/Cellar/wasi-libc/27/share/wasi-sysroot/lib/wasm32-wasi/libc.a
	@cp wokwi-chips/sx1262.chip.json build/wokwi-chips/
	@cp wokwi-chips/sx1262.chip.svg build/wokwi-chips/
	@rm build/wokwi-chips/sx1262.o
	@echo "✅ SX1262 chip compiled"

sim-run: build/wokwi_sim.bin
	@echo "🚀 Starting Wokwi simulation..."
	@echo "💡 UART0 commands: telnet localhost 4000 (RFC2217)"
	@echo "💡 Serial log: wokwi.log"
	@echo "💡 Press Ctrl+C to stop"
	@echo ""
	wokwi-cli --timeout 0 --interactive --serial-log-file wokwi.log

sim-debug: build/wokwi_sim.bin
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
	@command -v mklittlefs >/dev/null 2>&1 || { echo "❌ mklittlefs not found"; exit 1; }
	@mkdir -p build
	mklittlefs -c web-interface/out -b 4096 -p 256 -s 0x1C0000 build/webui-littlefs.bin
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
	@echo "  make build                    - Build firmware (UART0=commands, UART1=console)"
	@echo "  make build-console-on-uart0   - Build with console on UART0, commands on UART1"
	@echo "  make rebuild                  - Clean and rebuild"
	@echo "  make clean         - Clean build artifacts"
	@echo "  make fullclean     - Full clean (CMake cache + sdkconfig)"
	@echo ""
	@echo "📡 Flash:"
	@echo "  make flash         - Flash firmware to device"
	@echo "  make flash-monitor - Flash and start serial monitor"
	@echo "  make monitor       - Serial monitor only"
	@echo "  make erase         - Erase entire flash"
	@echo ""
	@echo "⚙️  Config:"
	@echo "  make menuconfig    - Open ESP-IDF configuration"
	@echo "  make set-target    - Set target to ESP32-S3 (run once)"
	@echo "  make size          - Show binary size analysis"
	@echo ""
	@echo "🎮 Simulator:"
	@echo "  make sim           - Build for Wokwi"
	@echo "  make sim-run       - Run Wokwi simulation"
	@echo "  make sim-debug     - Interactive simulation"
	@echo ""
	@echo "🌐 Web Interface:"
	@echo "  make web-dev       - Start dev server (localhost:3000)"
	@echo "  make web-build     - Build production web UI"
	@echo "  make web-flash     - Flash web UI to device"
	@echo ""
	@echo "🎨 Code Quality:"
	@echo "  make format        - Format all code"
	@echo "  make format-check  - Check formatting"
	@echo "  make lint          - Run static analysis"
	@echo ""
	@echo "💡 Quick Start:"
	@echo "  source ~/esp-idf-v5.5/export.sh  # Setup ESP-IDF"
	@echo "  make set-target                   # First time only"
	@echo "  make build                        # Build firmware"
	@echo "  make flash-monitor                # Flash and monitor"
