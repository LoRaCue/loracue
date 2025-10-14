# LoRaCue Makefile with ESP-IDF Auto-Detection and Wokwi Simulator
# Automatically finds and sets up ESP-IDF environment

.PHONY: build clean flash monitor menuconfig size erase help check-idf setup-env sim-build sim-web sim sim-debug sim-screenshot check-wokwi web-build web-flash web-dev format format-check lint

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

# Check if idf.py is already in PATH (environment already set)
IDF_PY_IN_PATH := $(shell which idf.py 2>/dev/null)

# Determine if we need to source export.sh
ifdef IDF_PY_IN_PATH
    # ESP-IDF already in environment
    IDF_SETUP := 
    IDF_VERSION := $(shell idf.py --version 2>/dev/null | grep -o 'v[0-9]\+\.[0-9]\+')
else ifdef IDF_PATH_FOUND
    # ESP-IDF found, need to source export.sh
    IDF_SETUP := source $(IDF_PATH_FOUND)/export.sh >/dev/null 2>&1 &&
    IDF_VERSION := $(shell source $(IDF_PATH_FOUND)/export.sh >/dev/null 2>&1 && idf.py --version 2>/dev/null | grep -o 'v[0-9]\+\.[0-9]\+')
else
    # ESP-IDF not found
    IDF_SETUP := 
    IDF_VERSION := 
endif

# Default target
all: check-idf build

# Check ESP-IDF installation and version
check-idf:
ifndef IDF_PY_IN_PATH
ifndef IDF_PATH_FOUND
	@echo "❌ ESP-IDF not found!"
	@echo ""
	@echo "Please install ESP-IDF v5.5 using one of these methods:"
	@echo ""
	@echo "1. Official installer:"
	@echo "   https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/get-started/index.html"
	@echo ""
	@echo "2. Manual installation:"
	@echo "   git clone -b v5.5 --recursive https://github.com/espressif/esp-idf.git ~/esp-idf-v5.5"
	@echo "   cd ~/esp-idf-v5.5 && ./install.sh esp32s3"
	@echo ""
	@echo "3. Then run: source ~/esp-idf-v5.5/export.sh"
	@echo "   Or add to your shell profile for permanent setup"
	@false
endif
endif
	@echo "✅ ESP-IDF found: $(if $(IDF_PATH_FOUND),$(IDF_PATH_FOUND),already in PATH)"
	@echo "📋 Version: $(if $(IDF_VERSION),$(IDF_VERSION),unknown)"
ifneq ($(IDF_VERSION),v5.5)
	@echo "⚠️  Warning: Expected ESP-IDF v5.5, found $(IDF_VERSION)"
	@echo "   This may cause build issues. Consider using ESP-IDF v5.5."
endif

# Build the project
build: check-idf
	@echo "🔨 Building LoRaCue firmware..."
	$(IDF_SETUP) idf.py build

# Build with debug logging on UART0
build-debug: check-idf
	@echo "🐛 Building LoRaCue firmware (DEBUG mode - logging on UART0)..."
	@rm -f sdkconfig
	$(IDF_SETUP) SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.debug" idf.py build

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	$(IDF_SETUP) idf.py clean

# Full clean (removes CMake cache - needed when switching between sim/hardware builds)
fullclean:
	@echo "🧹 Full clean (removing CMake cache)..."
	$(IDF_SETUP) idf.py fullclean

# Flash firmware to device
flash: check-idf
	@echo "🔍 Checking firmware type..."
	@if [ -f build/wokwi_sim.bin ] && [ ! -f build/heltec_v3.bin ]; then \
		echo "❌ ERROR: Cannot flash simulator build to real hardware!"; \
		echo ""; \
		echo "You built with 'make sim' which creates a Wokwi simulator binary."; \
		echo "This binary uses bsp_wokwi.c (SSD1306) instead of bsp_heltec_v3.c (SH1106)."; \
		echo ""; \
		echo "To flash to real hardware:"; \
		echo "  1. Full clean: make fullclean"; \
		echo "  2. Build: make build"; \
		echo "  3. Flash: make flash"; \
		echo ""; \
		exit 1; \
	fi
	@if [ ! -f build/heltec_v3.bin ]; then \
		echo "❌ ERROR: Hardware firmware not found!"; \
		echo ""; \
		echo "Please build first: make build"; \
		echo ""; \
		exit 1; \
	fi
	@echo "✅ Hardware firmware detected (heltec_v3.bin)"
	@echo "📡 Flashing firmware to device..."
	$(IDF_SETUP) idf.py flash

# Flash and start monitor
flash-monitor: flash monitor

# Monitor serial output
monitor:
	@echo "📺 Starting serial monitor (Ctrl+] to exit)..."
	$(IDF_SETUP) idf.py monitor

# Open configuration menu
menuconfig: check-idf
	@echo "⚙️  Opening configuration menu..."
	$(IDF_SETUP) idf.py menuconfig

# Show size information
size: check-idf
	@echo "📊 Analyzing binary size..."
	$(IDF_SETUP) idf.py size

# Erase entire flash
erase: check-idf
	@echo "🗑️  Erasing flash memory..."
	$(IDF_SETUP) idf.py erase-flash

# Code Quality Targets
format:
	@echo "🎨 Formatting code..."
	@if ! command -v clang-format >/dev/null 2>&1; then \
		echo "❌ clang-format not found. Install with:"; \
		echo "  macOS: brew install clang-format"; \
		echo "  Linux: sudo apt install clang-format"; \
		exit 1; \
	fi
	@find main components -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \) \
		! -path "*/sx126x/*" \
		! -path "*/u8g2/*" \
		! -path "*/u8g2-hal-esp-idf/*" \
		-exec clang-format -i {} +
	@echo "✅ Code formatted"

format-check:
	@echo "🔍 Checking code formatting..."
	@if ! command -v clang-format >/dev/null 2>&1; then \
		echo "❌ clang-format not found. Install with:"; \
		echo "  macOS: brew install clang-format"; \
		echo "  Linux: sudo apt install clang-format"; \
		exit 1; \
	fi
	@UNFORMATTED=$$(find main components -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \) \
		! -path "*/sx126x/*" \
		! -path "*/u8g2/*" \
		! -path "*/u8g2-hal-esp-idf/*" \
		-exec clang-format --dry-run --Werror {} + 2>&1 | grep "warning:" || true); \
	if [ -n "$$UNFORMATTED" ]; then \
		echo "❌ Code formatting issues found"; \
		echo "$$UNFORMATTED"; \
		echo ""; \
		echo "Fix with: make format"; \
		exit 1; \
	fi
	@echo "✅ Code formatting OK"

lint:
	@echo "🔍 Running static analysis..."
	@if ! command -v cppcheck >/dev/null 2>&1; then \
		echo "❌ cppcheck not found. Install with:"; \
		echo "  macOS: brew install cppcheck"; \
		echo "  Linux: sudo apt install cppcheck"; \
		exit 1; \
	fi
	@cppcheck --enable=warning,style,performance,portability \
		--suppress=missingIncludeSystem \
		--suppress=unmatchedSuppression \
		--inline-suppr \
		--error-exitcode=1 \
		--quiet \
		-i components/sx126x \
		-i components/u8g2 \
		-i components/u8g2-hal-esp-idf \
		-I main/include \
		-I components/*/include \
		main/ components/ 2>&1 | grep -v "Checking" || true
	@if [ $$? -eq 1 ]; then \
		echo "❌ Static analysis found issues"; \
		exit 1; \
	fi
	@echo "✅ Static analysis passed"

# Set target (run once after fresh clone)
set-target: check-idf
	@echo "🎯 Setting target to ESP32-S3..."
	$(IDF_SETUP) idf.py set-target esp32s3

# Full clean rebuild
rebuild: clean build

# Web Interface Commands
web-dev:
	@echo "🌐 Starting web interface development server..."
	@cd web-interface && npm run dev

web-build:
	@echo "🏗️  Building web interface for production..."
	@cd web-interface && npm install && npm run build
	@echo "✅ Web interface built"

web-flash: check-idf web-build
	@echo "📦 Creating LittleFS image..."
	@command -v mklittlefs >/dev/null 2>&1 || { echo "❌ mklittlefs not found. Install with: brew install mklittlefs"; exit 1; }
	mklittlefs -c web-interface/out -s 0x1C0000 littlefs.bin
	@echo "⚡ Flashing LittleFS partition..."
	$(IDF_SETUP) esptool.py --chip esp32s3 write_flash 0x640000 littlefs.bin
	@rm -f littlefs.bin
	@echo "✅ Web interface flashed to LittleFS"

# Development cycle: build, flash, monitor
dev: web-build build flash monitor

# Setup development environment
setup-env:
	@echo "🔧 Setting up development environment..."
	@echo ""
	@echo "Installing Node.js dependencies for development tools..."
	npm install
	@echo ""
	@echo "✅ Development environment ready!"
	@echo ""
	@echo "Next steps:"
	@echo "1. Run 'make set-target' (first time only)"
	@echo "2. Run 'make build' to build firmware"
	@echo "3. Run 'make flash-monitor' to flash and monitor"

# Wokwi CLI detection
WOKWI_CLI := $(shell which wokwi-cli 2>/dev/null)

# Check Wokwi CLI installation
check-wokwi:
ifndef WOKWI_CLI
	@echo "❌ Wokwi CLI not found!"
	@echo ""
	@echo "Please install Wokwi CLI to run local simulation:"
	@echo "https://docs.wokwi.com/wokwi-ci/cli-installation"
	@echo ""
	@echo "Quick install options:"
	@echo "• npm install -g wokwi-cli"
	@echo "• curl -L https://wokwi.com/ci/install.sh | sh"
	@false
else
	@echo "✅ Wokwi CLI found: $(WOKWI_CLI)"
endif

# Wokwi Simulator targets
sim: check-idf
	@echo "🎮 Building for Wokwi simulator..."
	$(IDF_SETUP) SIMULATOR_BUILD=1 SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.debug" idf.py -D CMAKE_C_FLAGS=-DSIMULATOR_BUILD=1 build

sim-run: check-wokwi sim
	@echo "🚀 Starting Wokwi simulation..."
	@echo "💡 Press Ctrl+C to stop simulation"
	@echo "📺 Serial output will appear below..."
	@echo ""
	wokwi-cli --timeout 0 --serial-log-file -

sim-screenshot: check-wokwi sim
	@echo "📸 Taking OLED screenshot after 5 seconds..."
	wokwi-cli --timeout 5000 --screenshot-part oled --screenshot-time 4500 --timeout-exit-code 0
	@echo "✅ Screenshot saved as screenshot.png"

sim-debug: check-wokwi sim
	@echo "🐛 Starting debug simulation with interactive serial..."
	@echo "💡 You can type commands that will be sent to the ESP32"
	@echo "📺 Press Ctrl+C to stop"
	@echo ""
	wokwi-cli --timeout 0 --interactive --serial-log-file -

sim-web:
	@echo "🌐 For VISUAL simulation with clickable buttons and OLED display:"
	@echo ""
	@echo "1. Go to https://wokwi.com/projects/new/esp32"
	@echo "2. Upload the diagram.json file"
	@echo "3. Upload the firmware binary from build/loracue.bin"
	@echo "4. Click 'Start Simulation'"
	@echo ""
	@echo "In the web simulator you can:"
	@echo "• Click buttons with your mouse"
	@echo "• See the OLED display update in real-time"
	@echo "• Adjust the battery potentiometer"
	@echo "• Watch status LEDs blink"
	@echo ""
	@echo "CLI simulation (make sim) only shows serial output, no visual interface."

sim-info:
	@echo "🎮 Wokwi Simulator Information"
	@echo ""
	@echo "📋 Simulated Hardware:"
	@echo "  • ESP32-S3 microcontroller"
	@echo "  • SSD1306 OLED display (128x64)"
	@echo "  • Two pushbuttons (PREV/NEXT)"
	@echo "  • Battery voltage potentiometer"
	@echo "  • Status LEDs (Power, TX, RX)"
	@echo ""
	@echo "🔌 Pin Connections:"
	@echo "  • OLED: SDA=GPIO17, SCL=GPIO18"
	@echo "  • Buttons: PREV=GPIO45, NEXT=GPIO46"
	@echo "  • Battery: ADC=GPIO1"
	@echo "  • LEDs: Power=GPIO2, TX=GPIO3, RX=GPIO4"
	@echo ""
	@echo "🚀 Usage:"
	@echo "  1. make sim-build    # Build firmware"
	@echo "  2. make sim-web      # Instructions for web simulator"
	@echo "  3. Use buttons to navigate OLED menu"
	@echo "  4. Adjust potentiometer to simulate battery"

# Show environment info
env-info: check-idf
	@echo "🔍 Environment Information:"
	@echo "  ESP-IDF Path: $(if $(IDF_PATH_FOUND),$(IDF_PATH_FOUND),in PATH)"
	@echo "  ESP-IDF Version: $(IDF_VERSION)"
	@echo "  idf.py Location: $(if $(IDF_PY_IN_PATH),$(IDF_PY_IN_PATH),will be sourced)"
	@echo "  Project Path: $(PWD)"
	@echo "  Target: ESP32-S3"

# Show help
help:
	@echo "🚀 LoRaCue Build System"
	@echo ""
	@echo "📋 Hardware targets:"
	@echo "  build         - Build (UART0=commands, logging disabled)"
	@echo "  build-debug   - Build (UART0=logging, commands disabled)"
	@echo "  clean         - Clean build artifacts"
	@echo "  flash         - Flash firmware to device"
	@echo "  monitor       - Monitor serial output"
	@echo "  flash-monitor - Flash and monitor"
	@echo "  menuconfig    - Open configuration menu"
	@echo "  size          - Show size information"
	@echo "  erase         - Erase entire flash"
	@echo "  rebuild       - Clean and rebuild"
	@echo "  dev           - Build, flash, and monitor"
	@echo ""
	@echo "🎨 Code quality:"
	@echo "  format        - Format all C/C++ code"
	@echo "  format-check  - Check code formatting"
	@echo "  lint          - Run static analysis"
	@echo ""
	@echo "🌐 Web interface targets:"
	@echo "  web-dev       - Start development server (localhost:3000)"
	@echo "  web-build     - Build production web interface for SPIFFS"
	@echo ""
	@echo "🎮 Simulator targets:"
	@echo "  sim-build     - Build for Wokwi simulator"
	@echo "  sim           - Run simulation with serial output"
	@echo "  sim-debug     - Run with interactive serial input"
	@echo "  sim-screenshot- Take OLED screenshot after 5 seconds"
	@echo "  sim-web       - Instructions for web simulator"
	@echo "  sim-info      - Show simulator information"
	@echo "  check-wokwi   - Check Wokwi CLI installation"
	@echo ""
	@echo "🔧 Setup targets:"
	@echo "  setup-env     - Setup development environment"
	@echo "  set-target    - Set target to ESP32-S3 (run once)"
	@echo "  check-idf     - Check ESP-IDF installation"
	@echo "  env-info      - Show environment information"
	@echo ""
	@echo "💡 Quick start:"
	@echo "  make setup-env    # First time setup"
	@echo "  make set-target   # Set ESP32-S3 target"
	@echo "  make dev          # Build, flash, and monitor"
	@echo "  make web-dev      # Test web interface locally"
	@echo ""
	@echo "🎮 Simulator quick start:"
	@echo "  make sim          # Run local Wokwi simulation"
	@echo "  make sim-web      # Use web simulator instead"

# 🔍 Firmware Validation
.PHONY: check-firmware
check-firmware: ## Check firmware manifest (magic, board ID, version)
	@python3 tools/check_firmware.py build/heltec_v3.bin
