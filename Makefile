# LoRaCue Makefile with ESP-IDF Auto-Detection
# Automatically finds and sets up ESP-IDF environment

.PHONY: build clean flash monitor menuconfig size erase help check-idf setup-env

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

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	$(IDF_SETUP) idf.py clean

# Flash firmware to device
flash: check-idf
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

# Set target (run once after fresh clone)
set-target: check-idf
	@echo "🎯 Setting target to ESP32-S3..."
	$(IDF_SETUP) idf.py set-target esp32s3

# Full clean rebuild
rebuild: clean build

# Development cycle: build, flash, monitor
dev: build flash monitor

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
	@echo "📋 Available targets:"
	@echo "  build         - Build the project"
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
