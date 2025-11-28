#include "ota_engine.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/sha256.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include <string.h>

static const char *TAG = "ota_engine";

static SemaphoreHandle_t ota_mutex          = NULL;
static esp_ota_handle_t ota_handle          = 0;
static const esp_partition_t *ota_partition = NULL;
static size_t ota_received_bytes            = 0;
static size_t ota_total_size                = 0;
static ota_state_t ota_state                = OTA_STATE_IDLE;
static TickType_t ota_last_write_time       = 0;
static mbedtls_sha256_context sha256_ctx;
static char expected_sha256[65]             = {0}; // 64 hex chars + null terminator
static bool sha256_verification_enabled     = false;
static uint8_t public_key[32]               = {0}; // Ed25519 public key (32 bytes)
static bool public_key_set                  = false;
static char expected_signature[129]         = {0}; // 128 hex chars + null terminator
static bool signature_verification_enabled  = false;

#define OTA_TIMEOUT_MS 30000

// Embedded Ed25519 public key (32 bytes) from keys/firmware_public_ed25519.bin
extern const uint8_t firmware_public_ed25519_bin_start[] asm("_binary_firmware_public_ed25519_bin_start");
extern const uint8_t firmware_public_ed25519_bin_end[] asm("_binary_firmware_public_ed25519_bin_end");

esp_err_t ota_engine_init(void)
{
    if (!ota_mutex) {
        ota_mutex = xSemaphoreCreateMutex();
        if (!ota_mutex)
            return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t ota_engine_set_expected_sha256(const char *sha256_hex)
{
    if (!sha256_hex) {
        sha256_verification_enabled = false;
        expected_sha256[0] = '\0';
        return ESP_OK;
    }

    // Validate format: 64 hex characters
    size_t len = strlen(sha256_hex);
    if (len != 64) {
        ESP_LOGE(TAG, "Invalid SHA256 length: %zu (expected 64)", len);
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < 64; i++) {
        char c = sha256_hex[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            ESP_LOGE(TAG, "Invalid SHA256 character at position %zu: '%c'", i, c);
            return ESP_ERR_INVALID_ARG;
        }
    }

    strncpy(expected_sha256, sha256_hex, sizeof(expected_sha256) - 1);
    expected_sha256[64] = '\0';
    sha256_verification_enabled = true;
    
    ESP_LOGI(TAG, "Expected SHA256 set: %.16s...", expected_sha256);
    return ESP_OK;
}

esp_err_t ota_engine_set_public_key(const uint8_t *key, size_t key_len)
{
    if (!key || key_len != 32) {
        ESP_LOGE(TAG, "Invalid public key (expected 32 bytes)");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(public_key, key, 32);
    public_key_set = true;
    ESP_LOGI(TAG, "Ed25519 public key set");
    return ESP_OK;
}

esp_err_t ota_engine_verify_signature(const char *signature_hex)
{
    if (!signature_hex) {
        signature_verification_enabled = false;
        expected_signature[0] = '\0';
        return ESP_OK;
    }

    // Validate format: 128 hex characters (64 bytes)
    size_t len = strlen(signature_hex);
    if (len != 128) {
        ESP_LOGE(TAG, "Invalid signature length: %zu (expected 128)", len);
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < 128; i++) {
        char c = signature_hex[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            ESP_LOGE(TAG, "Invalid signature character at position %zu: '%c'", i, c);
            return ESP_ERR_INVALID_ARG;
        }
    }

    strncpy(expected_signature, signature_hex, sizeof(expected_signature) - 1);
    expected_signature[128] = '\0';
    signature_verification_enabled = true;

    // Use embedded public key if not set
    if (!public_key_set) {
        size_t key_size = firmware_public_ed25519_bin_end - firmware_public_ed25519_bin_start;
        if (key_size != 32) {
            ESP_LOGE(TAG, "Invalid embedded public key size: %zu (expected 32)", key_size);
            return ESP_ERR_INVALID_SIZE;
        }
        memcpy(public_key, firmware_public_ed25519_bin_start, 32);
        ESP_LOGI(TAG, "Using embedded Ed25519 public key");
    }

    ESP_LOGI(TAG, "Expected signature set: %.16s...", expected_signature);
    return ESP_OK;
}

esp_err_t ota_engine_start(size_t firmware_size)
{
    if (!ota_mutex)
        return ESP_ERR_INVALID_STATE;

    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (ota_state != OTA_STATE_IDLE) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    if (firmware_size == 0 || firmware_size > 4 * 1024 * 1024) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }

    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        xSemaphoreGive(ota_mutex);
        return ESP_FAIL;
    }

    if (firmware_size > ota_partition->size) {
        ESP_LOGE(TAG, "Firmware too large: %zu > %zu", firmware_size, ota_partition->size);
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t ret = esp_ota_begin(ota_partition, firmware_size, &ota_handle);
    if (ret != ESP_OK) {
        xSemaphoreGive(ota_mutex);
        return ret;
    }

    ota_total_size      = firmware_size;
    ota_received_bytes  = 0;
    ota_state           = OTA_STATE_ACTIVE;
    ota_last_write_time = xTaskGetTickCount();

    // Initialize SHA256 context if verification is enabled
    if (sha256_verification_enabled) {
        mbedtls_sha256_init(&sha256_ctx);
        mbedtls_sha256_starts(&sha256_ctx, 0); // 0 = SHA256 (not SHA224)
        ESP_LOGI(TAG, "SHA256 verification enabled");
    }

    ESP_LOGI(TAG, "OTA started: %zu bytes to partition %s", firmware_size, ota_partition->label);

    xSemaphoreGive(ota_mutex);
    return ESP_OK;
}

esp_err_t ota_engine_write(const uint8_t *data, size_t len)
{
    if (!ota_mutex)
        return ESP_ERR_INVALID_STATE;

    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (ota_state != OTA_STATE_ACTIVE || !ota_handle) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    TickType_t now = xTaskGetTickCount();
    if ((now - ota_last_write_time) > pdMS_TO_TICKS(OTA_TIMEOUT_MS)) {
        ESP_LOGE(TAG, "OTA timeout");
        xSemaphoreGive(ota_mutex);
        ota_engine_abort();
        return ESP_ERR_TIMEOUT;
    }

    if (ota_received_bytes + len > ota_total_size) {
        ESP_LOGE(TAG, "Write exceeds firmware size");
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t ret = esp_ota_write(ota_handle, data, len);
    if (ret == ESP_OK) {
        ota_received_bytes += len;
        ota_last_write_time = now;
        
        // Update SHA256 hash if verification is enabled
        if (sha256_verification_enabled) {
            mbedtls_sha256_update(&sha256_ctx, data, len);
        }
    }

    xSemaphoreGive(ota_mutex);
    return ret;
}

esp_err_t ota_engine_finish(void)
{
    if (!ota_mutex)
        return ESP_ERR_INVALID_STATE;

    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (ota_state != OTA_STATE_ACTIVE || !ota_handle) {
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    if (ota_received_bytes != ota_total_size) {
        ESP_LOGW(TAG, "Size mismatch: received %zu, expected %zu", ota_received_bytes, ota_total_size);
        xSemaphoreGive(ota_mutex);
        return ESP_ERR_INVALID_SIZE;
    }

    ota_state = OTA_STATE_FINALIZING;

    // Verify SHA256 if enabled
    if (sha256_verification_enabled) {
        uint8_t calculated_hash[32];
        mbedtls_sha256_finish(&sha256_ctx, calculated_hash);
        mbedtls_sha256_free(&sha256_ctx);

        // Convert calculated hash to hex string
        char calculated_hex[65];
        for (int i = 0; i < 32; i++) {
            sprintf(&calculated_hex[i * 2], "%02x", calculated_hash[i]);
        }
        calculated_hex[64] = '\0';

        // Compare with expected hash (case-insensitive)
        if (strcasecmp(calculated_hex, expected_sha256) != 0) {
            ESP_LOGE(TAG, "SHA256 mismatch!");
            ESP_LOGE(TAG, "Expected:   %s", expected_sha256);
            ESP_LOGE(TAG, "Calculated: %s", calculated_hex);
            ota_handle = 0;
            ota_state = OTA_STATE_IDLE;
            sha256_verification_enabled = false;
            xSemaphoreGive(ota_mutex);
            return ESP_ERR_INVALID_CRC;
        }

        ESP_LOGI(TAG, "SHA256 verification passed: %s", calculated_hex);
        sha256_verification_enabled = false;

        // Verify Ed25519 signature if enabled
        if (signature_verification_enabled) {
            // Convert signature from hex to binary
            uint8_t signature_bin[64];
            for (int i = 0; i < 64; i++) {
                char hex[3] = {expected_signature[i * 2], expected_signature[i * 2 + 1], '\0'};
                signature_bin[i] = (uint8_t)strtol(hex, NULL, 16);
            }

            // Convert SHA256 hash from hex to binary for signing
            uint8_t hash_bin[32];
            for (int i = 0; i < 32; i++) {
                char hex[3] = {calculated_hex[i * 2], calculated_hex[i * 2 + 1], '\0'};
                hash_bin[i] = (uint8_t)strtol(hex, NULL, 16);
            }

            // Verify signature using mbedtls Ed25519
            mbedtls_pk_context pk_ctx;
            mbedtls_pk_init(&pk_ctx);

            // Setup Ed25519 key (mbedtls uses ECDSA with Ed25519 curve)
            int ret_sig = mbedtls_pk_setup(&pk_ctx, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
            if (ret_sig == 0) {
                // Note: mbedtls doesn't have native Ed25519, we'll use a simplified check
                // In production, use a proper Ed25519 library like libsodium or TweetNaCl
                ESP_LOGW(TAG, "Ed25519 signature verification not fully implemented");
                ESP_LOGW(TAG, "Using placeholder verification (INSECURE!)");
                
                // Placeholder: Just check signature is not all zeros
                bool all_zeros = true;
                for (int i = 0; i < 64; i++) {
                    if (signature_bin[i] != 0) {
                        all_zeros = false;
                        break;
                    }
                }

                if (all_zeros) {
                    ESP_LOGE(TAG, "Invalid signature (all zeros)");
                    mbedtls_pk_free(&pk_ctx);
                    ota_handle = 0;
                    ota_state = OTA_STATE_IDLE;
                    signature_verification_enabled = false;
                    xSemaphoreGive(ota_mutex);
                    return ESP_ERR_INVALID_RESPONSE;
                }

                ESP_LOGI(TAG, "Signature format validated (full Ed25519 verification TODO)");
            }

            mbedtls_pk_free(&pk_ctx);
            signature_verification_enabled = false;
        }
    }

    esp_err_t ret = esp_ota_end(ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA validation failed: %s", esp_err_to_name(ret));
        ota_handle = 0;
        ota_state  = OTA_STATE_IDLE;
        xSemaphoreGive(ota_mutex);
        return ret;
    }

    ESP_LOGI(TAG, "OTA complete: %zu bytes written to %s", ota_received_bytes, ota_partition->label);
    ESP_LOGI(TAG, "Image validated. Call esp_ota_set_boot_partition() and reboot to activate.");

    ota_handle = 0;
    ota_state  = OTA_STATE_IDLE;

    xSemaphoreGive(ota_mutex);
    return ESP_OK;
}

esp_err_t ota_engine_abort(void)
{
    if (!ota_mutex)
        return ESP_ERR_INVALID_STATE;

    if (xSemaphoreTake(ota_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (ota_handle) {
        esp_ota_abort(ota_handle);
        ota_handle = 0;
        ESP_LOGW(TAG, "OTA aborted");
    }

    ota_received_bytes = 0;
    ota_total_size     = 0;
    ota_state          = OTA_STATE_IDLE;

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
