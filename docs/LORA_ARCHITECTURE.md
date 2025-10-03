# LoRa Architecture - Separation of Concerns

## Overview

The LoRa subsystem follows a clean 3-layer architecture with proper separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER                         │
│                      (main.c)                                │
│  • Command-to-action mapping (LoRa → USB HID)               │
│  • Button-to-command mapping (Button → LoRa)                │
│  • UI updates and user feedback                             │
└─────────────────────────────────────────────────────────────┘
                              ↕ Callbacks
┌─────────────────────────────────────────────────────────────┐
│                  COMMUNICATION LAYER                         │
│                   (lora_comm)                                │
│  • Generic command receive/send with callbacks              │
│  • Connection state monitoring                              │
│  • No application-specific logic                            │
│  • Reusable for any LoRa application                        │
└─────────────────────────────────────────────────────────────┘
                              ↕ Protocol API
┌─────────────────────────────────────────────────────────────┐
│                   PROTOCOL LAYER                             │
│                  (lora_protocol)                             │
│  • Packet format and encryption (AES-256)                   │
│  • Sequence numbers and replay protection                   │
│  • ACK/retry mechanism                                      │
│  • RSSI monitoring and connection quality                   │
└─────────────────────────────────────────────────────────────┘
                              ↕ Driver API
┌─────────────────────────────────────────────────────────────┐
│                    DRIVER LAYER                              │
│                   (lora_driver)                              │
│  • SX1262 hardware control (SPI)                            │
│  • Low-level radio configuration                            │
│  • TX/RX buffer management                                  │
└─────────────────────────────────────────────────────────────┘
```

## Layer Responsibilities

### 1. Driver Layer (`lora_driver`)
**Purpose**: Hardware abstraction for SX1262 LoRa transceiver

**Responsibilities**:
- SPI communication with SX1262
- Radio configuration (frequency, spreading factor, bandwidth)
- TX/RX buffer management
- Interrupt handling

**Dependencies**: ESP-IDF HAL only

**API Example**:
```c
esp_err_t lora_driver_init(void);
esp_err_t lora_driver_send(const uint8_t *data, size_t len);
esp_err_t lora_driver_receive(uint8_t *data, size_t *len, uint32_t timeout_ms);
```

### 2. Protocol Layer (`lora_protocol`)
**Purpose**: LoRaCue custom protocol implementation

**Responsibilities**:
- Packet structure: `DeviceID(2) + Encrypted[SeqNum(2) + Cmd(1) + Payload(0-7)] + MAC(4)`
- AES-256 encryption/decryption
- Sequence number management and replay protection
- ACK/retry mechanism for reliable delivery
- RSSI monitoring and connection quality assessment

**Dependencies**: `lora_driver`, mbedTLS

**API Example**:
```c
esp_err_t lora_protocol_init(uint16_t device_id, const uint8_t *aes_key);
esp_err_t lora_protocol_send_command(lora_command_t cmd, const uint8_t *payload, uint8_t len);
esp_err_t lora_protocol_send_reliable(lora_command_t cmd, const uint8_t *payload, uint8_t len, uint32_t timeout_ms, uint8_t retries);
esp_err_t lora_protocol_receive_packet(lora_packet_data_t *packet, uint32_t timeout_ms);
lora_connection_state_t lora_protocol_get_connection_state(void);
```

### 3. Communication Layer (`lora_comm`)
**Purpose**: Generic LoRa communication with callback-based event system

**Responsibilities**:
- Receive task management
- Callback dispatch for received commands
- Connection state change notifications
- Simple send API wrapper
- **NO application-specific logic** (no USB HID, no command mapping)

**Dependencies**: `lora_protocol`

**API Example**:
```c
// Callback types
typedef void (*lora_comm_rx_callback_t)(lora_command_t cmd, const uint8_t *payload, uint8_t len, int16_t rssi, void *ctx);
typedef void (*lora_comm_state_callback_t)(lora_connection_state_t state, void *ctx);

// API
esp_err_t lora_comm_init(void);
esp_err_t lora_comm_register_rx_callback(lora_comm_rx_callback_t callback, void *ctx);
esp_err_t lora_comm_register_state_callback(lora_comm_state_callback_t callback, void *ctx);
esp_err_t lora_comm_send_command(lora_command_t cmd, const uint8_t *payload, uint8_t len);
esp_err_t lora_comm_send_command_reliable(lora_command_t cmd, const uint8_t *payload, uint8_t len);
esp_err_t lora_comm_start(void);
esp_err_t lora_comm_stop(void);
```

### 4. Application Layer (`main.c`)
**Purpose**: Application-specific logic and integration

**Responsibilities**:
- **LoRa → USB HID mapping**: Translate received LoRa commands to keyboard actions
- **Button → LoRa mapping**: Translate button events to LoRa commands
- **UI updates**: Update OLED display based on connection state
- **User feedback**: LED patterns, status messages

**Dependencies**: `lora_comm`, `usb_hid`, `button_manager`, `oled_ui`

**Implementation Example**:
```c
// LoRa command received → Send USB HID keypress
static void lora_rx_handler(lora_command_t cmd, const uint8_t *payload, uint8_t len, int16_t rssi, void *ctx)
{
    usb_hid_keycode_t keycode;
    switch (cmd) {
        case CMD_NEXT_SLIDE:      keycode = HID_KEY_PAGE_DOWN; break;
        case CMD_PREV_SLIDE:      keycode = HID_KEY_PAGE_UP; break;
        case CMD_BLACK_SCREEN:    keycode = HID_KEY_B; break;
        case CMD_START_PRESENTATION: keycode = HID_KEY_F5; break;
        default: return;
    }
    usb_hid_send_key(keycode);
}

// Connection state changed → Update UI
static void lora_state_handler(lora_connection_state_t state, void *ctx)
{
    oled_status_t *status = (oled_status_t *)ctx;
    status->lora_connected = (state != LORA_CONNECTION_LOST);
    status->lora_signal = (state == LORA_CONNECTION_EXCELLENT) ? 100 : 
                          (state == LORA_CONNECTION_GOOD) ? 75 :
                          (state == LORA_CONNECTION_WEAK) ? 50 : 25;
    oled_ui_update_status(status);
}

// Button pressed → Send LoRa command
static void button_handler(button_event_type_t event, void *arg)
{
    lora_command_t cmd;
    switch (event) {
        case BUTTON_EVENT_NEXT_SHORT: cmd = CMD_NEXT_SLIDE; break;
        case BUTTON_EVENT_PREV_SHORT: cmd = CMD_PREV_SLIDE; break;
        case BUTTON_EVENT_NEXT_LONG:  cmd = CMD_START_PRESENTATION; break;
        case BUTTON_EVENT_PREV_LONG:  cmd = CMD_BLACK_SCREEN; break;
        default: return;
    }
    lora_comm_send_command_reliable(cmd, NULL, 0);
}

// Register callbacks during initialization
void app_main(void) {
    // ... initialization ...
    lora_comm_register_rx_callback(lora_rx_handler, &status);
    lora_comm_register_state_callback(lora_state_handler, &status);
    button_manager_register_callback(button_handler, NULL);
    lora_comm_start();
}
```

## Benefits of This Architecture

### ✅ Proper Separation of Concerns
- Each layer has a single, well-defined responsibility
- No cross-layer dependencies (only downward dependencies)
- Clear interfaces between layers

### ✅ Reusability
- `lora_comm` can be reused for any LoRa application (not just USB HID)
- Easy to add new command handlers without modifying `lora_comm`
- Protocol layer is independent of application logic

### ✅ Testability
- Each layer can be tested independently
- Mock callbacks for testing `lora_comm`
- Mock protocol layer for testing application logic

### ✅ Maintainability
- Changes to USB HID mapping don't affect `lora_comm`
- New commands only require application layer changes
- Clear code organization and responsibilities

### ✅ Flexibility
- Easy to add multiple command handlers (e.g., USB HID + MQTT + BLE)
- Simple to implement different device modes (presenter vs receiver)
- Callback pattern allows dynamic behavior changes

## Command Flow Examples

### Receiving a Command (Receiver Mode)
```
1. SX1262 interrupt → lora_driver_receive()
2. lora_protocol_receive_packet() → decrypt, validate, extract command
3. lora_comm receive task → dispatch to rx_callback
4. lora_rx_handler() in main.c → usb_hid_send_key()
5. Computer receives keypress
```

### Sending a Command (Presenter Mode)
```
1. Button press → button_manager → button_handler() in main.c
2. button_handler() → lora_comm_send_command_reliable()
3. lora_protocol_send_reliable() → encrypt, add MAC, send
4. lora_driver_send() → SX1262 transmits
5. Wait for ACK, retry if needed
```

## Migration Notes

### Old Architecture (❌ Problematic)
```c
// lora_comm.c - TIGHTLY COUPLED TO USB HID
static void handle_lora_command(lora_command_t command) {
    usb_hid_keycode_t keycode;
    switch (command) {
        case CMD_NEXT_SLIDE: keycode = HID_KEY_PAGE_DOWN; break;
        // ...
    }
    usb_hid_send_key(keycode);  // ❌ Direct USB HID dependency
}
```

### New Architecture (✅ Clean)
```c
// lora_comm.c - GENERIC, NO USB HID DEPENDENCY
static void lora_receive_task(void *pvParameters) {
    lora_packet_data_t packet;
    if (lora_protocol_receive_packet(&packet, 1000) == ESP_OK) {
        if (rx_callback) {
            rx_callback(packet.command, packet.payload, packet.payload_length, rssi, rx_callback_ctx);
        }
    }
}

// main.c - APPLICATION LOGIC
static void lora_rx_handler(lora_command_t cmd, ...) {
    // Map command to USB HID keycode
    usb_hid_send_key(keycode);
}
```

## Future Extensions

This architecture makes it easy to add:

1. **Multiple Output Handlers**: Register multiple callbacks for the same command
   ```c
   lora_comm_register_rx_callback(usb_hid_handler, NULL);
   lora_comm_register_rx_callback(mqtt_handler, NULL);
   lora_comm_register_rx_callback(ble_handler, NULL);
   ```

2. **Dynamic Command Mapping**: Change behavior at runtime
   ```c
   if (presentation_mode) {
       lora_comm_register_rx_callback(presentation_handler, NULL);
   } else {
       lora_comm_register_rx_callback(media_player_handler, NULL);
   }
   ```

3. **Command Filtering**: Add middleware layer for command validation
   ```c
   static void filtered_rx_handler(lora_command_t cmd, ...) {
       if (is_command_allowed(cmd)) {
           actual_handler(cmd, ...);
       }
   }
   ```

4. **Logging and Analytics**: Track command usage without modifying core logic
   ```c
   static void logging_rx_handler(lora_command_t cmd, ...) {
       log_command_received(cmd, rssi);
       main_rx_handler(cmd, ...);
   }
   ```

## Conclusion

The refactored architecture achieves **100% separation of concerns** with:
- ✅ Generic, reusable `lora_comm` component
- ✅ Application logic isolated in `main.c`
- ✅ Callback-based event system for flexibility
- ✅ No cross-layer dependencies
- ✅ Easy to test, maintain, and extend
