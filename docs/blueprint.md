# LoRaCue - Enterprise LoRa Presentation Clicker Blueprint

## Projektübersicht

**LoRaCue** ist ein professioneller, drahtloser Präsentations-Clicker mit LoRa-Kommunikation für Reichweiten >100m und <50ms Latenz. Das System basiert auf ESP32-S3 Mikrocontrollern mit SX1262 LoRa-Transceivern und bietet enterprise-grade Sicherheit mit AES-128 Verschlüsselung.

### Kernspezifikationen
- **Reichweite**: >100 Meter (LoRa SF7/BW500kHz)
- **Latenz**: <50ms für Tastendruck-zu-Aktion
- **Sicherheit**: AES-128 Hardware-Verschlüsselung mit Replay-Schutz
- **Batterielaufzeit**: >24 Stunden kontinuierlicher Betrieb
- **Pairing**: Sichere USB-C Challenge-Response Authentifizierung
- **Updates**: Dual-OTA Firmware-Updates über WiFi
- **Konfiguration**: Web-Interface für Remote-Management

## Hardware-Architektur

### Hauptkomponenten
- **MCU**: ESP32-S3 (Dual-Core, 240MHz, 8MB PSRAM, 16MB Flash)
- **LoRa**: SX1262 Transceiver (Sub-GHz, bis zu +22dBm)
- **Display**: SH1106 OLED 128x64 (I2C)
- **Interface**: 2x Taster für Navigation
- **Power**: LiPo-Akku mit USB-C Ladung
- **Gehäuse**: Ergonomisches Design für Präsentationen

### Entwicklungsboard
**Heltec LoRa V3** (ESP32-S3 + SX1262 + SH1106 OLED)
- Integrierte Antenne und Batteriemanagement
- USB-C Programmierung und Ladung
- Alle erforderlichen Peripheriegeräte onboard

## Software-Architektur

### Board Support Package (BSP)
```
components/bsp/
├── include/bsp.h              # Hardware-Abstraktionsschicht
├── bsp_heltec_v3.c           # Heltec V3 spezifische Implementierung
└── bsp_simulator.c           # Wokwi Simulator Implementierung
```

**Zukunftssichere Architektur**: BSP ermöglicht Unterstützung mehrerer Hardware-Plattformen ohne Änderungen am Anwendungscode.

### Komponentenstruktur
```
components/
├── bsp/                      # Board Support Package
├── lora/                     # LoRa Kommunikation + WiFi Simulation
├── oled_ui/                  # Benutzeroberfläche
├── device_registry/          # Geräte-Management
├── power_mgmt/              # Energieverwaltung
├── usb_hid/                 # USB Keyboard Emulation
├── usb_pairing/             # Sichere Geräte-Paarung
└── web_config/              # WiFi Konfiguration
```

## Wokwi Simulator Integration

### Vollständige Systemsimulation
Das Projekt unterstützt **komplette Systemsimulation** in Wokwi ohne Hardware-Anforderungen:

**Simulator-Features**:
- ✅ **Vollständige UI-Simulation** - OLED Menü-Navigation
- ✅ **Button-Interaktion** - Zwei-Tasten Navigation mit Timing
- ✅ **Batterie-Monitoring** - Spannungsüberwachung via Potentiometer
- ✅ **Power Management** - Sleep-Modi Simulation
- ✅ **Multi-Device Testing** - Mehrere Simulatoren gleichzeitig
- ✅ **WiFi LoRa Simulation** - UDP Broadcast statt RF-Übertragung
- ✅ **Web-Konfiguration** - Vollständiges HTTP Interface
- ✅ **System-Logging** - Komplette ESP-IDF Log-Ausgabe

### Wokwi Circuit Design
**Professioneller Schaltplan** mit pin-genauer Heltec V3 Zuordnung:

```json
{
  "version": 1,
  "author": "Fabian Schmieder", 
  "editor": "wokwi",
  "parts": [
    {
      "type": "board-esp32-s3-devkitc-1",
      "id": "esp"
    },
    {
      "type": "wokwi-ssd1306", 
      "id": "oled"
    },
    // Buttons, LEDs, LoRa-Modul Simulation...
  ]
}
```

**Hardware-Mapping**:
- **OLED SSD1306**: SDA=GPIO17, SCL=GPIO18
- **Buttons**: PREV=GPIO46, NEXT=GPIO45 mit Pull-ups
- **Battery ADC**: GPIO1 mit Potentiometer-Simulation
- **Status LEDs**: Power=GPIO2, TX=GPIO3, RX=GPIO4
- **LoRa SPI**: SCK=GPIO9, MISO=GPIO10, MOSI=GPIO11, CS=GPIO8
- **LoRa Control**: RST=GPIO12, DIO1=GPIO14, BUSY=GPIO13

### WiFi LoRa Simulation

**Konzept**: Ersetze LoRa RF-Kommunikation durch WiFi UDP Broadcast für Multi-Device Testing ohne Hardware.

**Implementierung**: Direkte bedingte Kompilierung in LoRa-Treiber:

```c
// components/lora/lora_driver.c
esp_err_t lora_send_packet(const uint8_t *data, size_t length)
{
#ifdef SIMULATOR_BUILD
    // WiFi UDP Broadcast zu 192.168.4.255:8080
    return sendto(udp_socket, data, length, 0, &broadcast_addr, sizeof(broadcast_addr));
#else
    // Hardware LoRa SX1262 Übertragung
    return sx1262_transmit(data, length);
#endif
}
```

**WiFi Transport Details**:
- **Netzwerk**: "LoRaCue-Sim" (Passwort: "simulator")
- **Protokoll**: UDP Broadcast auf Port 8080
- **Paket-Format**: Identisch zu LoRa (22 Bytes)
- **Verschlüsselung**: Gleiche AES-128 wie Hardware
- **Multi-Device**: Alle Simulatoren im gleichen Netzwerk

**Build-Targets**:
```bash
make build      # Hardware LoRa (ohne SIMULATOR_BUILD)
make sim-build  # Simulator WiFi (mit -DSIMULATOR_BUILD=1)
make sim        # Wokwi Simulation mit WiFi Transport
```

**Vorteile**:
- ✅ **Gleiche API** - lora_send_packet() funktioniert in beiden Modi
- ✅ **Gleiche Protokoll-Logik** - Verschlüsselung, Sequenzierung, ACKs
- ✅ **Multi-Device Testing** - PC + Remote gleichzeitig testen
- ✅ **Keine Abstraktion** - Direkte bedingte Kompilierung
- ✅ **WiFi Wiederverwendung** - Nutzt web_config WiFi Infrastructure

### Produktions- vs. Simulator-Strategie

**Hardware Builds (Produktion)**:
```c
#ifndef SIMULATOR_BUILD
// WiFi NUR für Config/Firmware Modi
// LoRa nutzt echte SX1262 Hardware
// Maximale Batterielaufzeit >24h
#endif
```

**Simulator Builds (Entwicklung)**:
```c
#ifdef SIMULATOR_BUILD  
// WiFi immer aktiv für LoRa Simulation
// Keine Energie-Beschränkungen in Wokwi
#endif
```

**Power-Optimierung Produktion**:
- ✅ **Normal-Betrieb**: WiFi komplett AUS, nur LoRa aktiv (~20mA)
- ✅ **Config-Modus**: WiFi AP on-demand (Benutzer-aktiviert, ~150mA)
- ✅ **Firmware-Update**: WiFi AP nur während OTA (~150mA)
- ✅ **Batterielaufzeit**: >24 Stunden kontinuierlicher Betrieb

**Entwicklungs-Flexibilität**:
- ✅ **Simulator**: WiFi permanent für Multi-Device Testing
- ✅ **Hardware**: Minimaler WiFi-Einsatz für maximale Effizienz
- ✅ **Gleicher Code**: Bedingte Kompilierung für beide Modi

## Entwicklungsworkflow

### Simulator-First Development
```bash
# 1. Entwicklung im Simulator
make sim                    # Vollständige Systemsimulation

# 2. Multi-Device Testing  
# Terminal 1: PC Empfänger
make sim
# Terminal 2: Remote Clicker
make sim

# 3. Hardware Deployment
make build flash monitor    # Auf echte Hardware
```

### Debugging Capabilities
- **Wokwi Web Interface**: Visuelle Komponenten-Interaktion
- **Serial Monitor**: Vollständige ESP-IDF Logs
- **Network Analysis**: Wireshark für WiFi Pakete
- **Web Configuration**: http://192.168.4.1 für Live-Konfiguration

## Sicherheitsarchitektur

### Verschlüsselungsprotokoll
```
Paket-Struktur (22 Bytes):
┌─────────────┬──────────────────────────────────┬─────────────┐
│ Device ID   │ AES-128 Encrypted Payload        │ HMAC-SHA256 │
│ (2 Bytes)   │ (16 Bytes)                       │ (4 Bytes)   │
└─────────────┴──────────────────────────────────┴─────────────┘

Encrypted Payload:
┌─────────────┬─────────────┬─────────────┬─────────────┐
│ Sequence    │ Command     │ Payload Len │ Payload     │
│ (2 Bytes)   │ (1 Byte)    │ (1 Byte)    │ (0-12 Bytes)│
└─────────────┴─────────────┴─────────────┴─────────────┘
```

### Pairing-Prozess
1. **USB-C Verbindung** zwischen PC und Remote
2. **Challenge-Response** Authentifizierung
3. **Schlüssel-Austausch** über sichere USB-Verbindung
4. **Device Registry** Speicherung in NVS Flash
5. **Automatische Wiederverbindung** bei bekannten Geräten

## Implementierungsphasen

### Phase 1: Foundation ✅
- [x] ESP-IDF v5.5 Projekt-Setup
- [x] BSP Architektur-Design
- [x] Build-System (Make, CMake, Partitionen)
- [x] CI/CD Pipeline (GitHub Actions)
- [x] Entwicklungsworkflow (Husky, Commitlint)

### Phase 2: Wokwi Simulation ✅
- [x] Vollständige Wokwi Circuit Integration
- [x] Pin-genaue Hardware-Simulation
- [x] WiFi LoRa Transport Simulation
- [x] Multi-Device Testing Capability
- [x] Professional Circuit Layout

### Phase 3: Core Communication
- [ ] LoRa Treiber-Implementierung (SX1262)
- [ ] Protokoll-Stack mit AES-128
- [ ] Device Registry System
- [ ] Paket-Routing und ACK-Handling

### Phase 4: User Interface
- [ ] OLED UI System (SH1106)
- [ ] Menü-Navigation (2-Tasten)
- [ ] Status-Anzeigen und Feedback
- [ ] Batterie-Monitoring UI

### Phase 5: USB Integration
- [ ] USB HID Keyboard Emulation
- [ ] Sichere USB-C Pairing
- [ ] Challenge-Response Protokoll
- [ ] Cross-Platform Treiber

### Phase 6: Enterprise Features
- [ ] WiFi Konfiguration (AP-Modus)
- [ ] Web-Interface für Management
- [ ] OTA Firmware-Updates
- [ ] Erweiterte Sicherheitsfeatures

## Testing Strategy

### Simulator Testing
```bash
# Vollständige Systemvalidierung ohne Hardware
make sim                    # Einzelgerät-Tests
make sim-multi             # Multi-Device Szenarien
make sim-screenshot        # UI Screenshot-Tests
```

### Hardware Testing
```bash
# Hardware-spezifische Tests
make test-hardware         # BSP Funktionalität
make test-lora            # RF Kommunikation
make test-power           # Energieverwaltung
```

### Integration Testing
- **Protocol Compliance**: Paket-Format Validierung
- **Security Testing**: Verschlüsselung und Replay-Schutz
- **Performance Testing**: Latenz und Reichweiten-Messungen
- **Compatibility Testing**: Multi-Platform USB HID

## Deployment

### Firmware Distribution
- **GitHub Releases** mit versionierten Binaries
- **OTA Updates** über WiFi Web-Interface
- **USB Flashing** für initiale Installation
- **Dual-Partition** für sichere Updates

### Produktionsbereitschaft
- **CE/FCC Zertifizierung** für LoRa Frequenzen
- **Gehäuse-Design** für ergonomische Nutzung
- **Batterie-Optimierung** für >24h Laufzeit
- **Qualitätssicherung** mit automatisierten Tests

## Technische Spezifikationen

### LoRa Konfiguration
- **Frequenz**: 915MHz (US) / 868MHz (EU)
- **Spreading Factor**: SF7 (niedrige Latenz)
- **Bandwidth**: 500kHz (hoher Durchsatz)
- **Coding Rate**: 4/5 (minimaler Overhead)
- **TX Power**: 14dBm (optimale Reichweite/Verbrauch)

### Power Management
- **Active Mode**: 80MHz CPU, WiFi/LoRa aktiv
- **Light Sleep**: 10MHz CPU, Peripherie aktiv
- **Deep Sleep**: RTC only, GPIO Wakeup
- **Batterie**: 3.7V LiPo, USB-C Ladung

### Speicher-Layout
```
Flash Partitionen (16MB):
├── Bootloader (32KB)
├── Partition Table (4KB) 
├── NVS (24KB)
├── OTA Data (8KB)
├── Factory App (6MB)
├── OTA App (6MB)
└── SPIFFS (4MB)
```

## Fazit

**LoRaCue** kombiniert moderne ESP32-S3 Hardware mit professioneller LoRa-Kommunikation und enterprise-grade Sicherheit. Die **Wokwi Simulator Integration** mit **WiFi LoRa Simulation** ermöglicht vollständige Systementwicklung und Multi-Device Testing ohne Hardware-Anforderungen.

Die **zukunftssichere BSP-Architektur** und **bedingte Kompilierung** für Simulator/Hardware Modi schaffen eine wartbare, skalierbare Codebasis für professionelle Präsentations-Hardware.

**Status**: Foundation und Simulation komplett implementiert, bereit für Core Communication Entwicklung.
