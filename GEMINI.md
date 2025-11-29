# LoRaCue Project Memory & Context

**Project:** LoRaCue
**Description:** Enterprise-grade wireless presentation remote with LoRa (SX1262), OLED/E-Paper display, and USB-HID.
**Platform:** ESP32-S3 (ESP-IDF v5.5)
**Language:** C17 (GNU17)

---

## 🏛️ Architecture & Architectural Decisions

### 1. Hardware Abstraction Layer (HAL)
*   **Strict Compile-Time Selection:** We do **not** use runtime hardware detection for pin definitions or driver selection.
*   **Mechanism:** We use **Kconfig** (`idf.py menuconfig`) to select the target board (e.g., `CONFIG_BOARD_HELTEC_V3`).
*   **BSP Component:** `components/bsp/` contains board-specific implementations (e.g., `bsp_heltec_v3.c`).
*   **Display Driver:** `components/display/CMakeLists.txt` uses Kconfig to compile *only* the driver needed for the selected board (e.g., `display_ssd1306.c` OR `display_ssd1681.c`). This saves flash and prevents linking errors.

### 2. Command & Control (The "Brain")
*   **Separation of Concerns:** We strictly separate the **Transport Layer** (JSON/UART/WiFi) from the **Business Logic**.
*   **API Component:** `components/commands/commands_api.c` (header: `commands_api.h`) is the **Single Source of Truth**.
    *   It exposes pure C functions (e.g., `cmd_set_general_config(config_t *cfg)`).
    *   It handles logic, validation, NVS persistence, and system events.
*   **JSON-RPC Adapter:** `components/commands/commands.c` is a **dumb adapter**.
    *   It parses JSON strings.
    *   It maps strings to C API calls.
    *   It serializes C results back to JSON.
    *   **Rule:** Never put business logic inside `commands.c`.

### 3. Event Bus
*   **Component:** `components/system_events/`
*   **Pattern:** Publish-Subscribe.
*   **Usage:** Decouples input (Button Manager) from output (LoRa TX, UI Update).
    *   Example: `BUTTON_PRESSED` -> `presenter_mode_manager` -> `LORA_TX` -> `UI_UPDATE`.

### 4. Build System
*   **Tool:** Makefile wrapper around `idf.py`.
*   **Usage:**
    *   `make build MODEL=alpha` (Heltec V3)
    *   `make build MODEL=gamma` (LilyGO T5)
    *   `make sim-run` (Wokwi Simulation)

---

## 🛠️ Supported Hardware

| Model Name | Board ID (Kconfig) | Hardware | Display | Features |
| :--- | :--- | :--- | :--- | :--- |
| **LC-Alpha** | `BOARD_HELTEC_V3` | Heltec WiFi LoRa 32 V3 | SSD1306 (OLED) | 1 Button (GPIO0) |
| **LC-Alpha+** | `BOARD_HELTEC_V3` | Heltec WiFi LoRa 32 V3 | SSD1306 (OLED) | 2 Ext. Buttons |
| **LC-Beta** | `BOARD_LILYGO_T3` | LilyGO T3-S3 | SSD1681 (E-Paper 2.13") | 2 Buttons |
| **LC-Gamma** | `BOARD_LILYGO_T5` | LilyGO T5-4.7 | SSD1681 (E-Paper 4.7") | Touch |

---

## 🔄 Recent Refactoring (Nov 2025)
*   **Refactor:** Removed custom `BOARD_ID` CMake variable in favor of Kconfig.
*   **Refactor:** Split `commands.c` into `commands.c` (adapter) and `commands_api.c` (logic).
*   **Feature:** Added `commands_api` for programmatic control (used by WiFi Config Server).
*   **Feature:** Added Unit Tests for Commands API (`tests/unit/test_commands.c`).

## ⚠️ Coding Conventions
*   **No String Parsing in Logic:** All parsing happens at the edge (Adapter). Logic functions take C structs.
*   **Thread Safety:** UI functions must be thread-safe or called from the UI task. System events are safe.
*   **Component Deps:** Use `REQUIRES` in `idf_component_register` carefully. Don't link E-Paper drivers if building for OLED.
