# LoRaCue Implementation Roadmap

## Context-Loss Resilient Development Strategy

Each ticket is **self-contained** with complete context, requirements, and acceptance criteria.

## Phase 1: Hardware Abstraction Layer (HAL) - Foundation
**Duration: 1-2 weeks**

### Ticket 1.1: BSP Core Infrastructure
**Context**: Create hardware abstraction layer for Heltec LoRa V3
**Files**: `components/bsp/include/bsp.h`, `components/bsp/bsp_heltec_v3.c`
**Requirements**:
- GPIO initialization (buttons GPIO45/46 with pullups)
- ADC setup for battery monitoring (GPIO1, control GPIO37)
- Power management functions
- Hardware detection and validation
**Acceptance**: All BSP functions callable, hardware responds correctly

### Ticket 1.2: SPI Bus for LoRa (SX1262)
**Context**: Initialize SPI2_HOST for SX1262 LoRa chip communication
**Pin Mapping**: MOSI=11, MISO=10, SCK=9, CS=8, BUSY=13, DIO1=14, RST=12
**Requirements**:
- SPI bus configuration with correct timing
- GPIO setup for control pins
- Basic SX1262 register read/write functions
**Acceptance**: Can read SX1262 version register (0x14)

### Ticket 1.3: I2C Bus for OLED (SH1106)
**Context**: Initialize I2C0 for SH1106 OLED display
**Pin Mapping**: SDA=17, SCL=18
**Requirements**:
- I2C master configuration
- Display initialization sequence
- Basic pixel/text drawing functions
**Acceptance**: Can display "LoRaCue" text on OLED

## Phase 2: Core Communication - LoRa Point-to-Point
**Duration: 2-3 weeks**

### Ticket 2.1: LoRa Driver Integration
**Context**: Integrate nopnop2002/esp-idf-sx126x driver
**Requirements**:
- Add as ESP-IDF component
- Configure for SF7/BW500kHz/CR4:5 (low latency)
- Basic send/receive functions
**Acceptance**: Two boards can exchange simple packets

### Ticket 2.2: Custom Protocol Implementation
**Context**: Implement secure packet structure from blueprint Table 3
**Packet Structure**: DeviceID(2) + SequenceNum(2) + Command(1) + Payload(0-7) + MAC(4)
**Requirements**:
- Packet serialization/deserialization
- Sequence number management
- Command definitions (NEXT, PREV, BLACK_SCREEN)
**Acceptance**: Structured packets sent/received correctly

### Ticket 2.3: AES Encryption Layer
**Context**: Hardware-accelerated AES-128 encryption using ESP32-S3 crypto
**Requirements**:
- Key management in NVS
- Encrypt/decrypt packet payload
- MAC generation for integrity
**Acceptance**: Encrypted packets exchanged, invalid packets rejected

## Phase 3: USB HID Receiver
**Duration: 1-2 weeks**

### Ticket 3.1: TinyUSB HID Keyboard
**Context**: Implement USB composite device (HID + CDC)
**Requirements**:
- HID keyboard interface
- CDC serial interface for debugging
- Command to keycode mapping (Table 4 from blueprint)
**Acceptance**: PC recognizes device, keypresses work in presentation software

### Ticket 3.2: Multi-Sender Management
**Context**: Handle multiple paired STAGE devices
**Requirements**:
- Device registry in NVS
- Per-device AES keys
- Device identification by DeviceID
**Acceptance**: Multiple senders can control same receiver

## Phase 4: User Interface & Power Management
**Duration: 2-3 weeks**

### Ticket 4.1: OLED State Machine
**Context**: Implement UI state machine from blueprint Section 3.4.1
**States**: Main, Menu, Settings, Pairing, Name Editor
**Requirements**:
- Two-button navigation logic
- Menu hierarchy implementation
- Status display (battery, signal, pairing)
**Acceptance**: Full menu navigation works with two buttons

### Ticket 4.2: Power Management
**Context**: Implement energy-efficient operation
**Requirements**:
- Light sleep between operations
- Deep sleep after 5min inactivity
- Wake on button press
- Battery monitoring with voltage divider
**Acceptance**: >24h battery life in normal use

## Phase 5: Secure Pairing System
**Duration: 1-2 weeks**

### Ticket 5.1: USB-C Pairing Protocol
**Context**: Implement secure pairing via USB-C (Section 5.1)
**Requirements**:
- USB OTG role switching (PC=Host, STAGE=Device)
- Secure key exchange over USB CDC
- Device name and ID exchange
- NVS storage of paired devices
**Acceptance**: STAGE and PC can pair securely via USB-C

## Phase 6: Configuration & Updates
**Duration: 2-3 weeks**

### Ticket 6.1: Web Configuration Interface
**Context**: Wi-Fi AP mode with web interface (Section 5.2)
**Requirements**:
- HTTP server with configuration forms
- NVS read/write via web interface
- OTA update page
**Acceptance**: All settings configurable via web browser

### Ticket 6.2: Serial CLI Interface
**Context**: Command-line interface via USB CDC (Section 5.4)
**Requirements**:
- esp_console integration
- All configuration commands
- Scripting support
**Acceptance**: Full device configuration via serial commands

### Ticket 6.3: OTA Update System
**Context**: Secure firmware updates (Section 5.3)
**Requirements**:
- Signed firmware validation
- Rollback protection
- Web and USB update methods
**Acceptance**: Firmware updates work reliably with rollback safety

## Context-Loss Mitigation Strategies

### 1. Self-Contained Tickets
Each ticket includes:
- Complete context and background
- Exact file paths and function signatures
- Pin mappings and hardware details
- Acceptance criteria with test procedures

### 2. Reference Architecture
Maintain these always-current files:
- `docs/ARCHITECTURE.md` - System overview
- `docs/PIN_MAPPING.md` - Hardware connections
- `docs/PROTOCOL.md` - Communication protocol
- `docs/API_REFERENCE.md` - Function signatures

### 3. Incremental Validation
Each phase has working deliverables:
- Phase 1: Hardware responds
- Phase 2: Basic communication works
- Phase 3: USB HID functional
- Phase 4: Complete user experience
- Phase 5: Secure pairing
- Phase 6: Production-ready features

### 4. Automated Testing
- Unit tests for each BSP function
- Integration tests for communication
- Hardware-in-loop tests for full system
- CI/CD pipeline validates every change

This approach ensures any developer can pick up any ticket with full context and implement it successfully, even after extended breaks in development.
