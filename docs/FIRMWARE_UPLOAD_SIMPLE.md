# Simple Firmware Upload Protocol (No XMODEM)

## Problem with Current Implementation

XMODEM was designed for unreliable serial links. With USB CDC:
- **Reliable transport** - USB handles error correction
- **Flow control** - USB handles buffering
- **Complexity** - XMODEM adds unnecessary overhead

## Espressif's Solution

Use **internal ringbuffers** in USB CDC driver:
```c
usbh_cdc_device_config_t dev_config = {
    .rx_buffer_size = 2048,  // Internal ringbuffer
    .tx_buffer_size = 2048,
};
```

Data flows: `USB → Ringbuffer → Application`

## Proposed Simple Protocol

### Command
```
FIRMWARE_UPLOAD <size>\n
```

### Response
```
OK Ready\n
```

### Transfer
```
[size bytes of raw firmware data]
```

### Completion
```
OK Complete\n
```

## Implementation

### USB CDC Side (Already Has Ringbuffer)
```c
// usb_cdc.c already has ringbuffer via TinyUSB
void tud_cdc_rx_cb(uint8_t itf) {
    // Data automatically buffered
    while (tud_cdc_available()) {
        char c = tud_cdc_read_char();
        // Process...
    }
}
```

### Firmware Upload Handler
```c
static esp_err_t firmware_upload_simple(size_t size) {
    esp_err_t ret = ota_engine_start(size);
    if (ret != ESP_OK) return ret;
    
    size_t received = 0;
    uint8_t buf[512];
    
    while (received < size) {
        size_t to_read = MIN(sizeof(buf), size - received);
        size_t read = usb_cdc_read_bytes(buf, to_read, pdMS_TO_TICKS(5000));
        
        if (read == 0) {
            ota_engine_abort();
            return ESP_ERR_TIMEOUT;
        }
        
        ret = ota_engine_write(buf, read);
        if (ret != ESP_OK) {
            ota_engine_abort();
            return ret;
        }
        
        received += read;
    }
    
    return ota_engine_finish();
}
```

## Advantages

1. **Simple** - No protocol overhead
2. **Fast** - No ACK/NAK delays
3. **Reliable** - USB handles errors
4. **Small** - Less code
5. **Works** - No reset issues

## Migration

1. Remove `xmodem.c` and `xmodem.h`
2. Add simple binary read in `commands.c`
3. Update LoRaCueManager to send raw binary
4. Done!
