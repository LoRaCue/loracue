# OTA System Refactor - Enterprise Grade

## Executive Summary

**Problem:** Current OTA causes guru meditation due to chunked HTTP requests with 20ms delays between writes.

**Solution:** Direct streaming writes with transport-agnostic OTA engine using XMODEM-1K for serial transports.

**Result:** <30s WiFi OTA, reliable USB/BLE updates, automatic rollback protection.

---

## Problem Analysis

### Root Cause: Flash Controller Expectations Violated

1. **Per-chunk HTTP requests** - Breaks sequential write assumption
2. **20ms delays** - Flash controller expects continuous stream  
3. **malloc() per chunk** - Heap fragmentation during critical operation
4. **Base64 overhead** - 25% size increase + CPU load
5. **JSON parsing** - Latency between writes

### ESP-IDF Best Practice (from official example)

✅ Single continuous stream  
✅ Immediate writes (no delays)  
✅ Stack buffers (no malloc)  
✅ Binary transfer  
✅ Single task context

---

## Solution Architecture

### Core Principles

1. **Direct writes** - Transport → `esp_ota_write()` immediately
2. **No delays** - Continuous write loop
3. **Binary transfer** - No encoding overhead
4. **Size validation** - Upfront verification
5. **Mutex protection** - Thread-safe state
6. **Automatic rollback** - Boot failure protection

### System Diagram

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
│         │  (Thread-Safe)      │                     │
│         └──────────┬──────────┘                     │
│                    │                                  │
│         ┌──────────▼──────────┐                     │
│         │  esp_ota_write()    │                     │
│         │  (Direct, No Delay) │                     │
│         └─────────────────────┘                     │
└─────────────────────────────────────────────────────┘
```

---

## Implementation

### Phase 1: OTA Engine Core (6-8 hours)

#### Component Structure
```
components/ota_engine/
├── include/ota_engine.h
├── ota_engine.c
└── CMakeLists.txt
```

#### Public API
```c
// ota_engine.h
typedef enum {
    OTA_STATE_IDLE,
    OTA_STATE_ACTIVE,
    OTA_STATE_FINALIZING
} ota_state_t;

esp_err_t ota_engine_init(void);
esp_err_t ota_engine_start(size_t firmware_size);
esp_err_t ota_engine_write(const uint8_t *data, size_t len);
esp_err_t ota_engine_finish(void);
esp_err_t ota_engine_abort(void);
size_t ota_engine_get_progress(void);
ota_state_t ota_engine_get_state(void);
```

#### Implementation (Enterprise-Grade)
```c
// ota_engine.c
static SemaphoreHandle_t ota_mutex = NULL;
static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *ota_partition = NULL;
static size_t ota_received_bytes = 0;
static size_t ota_total_size = 0;
static ota_state_t ota_state = OTA_STATE_IDLE;
static TickType_t ota_last_write_time = 0;

#define OTA_TIMEOUT_MS 30000  // 30 second timeout

esp_err_t ota_engine_init(void)
{
    if (!ota_mutex) {
        ota_mutex = xSemaphoreCreateMutex();
        if (!ota_mutex) return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t ota_engine_start(size_t firmware_size)
{
    if (!ota_mutex) return ESP_ERR_INVALID_STATE;
    
    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Check if already active
    if (ota_state != OTA_STATE_IDLE) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    
    // Validate size
    if (firmware_size == 0 || firmware_size > 4*1024*1024) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }
    
    // Get next partition
    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        xSemaphoreGive(ota_mutex);
        return ESP_FAIL;
    }
    
    // Validate partition size
    if (firmware_size > ota_partition->size) {
        ESP_LOGE(TAG, "Firmware too large: %zu > %zu", 
                 firmware_size, ota_partition->size);
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }
    
    // Begin OTA
    esp_err_t ret = esp_ota_begin(ota_partition, firmware_size, &ota_handle);
    if (ret != ESP_OK) {
        xSemaphoreGive(ota_mutex);
        return ret;
    }
    
    ota_total_size = firmware_size;
    ota_received_bytes = 0;
    ota_state = OTA_STATE_ACTIVE;
    ota_last_write_time = xTaskGetTickCount();
    
    ESP_LOGI(TAG, "OTA started: %zu bytes to partition %s", 
             firmware_size, ota_partition->label);
    
    xSemaphoreGive(ota_mutex);
    return ESP_OK;
}

esp_err_t ota_engine_write(const uint8_t *data, size_t len)
{
    if (!ota_mutex) return ESP_ERR_INVALID_STATE;
    
    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Check state
    if (ota_state != OTA_STATE_ACTIVE || !ota_handle) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    
    // Check timeout
    TickType_t now = xTaskGetTickCount();
    if ((now - ota_last_write_time) > pdMS_TO_TICKS(OTA_TIMEOUT_MS)) {
        ESP_LOGE(TAG, "OTA timeout");
        xSemaphoreGive(ota_mutex);
        ota_engine_abort();
        return ESP_ERR_TIMEOUT;
    }
    
    // Validate write won't exceed size
    if (ota_received_bytes + len > ota_total_size) {
        ESP_LOGE(TAG, "Write exceeds firmware size");
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }
    
    // Direct write - NO delays, NO buffering
    esp_err_t ret = esp_ota_write(ota_handle, data, len);
    if (ret == ESP_OK) {
        ota_received_bytes += len;
        ota_last_write_time = now;
    }
    
    xSemaphoreGive(ota_mutex);
    return ret;
}

esp_err_t ota_engine_finish(void)
{
    if (!ota_mutex) return ESP_ERR_INVALID_STATE;
    
    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    if (ota_state != OTA_STATE_ACTIVE || !ota_handle) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    
    // Verify size
    if (ota_received_bytes != ota_total_size) {
        ESP_LOGW(TAG, "Size mismatch: received %zu, expected %zu", 
                 ota_received_bytes, ota_total_size);
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }
    
    ota_state = OTA_STATE_FINALIZING;
    
    // End OTA (validates image)
    esp_err_t ret = esp_ota_end(ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA validation failed: %s", esp_err_to_name(ret));
        ota_handle = 0;
        ota_state = OTA_STATE_IDLE;
        xSemaphoreGive(ota_mutex);
        return ret;
    }
    
    // Set boot partition (enables rollback on next boot)
    ret = esp_ota_set_boot_partition(ota_partition);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(ret));
        ota_handle = 0;
        ota_state = OTA_STATE_IDLE;
        xSemaphoreGive(ota_mutex);
        return ret;
    }
    
    ESP_LOGI(TAG, "OTA complete: %zu bytes written", ota_received_bytes);
    
    ota_handle = 0;
    ota_state = OTA_STATE_IDLE;
    
    xSemaphoreGive(ota_mutex);
    return ESP_OK;
}

esp_err_t ota_engine_abort(void)
{
    if (!ota_mutex) return ESP_ERR_INVALID_STATE;
    
    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    if (ota_handle) {
        esp_ota_abort(ota_handle);
        ota_handle = 0;
        ESP_LOGW(TAG, "OTA aborted");
    }
    
    ota_received_bytes = 0;
    ota_total_size = 0;
    ota_state = OTA_STATE_IDLE;
    
    xSemaphoreGive(ota_mutex);
    return ESP_OK;
}

size_t ota_engine_get_progress(void)
{
    if (ota_total_size > 0) {
        return (ota_received_bytes * 100) / ota_total_size;
    }
    return 0;
}

ota_state_t ota_engine_get_state(void)
{
    return ota_state;
}
```

#### Rollback Protection (Add to main.c)
```c
// main.c - app_main()
void app_main(void)
{
    // Check if we just booted from OTA
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // New firmware booted successfully
            ESP_LOGI(TAG, "New firmware running, marking as valid");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
    
    // Rest of initialization...
}
```

---

### Phase 2: WiFi Transport (3-4 hours)

**Remove:** All chunked endpoints  
**Add:** Single streaming endpoint

```c
static esp_err_t api_firmware_upload_handler(httpd_req_t *req)
{
    size_t content_length = req->content_len;
    
    esp_err_t ret = ota_engine_start(content_length);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA start failed");
        return ESP_FAIL;
    }
    
    uint8_t buffer[4096];
    size_t received = 0;
    
    while (received < content_length) {
        int len = httpd_req_recv(req, (char *)buffer, sizeof(buffer));
        if (len <= 0) {
            ota_engine_abort();
            return ESP_FAIL;
        }
        
        ret = ota_engine_write(buffer, len);
        if (ret != ESP_OK) {
            ota_engine_abort();
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
            return ESP_FAIL;
        }
        
        received += len;
    }
    
    ret = ota_engine_finish();
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validation failed");
        return ESP_FAIL;
    }
    
    httpd_resp_send(req, "OK", 2);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    
    return ESP_OK;
}
```

**Frontend:** XMLHttpRequest with native progress tracking

---

### Phase 3: USB-CDC/UART with XMODEM (4-5 hours)

**Command:** `FIRMWARE_UPGRADE <size>`  
**Protocol:** XMODEM-1K (1024-byte blocks, CRC16)

**Why XMODEM over YMODEM:**
- ✅ Simpler (no header block parsing)
- ✅ Size from command (explicit validation)
- ✅ Same pattern as BLE
- ✅ Proven reliability (CRC16 error detection)
- ✅ Less code to maintain

---

### Phase 4: BLE Transport (4-5 hours)

**Command:** `FIRMWARE_UPGRADE <size>` to Control characteristic  
**Data:** MTU-sized chunks to Data characteristic  
**Progress:** 0-100% notifications on Status characteristic

---

## Enterprise-Grade Features

### ✅ Thread Safety
- Mutex-protected state transitions
- Atomic operations
- Timeout detection

### ✅ Rollback Protection
- `ESP_OTA_IMG_PENDING_VERIFY` state
- Automatic rollback on boot failure
- Manual validation after successful boot

### ✅ Error Handling
- Size validation upfront
- Timeout detection (30s)
- Proper cleanup on failure
- Detailed error logging

### ✅ Security
- Size limits (max 4MB)
- Partition boundary checks
- Image validation (esp_ota_end)
- No buffer overflows

### ✅ Observability
- Progress tracking
- State machine visibility
- Detailed logging
- Error reporting

---

## Testing Strategy

### Unit Tests
- [ ] Mutex contention handling
- [ ] Timeout detection
- [ ] Size validation
- [ ] State transitions

### Integration Tests
- [ ] WiFi: 2MB in <30s
- [ ] USB: XMODEM with retries
- [ ] BLE: Small MTU chunks
- [ ] Concurrent access rejection

### Failure Tests
- [ ] Power loss at 50%
- [ ] Network disconnect
- [ ] Invalid firmware
- [ ] Timeout scenarios
- [ ] Rollback verification

---

## Success Criteria

✅ No guru meditation errors  
✅ <30s WiFi OTA for 2MB firmware  
✅ Automatic rollback on boot failure  
✅ Thread-safe concurrent access  
✅ Timeout protection  
✅ 100% error handling coverage  
✅ Production-ready logging

---

## Timeline

| Phase | Hours | Priority |
|-------|-------|----------|
| OTA Engine | 6-8h | P0 |
| WiFi | 3-4h | P0 |
| USB-CDC | 4-5h | P1 |
| BLE | 4-5h | P2 |
| Testing | 6-8h | P0 |
| **Total** | **23-30h** | |

**MVP:** Phase 1 + 2 = 10-12 hours

