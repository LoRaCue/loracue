# Web UI Updates for Band and Slot Support

## Changes Made

### 1. Device Settings Page (`pages/index.tsx`)

**Added:**
- `slot_id` field to `DeviceSettings` interface (number, 1-16)
- Slot selector dropdown with 16 options
- Help text: "Select which PC this presenter controls (1-16)"

**UI Location:**
- Between "Auto Sleep" checkbox and "Sleep Timeout" field
- Dropdown shows "Slot 1" through "Slot 16"

### 2. LoRa Settings Page (`pages/lora.tsx`)

**Added:**
- `band_id` field to `LoRaSettings` interface (string)
- `Band` interface with id, name, center_khz, min_khz, max_khz
- Fetch bands from `/api/lora/bands` on page load
- Band selector dropdown (replaces hardcoded frequency options)
- Frequency range display below band selector
- Frequency input field (number, Hz) for fine-tuning

**UI Changes:**
- Band selector as first field (auto-updates frequency to band center)
- Frequency as second field (manual input with band limits shown)
- Removed hardcoded frequency dropdown
- Shows valid frequency range for selected band

## API Endpoints Required

### Device Settings
```
GET /api/device/settings
→ Returns: { name, mode, sleepTimeout, autoSleep, brightness, slot_id }

POST /api/device/settings
← Accepts: { name?, mode?, sleepTimeout?, autoSleep?, brightness?, slot_id? }
```

### LoRa Settings
```
GET /api/lora/settings
→ Returns: { frequency, spreading_factor, bandwidth, coding_rate, tx_power, band_id }

POST /api/lora/settings
← Accepts: { frequency?, spreading_factor?, bandwidth?, coding_rate?, tx_power?, band_id? }

GET /api/lora/bands
→ Returns: [{ id, name, center_khz, min_khz, max_khz }, ...]
```

## Backend Command Mapping

The web UI API endpoints should map to these USB/Bluetooth commands:

```
GET /api/device/settings  → GET_DEVICE_CONFIG
POST /api/device/settings → SET_DEVICE_CONFIG {slot_id: 1-16}

GET /api/lora/settings    → GET_LORA_CONFIG
POST /api/lora/settings   → SET_LORA_CONFIG {band_id: "HW_868", frequency: 868000000}

GET /api/lora/bands       → GET_LORA_BANDS
```

## User Experience

### Slot Selection
1. User opens Device Settings
2. Sees "Slot ID (Multi-PC Routing)" dropdown
3. Selects slot 1-16 based on which PC they want to control
4. Saves settings
5. Presenter now sends commands to that slot

### Band Selection
1. User opens LoRa Settings
2. Sees "Hardware Band" dropdown with available bands
3. Selects band (e.g., "868 MHz EU Band")
4. Frequency auto-updates to band center
5. Can fine-tune frequency within band limits
6. Sees valid range displayed below band selector
7. Saves settings
8. Backend validates frequency is within band limits

## Validation

### Frontend
- Slot: 1-16 (enforced by dropdown)
- Frequency: Number input (user can enter any value)

### Backend
- Slot: 1-16 validation in SET_DEVICE_CONFIG
- Frequency: Validated against band's min/max in SET_LORA_CONFIG
- Returns error with valid range if out of bounds

## Build and Deploy

```bash
cd web-interface
npm install
npm run build
npm run export
```

The static files in `out/` directory are served by the ESP32's HTTP server.
