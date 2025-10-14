# OTA System Refactor

## Problem Analysis

### Current Implementation Issues (Causes Guru Meditation)

**Root Cause:** Breaks ESP32 flash controller's expectation of continuous writes

1. **Per-chunk HTTP requests** - Each 4KB chunk is a separate POST request
   - Creates connection overhead between writes
   - Breaks sequential write assumption
   - Flash controller loses write context

2. **20ms delays between writes** - `vTaskDelay(pdMS_TO_TICKS(20))` after each `esp_ota_write()`
   - Flash controller expects continuous stream
   - 20ms × 100+ chunks = 2+ seconds of delays
   - Causes flash cache coherency issues

3. **malloc() per chunk** - Heap allocation for each chunk
   - Heap fragmentation risk
   - Memory allocation can fail under load
   - Increases memory pressure during flash operations

4. **Base64 encoding overhead** - 25% size increase + CPU intensive decoding
   - Decoding during flash writes interferes with flash operations
   - Extra memory copies

5. **Commands system overhead** - JSON parsing between chunks
   - Adds latency during critical flash operations

### ESP-IDF Example Pattern (Working)

✅ Single continuous HTTP stream  
✅ Immediate writes after reading data  
✅ No artificial delays  
✅ Stack buffer (no malloc)  
✅ All in one task (no context switches)

---

## Solution: Simplified Transport-Agnostic OTA

### Core Design Principles

1. **Direct writes** - Transport → `esp_ota_write()` immediately
2. **No delays** - Continuous write loop
3. **Binary transfer** - No base64 encoding
4. **Single connection** - Per transport session
5. **Unified engine** - Same core for WiFi/USB/BLE

### Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│              Transport Layer                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐           │
│  │   WiFi   │ │ USB-CDC  │ │   BLE    │           │
│  │  (HTTP)  │ │ (XMODEM) │ │  (GATT)  │           │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘           │
│       │            │             │                   │
│       └────────────┴─────────────┘                   │
│                    │                                  │
│         ┌──────────▼──────────┐                     │
│         │    OTA Engine       │                     │
│         │  (State Machine)    │                     │
│         └──────────┬──────────┘                     │
│                    │                                  │
│         ┌──────────▼──────────┐                     │
│         │  esp_ota_write()    │                     │
│         │  (Direct, No Delay) │                     │
│         └─────────────────────┘                     │
└─────────────────────────────────────────────────────┘
```

### Key Improvements

| Current | Proposed |
|---------|----------|
| ❌ HTTP request per chunk | ✅ Single stream/connection |
| ❌ 20ms delay per chunk | ✅ No delays - continuous write |
| ❌ malloc() per chunk | ✅ Stack buffer |
| ❌ Base64 encoding | ✅ Binary transfer |
| ❌ Commands system overhead | ✅ Direct OTA engine calls |

---

## Implementation Plan

### **Phase 1: Core OTA Engine** (4-6 hours)

#### Create Component Structure
```
components/ota_engine/
├── include/ota_engine.h
├── ota_engine.c
└── CMakeLists.txt
```

#### Public API
```c
// ota_engine.h
esp_err_t ota_engine_start(size_t firmware_size);
esp_err_t ota_engine_write(const uint8_t *data, size_t len);
esp_err_t ota_engine_finish(void);
esp_err_t ota_engine_abort(void);
size_t ota_engine_get_progress(void);
bool ota_engine_is_active(void);
```

#### Core Implementation
```c
// ota_engine.c - Simplified, always requires size
static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *ota_partition = NULL;
static size_t ota_received_bytes = 0;
static size_t ota_total_size = 0;

esp_err_t ota_engine_start(size_t firmware_size)
{
    if (firmware_size == 0) {
        ESP_LOGE(TAG, "Firmware size must be specified");
        return ESP_ERR_INVALID_ARG;
    }
    
    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        ESP_LOGE(TAG, "No OTA partition available");
        return ESP_FAIL;
    }
    
    if (firmware_size > ota_partition->size) {
        ESP_LOGE(TAG, "Firmware too large: %zu > %zu", firmware_size, ota_partition->size);
        return ESP_ERR_INVALID_SIZE;
    }
    
    ota_total_size = firmware_size;
    ota_received_bytes = 0;
    
    return esp_ota_begin(ota_partition, firmware_size, &ota_handle);
}

esp_err_t ota_engine_write(const uint8_t *data, size_t len)
{
    if (!ota_handle) return ESP_ERR_INVALID_STATE;
    
    // Direct write - NO delays, NO buffering
    esp_err_t ret = esp_ota_write(ota_handle, data, len);
    if (ret == ESP_OK) {
        ota_received_bytes += len;
    }
    return ret;
}

esp_err_t ota_engine_finish(void)
{
    if (!ota_handle) return ESP_ERR_INVALID_STATE;
    
    // Verify we received expected amount
    if (ota_received_bytes != ota_total_size) {
        ESP_LOGW(TAG, "Size mismatch: received %zu, expected %zu", 
                 ota_received_bytes, ota_total_size);
    }
    
    esp_err_t ret = esp_ota_end(ota_handle);
    if (ret != ESP_OK) {
        ota_handle = 0;
        return ret;
    }
    
    ret = esp_ota_set_boot_partition(ota_partition);
    ota_handle = 0;
    return ret;
}

esp_err_t ota_engine_abort(void)
{
    if (ota_handle) {
        esp_ota_abort(ota_handle);
        ota_handle = 0;
    }
    ota_received_bytes = 0;
    ota_total_size = 0;
    return ESP_OK;
}

size_t ota_engine_get_progress(void)
{
    if (ota_total_size > 0) {
        return (ota_received_bytes * 100) / ota_total_size;
    }
    return 0;
}

bool ota_engine_is_active(void)
{
    return ota_handle != 0;
}
```

---

### **Phase 2: WiFi Transport Refactor** (3-4 hours)

#### Backend: Single Streaming Endpoint

**File:** `components/config_wifi_server/config_wifi_server.c`

**Remove these endpoints:**
- ❌ `/api/firmware/start`
- ❌ `/api/firmware/data`
- ❌ `/api/firmware/verify`
- ❌ `/api/firmware/commit`
- ❌ `/api/firmware/abort`

**Add single endpoint:**
```c
static esp_err_t api_firmware_upload_handler(httpd_req_t *req)
{
    size_t content_length = req->content_len;
    
    // Start OTA
    esp_err_t ret = ota_engine_start(content_length);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA start failed");
        return ESP_FAIL;
    }
    
    // Stream binary data directly
    uint8_t buffer[4096];
    size_t received = 0;
    
    while (received < content_length) {
        int len = httpd_req_recv(req, (char *)buffer, sizeof(buffer));
        if (len <= 0) {
            ota_engine_abort();
            return ESP_FAIL;
        }
        
        // Direct write - no delays
        ret = ota_engine_write(buffer, len);
        if (ret != ESP_OK) {
            ota_engine_abort();
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
            return ESP_FAIL;
        }
        
        received += len;
    }
    
    // Finalize
    ret = ota_engine_finish();
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validation failed");
        return ESP_FAIL;
    }
    
    httpd_resp_send(req, "OK", 2);
    
    // Reboot
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    
    return ESP_OK;
}

// Register endpoint
httpd_uri_t api_firmware_upload = {
    .uri = "/api/firmware/upload",
    .method = HTTP_POST,
    .handler = api_firmware_upload_handler
};
httpd_register_uri_handler(server, &api_firmware_upload);
```

#### Frontend: XMLHttpRequest with Progress

**File:** `web-interface/pages/firmware.tsx`

**Replace chunking logic:**
```typescript
const handleUpload = async () => {
  if (!file) return
  
  setUploading(true)
  setProgress(0)
  
  return new Promise((resolve, reject) => {
    const xhr = new XMLHttpRequest()
    
    // Progress tracking (native browser support)
    xhr.upload.addEventListener('progress', (e) => {
      if (e.lengthComputable) {
        const percent = Math.round((e.loaded / e.total) * 100)
        setProgress(percent)
      }
    })
    
    xhr.addEventListener('load', () => {
      if (xhr.status === 200) {
        toast.success('Firmware uploaded! Device rebooting...')
        resolve(null)
      } else {
        toast.error('Upload failed')
        reject(new Error('Upload failed'))
      }
      setUploading(false)
    })
    
    xhr.addEventListener('error', () => {
      toast.error('Network error')
      reject(new Error('Network error'))
      setUploading(false)
    })
    
    // Send binary file directly (no base64, no chunking)
    xhr.open('POST', '/api/firmware/upload')
    xhr.setRequestHeader('Content-Type', 'application/octet-stream')
    xhr.send(file)
  })
}
```

---

### **Phase 3: Remove Old OTA Code** (1 hour)

**File:** `components/commands/commands.c`

**Remove:**
- All `handle_fw_update_*` functions
- OTA state variables (`ota_handle`, `ota_partition`, `ota_received_bytes`, etc.)
- Base64 decoding logic
- Manifest checking (move to ota_engine if needed)

---

### **Phase 4: USB-CDC Transport with YMODEM** (4-5 hours)

#### Component Structure
```
components/usb_ota/
├── include/usb_ota.h
├── usb_ota.c
├── ymodem.c
├── ymodem.h
└── CMakeLists.txt
```

#### YMODEM Protocol
```c
// ymodem.h
#define YMODEM_SOH  0x01  // Start of 128-byte block (header)
#define YMODEM_STX  0x02  // Start of 1024-byte block (data)
#define YMODEM_EOT  0x04  // End of transmission
#define YMODEM_ACK  0x06  // Acknowledge
#define YMODEM_NAK  0x15  // Negative acknowledge
#define YMODEM_CAN  0x18  // Cancel
#define YMODEM_C    0x43  // 'C' - Request CRC mode

typedef struct {
    char filename[64];
    size_t filesize;
} ymodem_file_info_t;

typedef esp_err_t (*ymodem_data_cb_t)(const uint8_t *data, size_t len);
typedef int (*ymodem_read_cb_t)(uint8_t *buf, size_t len, uint32_t timeout_ms);
typedef int (*ymodem_write_cb_t)(const uint8_t *buf, size_t len);

typedef struct {
    ymodem_read_cb_t read;
    ymodem_write_cb_t write;
    ymodem_data_cb_t on_data;
    ymodem_file_info_t *file_info;  // Filled by ymodem_receive()
} ymodem_config_t;

esp_err_t ymodem_receive(const ymodem_config_t *config);
```

#### USB-CDC Handler
```c
// usb_ota.c
static esp_err_t usb_data_cb(const uint8_t *data, size_t len)
{
    return ota_engine_write(data, len);
}

void usb_ota_task(void *param)
{
    ESP_LOGI(TAG, "USB OTA task started");
    
    while (1) {
        // Wait for "FIRMWARE_UPGRADE" command from command parser
        char cmd[64];
        read_line(cmd, sizeof(cmd));
        
        if (strcmp(cmd, "FIRMWARE_UPGRADE") == 0) {
            ESP_LOGI(TAG, "Entering firmware upgrade mode...");
            send_response("OK Ready for YMODEM transfer\n");
            
            ymodem_file_info_t file_info = {0};
            ymodem_config_t config = {
                .read = usb_read_cb,
                .write = usb_write_cb,
                .on_data = usb_data_cb,
                .file_info = &file_info
            };
            
            // Receive file via YMODEM (gets size from header)
            if (ymodem_receive(&config) == ESP_OK) {
                ESP_LOGI(TAG, "Received: %s (%zu bytes)", 
                         file_info.filename, file_info.filesize);
                
                // Start OTA with size from YMODEM header
                if (ota_engine_start(file_info.filesize) == ESP_OK) {
                    // Data already written via callback
                    if (ota_engine_finish() == ESP_OK) {
                        ESP_LOGI(TAG, "Firmware upgrade successful, rebooting...");
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        esp_restart();
                    }
                }
            }
            
            ota_engine_abort();
            ESP_LOGE(TAG, "Firmware upgrade failed");
        }
    }
}
```

#### Client Tools

**Python Tool (Development/Testing):**
```python
# tools/usb_ota_upload.py
import serial
import os
from pathlib import Path

def crc16_xmodem(data):
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
            crc &= 0xFFFF
    return crc

def send_ymodem(port, firmware_path):
    ser = serial.Serial(port, 115200, timeout=3)
    
    filename = os.path.basename(firmware_path)
    filesize = Path(firmware_path).stat().st_size
    
    print(f"Uploading {filename} ({filesize} bytes)...")
    
    # Send "FIRMWARE_UPGRADE" command
    ser.write(b"FIRMWARE_UPGRADE\n")
    response = ser.readline().decode().strip()
    if "OK" not in response:
        print(f"Device not ready: {response}")
        return False
    
    # Wait for 'C' (CRC mode request)
    while ser.read(1) != b'C':
        pass
    
    # Send Block 0 (header with filename and size)
    header = f"{filename}\x00{filesize}\x00".encode()
    header += b'\x00' * (128 - len(header))  # Pad to 128 bytes
    
    packet = bytes([0x01, 0x00, 0xFF])  # SOH, block 0, ~block 0
    packet += header
    crc = crc16_xmodem(header)
    packet += bytes([crc >> 8, crc & 0xFF])
    
    ser.write(packet)
    if ser.read(1) != b'\x06':  # Wait for ACK
        print("Header rejected")
        return False
    
    # Wait for next 'C'
    while ser.read(1) != b'C':
        pass
    
    # Send data blocks
    with open(firmware_path, 'rb') as f:
        block_num = 1
        bytes_sent = 0
        
        while True:
            data = f.read(1024)
            if not data:
                break
            
            # Pad last block
            if len(data) < 1024:
                data += b'\x00' * (1024 - len(data))
            
            # Build packet: [STX][Block#][~Block#][Data][CRC16]
            packet = bytes([0x02, block_num & 0xFF, (0xFF - block_num) & 0xFF])
            packet += data
            crc = crc16_xmodem(data)
            packet += bytes([crc >> 8, crc & 0xFF])
            
            ser.write(packet)
            
            response = ser.read(1)
            if response == b'\x06':  # ACK
                bytes_sent += len(data)
                progress = (bytes_sent / filesize) * 100
                print(f"Progress: {bytes_sent}/{filesize} bytes ({progress:.1f}%)")
                block_num = (block_num + 1) % 256
            elif response == b'\x15':  # NAK
                print(f"Block {block_num} rejected, retrying...")
                f.seek(-1024, 1)
            else:
                print(f"Unexpected response: {response.hex()}")
                return False
    
    # Send EOT
    ser.write(b'\x04')
    if ser.read(1) != b'\x06':
        print("EOT not acknowledged")
        return False
    
    # Send final empty block (YMODEM protocol)
    ser.write(b'\x04')
    if ser.read(1) != b'\x06':
        print("Final EOT not acknowledged")
        return False
    
    print("Upload complete! Device will reboot.")
    return True

if __name__ == '__main__':
    import sys
    if len(sys.argv) != 3:
        print("Usage: python usb_ota_upload.py <port> <firmware.bin>")
        sys.exit(1)
    
    success = send_ymodem(sys.argv[1], sys.argv[2])
    sys.exit(0 if success else 1)
```

**Rust Tool (LoRaCueManager):**

> **Note:** For the LoRaCueManager desktop application, use the [`rmodem`](https://crates.io/crates/rmodem) Rust crate which supports both XMODEM and YMODEM protocols with proper error handling and async support.

```rust
// Example usage in LoRaCueManager
use rmodem::{Ymodem, YmodemMode};

async fn upload_firmware(port: &mut SerialPort, firmware_path: &Path) -> Result<()> {
    // Send OTA command
    port.write_all(b"OTA\n").await?;
    
    // Use YMODEM protocol (includes filename and size automatically)
    let mut ymodem = Ymodem::new();
    ymodem.send_file(port, firmware_path).await?;
    
    Ok(())
}
```

---

### **Phase 5: BLE Transport** (4-5 hours)

#### BLE Service Design
```
Service UUID: 0x1234 (OTA Service)
├── Control Characteristic (0x1235) - Write
│   Commands: "FIRMWARE_UPGRADE <size>", "ABORT"
├── Data Characteristic (0x1236) - Write
│   Binary chunks (up to MTU size)
└── Status Characteristic (0x1237) - Notify
    Progress updates (percentage)
```

#### Implementation
```c
// ble_ota.c
static void ble_control_write_handler(const uint8_t *data, uint16_t len)
{
    // Parse "FIRMWARE_UPGRADE <size>" command
    if (len > 17 && memcmp(data, "FIRMWARE_UPGRADE ", 17) == 0) {
        size_t firmware_size = atoi((char*)data + 17);
        
        if (ota_engine_start(firmware_size) == ESP_OK) {
            ESP_LOGI(TAG, "BLE firmware upgrade started: %zu bytes", firmware_size);
            uint8_t status = 0x00;  // OK
            esp_ble_gatts_send_indicate(..., 1, &status, false);
        } else {
            uint8_t status = 0xFF;  // ERROR
            esp_ble_gatts_send_indicate(..., 1, &status, false);
        }
    } else if (len == 5 && memcmp(data, "ABORT", 5) == 0) {
        ota_engine_abort();
    }
}

static void ble_data_write_handler(const uint8_t *data, uint16_t len)
{
    if (ota_engine_write(data, len) == ESP_OK) {
        // Send progress notification (0-100%)
        uint8_t progress = (uint8_t)ota_engine_get_progress();
        esp_ble_gatts_send_indicate(..., 1, &progress, false);
    } else {
        ota_engine_abort();
        uint8_t status = 0xFF;  // ERROR
        esp_ble_gatts_send_indicate(..., 1, &status, false);
    }
}

// Called when all data received
static void ble_ota_finish(void)
{
    if (ota_engine_finish() == ESP_OK) {
        ESP_LOGI(TAG, "BLE firmware upgrade successful, rebooting...");
        uint8_t status = 0x64;  // 100% complete
        esp_ble_gatts_send_indicate(..., 1, &status, false);
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        uint8_t status = 0xFF;  // ERROR
        esp_ble_gatts_send_indicate(..., 1, &status, false);
    }
}
```

#### Mobile App Flow
```
1. Get firmware file size
2. Write "FIRMWARE_UPGRADE <size>" to Control characteristic
3. Wait for Status notification (0x00 = OK)
4. Write firmware data in MTU-sized chunks to Data characteristic
5. Receive progress notifications (0-100%)
6. Device reboots automatically when complete
```

---

## Testing Checklist

### WiFi OTA
- [ ] Upload 2MB firmware in <30s without crash
- [ ] Progress bar updates smoothly
- [ ] Invalid firmware rejected
- [ ] Disconnect mid-upload handled gracefully
- [ ] Power loss doesn't brick device

### USB-CDC/UART OTA
- [ ] "FIRMWARE_UPGRADE" command triggers OTA mode
- [ ] YMODEM transfer completes successfully
- [ ] CRC errors detected and retried
- [ ] Unplug during upload aborts cleanly
- [ ] Filename and size parsed from YMODEM header

### BLE OTA
- [ ] "FIRMWARE_UPGRADE <size>" command accepted
- [ ] Small MTU chunks work correctly
- [ ] Connection loss handled
- [ ] Progress notifications sent (0-100%)
- [ ] Device reboots on completion

---

## Protocol Summary

### USB-CDC/UART Flow
```
1. Send "FIRMWARE_UPGRADE\n"
2. Device responds "OK Ready for YMODEM transfer\n"
3. YMODEM Block 0: filename + size
4. YMODEM Blocks 1-N: data (1024 bytes each)
5. Device validates and reboots
```

### BLE Flow
```
1. Write "FIRMWARE_UPGRADE <size>" to Control characteristic
2. Wait for Status notification (0x00 = OK)
3. Write firmware data in MTU-sized chunks to Data characteristic
4. Receive progress notifications (0-100%)
5. Device reboots automatically
```

### WiFi Flow
```
1. POST /api/firmware/upload with Content-Length header
2. Stream binary data
3. Device validates and reboots
```

---

## Timeline

| Phase | Component | Effort |
|-------|-----------|--------|
| 1 | OTA Engine Core | 4-6h |
| 2 | WiFi Refactor | 3-4h |
| 3 | Cleanup Old Code | 1h |
| 4 | USB-CDC | 4-5h |
| 5 | BLE | 4-5h |
| **Total** | **16-21h** |

---

## Migration Strategy

**Recommended: Big Bang Approach**

1. Implement Phase 1 (OTA Engine)
2. Implement Phase 2 (WiFi refactor)
3. Test thoroughly
4. Deploy
5. Add USB/BLE later (optional)

**Minimum Viable Product:** Phase 1 + 2 = ~8 hours for working WiFi OTA

---

## Success Criteria

✅ WiFi OTA completes 2MB firmware in <30s without crashes  
✅ Progress bar updates smoothly  
✅ No guru meditation errors  
✅ Power loss during OTA doesn't brick device  
✅ Memory usage stays reasonable  
✅ Code is simpler than current implementation
