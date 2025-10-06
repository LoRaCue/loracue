#include "config_wifi_server.h"
#include "cJSON.h"
#include "device_config.h"
#include "device_registry.h"
#include "esp_app_format.h"
#include "esp_crc.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "lora_driver.h"
#include "version.h"
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "CONFIG_WIFI_SERVER";

static httpd_handle_t server = NULL;
static esp_netif_t *ap_netif = NULL;
static bool server_running   = false;

// OTA update state
static esp_ota_handle_t ota_handle          = 0;
static const esp_partition_t *ota_partition = NULL;
static size_t ota_received_bytes            = 0;
static size_t ota_total_size                = 0;

// Generate WiFi credentials from MAC address
static void generate_wifi_credentials(char *ssid, size_t ssid_len, char *password, size_t pass_len)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // Generate SSID: LoRaCue-XXXX
    snprintf(ssid, ssid_len, "LoRaCue-%02X%02X", mac[4], mac[5]);

    // Generate password from MAC CRC32
    uint32_t crc        = esp_crc32_le(0, mac, 6);
    const char *charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < 8; i++) {
        password[i] = charset[crc % 62];
        crc /= 62;
    }
    password[8] = '\0';
}

// Serve static files from SPIFFS
static esp_err_t serve_static_file(httpd_req_t *req, const char *filepath)
{
    FILE *file = fopen(filepath, "r");
    if (!file) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // Set content type based on file extension
    const char *ext = strrchr(filepath, '.');
    if (ext) {
        if (strcmp(ext, ".html") == 0)
            httpd_resp_set_type(req, "text/html");
        else if (strcmp(ext, ".css") == 0)
            httpd_resp_set_type(req, "text/css");
        else if (strcmp(ext, ".js") == 0)
            httpd_resp_set_type(req, "application/javascript");
        else if (strcmp(ext, ".json") == 0)
            httpd_resp_set_type(req, "application/json");
    }

    char buffer[1024];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }
    httpd_resp_send_chunk(req, NULL, 0);
    fclose(file);
    return ESP_OK;
}

// HTTP handlers
static esp_err_t root_handler(httpd_req_t *req)
{
    return serve_static_file(req, "/spiffs/index.html");
}

static esp_err_t static_handler(httpd_req_t *req)
{
    char filepath[512];
    if (strlen(req->uri) > 500) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(filepath, sizeof(filepath), "/spiffs%s", req->uri);
#pragma GCC diagnostic pop
    return serve_static_file(req, filepath);
}

static esp_err_t device_settings_get_handler(httpd_req_t *req)
{
    device_config_t config;
    if (device_config_get(&config) != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "name", config.device_name);
    cJSON_AddStringToObject(json, "mode", device_mode_to_string(config.device_mode));
    cJSON_AddNumberToObject(json, "sleepTimeout", config.sleep_timeout_ms);
    cJSON_AddBoolToObject(json, "autoSleep", config.auto_sleep_enabled);
    cJSON_AddNumberToObject(json, "brightness", config.display_brightness);
    cJSON_AddBoolToObject(json, "bluetooth", config.bluetooth_enabled);

    char *json_string = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));

    free(json_string);
    cJSON_Delete(json);
    return ESP_OK;
}

static esp_err_t device_settings_post_handler(httpd_req_t *req)
{
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    device_config_t config;
    device_config_get(&config);

    cJSON *name = cJSON_GetObjectItem(json, "name");
    if (name && cJSON_IsString(name)) {
        strncpy(config.device_name, name->valuestring, sizeof(config.device_name) - 1);
    }

    cJSON *mode = cJSON_GetObjectItem(json, "mode");
    if (mode && cJSON_IsString(mode)) {
        if (strcmp(mode->valuestring, "presenter") == 0) {
            config.device_mode = DEVICE_MODE_PRESENTER;
        } else if (strcmp(mode->valuestring, "pc") == 0) {
            config.device_mode = DEVICE_MODE_PC;
        }
    }

    cJSON *sleepTimeout = cJSON_GetObjectItem(json, "sleepTimeout");
    if (sleepTimeout && cJSON_IsNumber(sleepTimeout)) {
        config.sleep_timeout_ms = sleepTimeout->valueint;
    }

    cJSON *autoSleep = cJSON_GetObjectItem(json, "autoSleep");
    if (autoSleep && cJSON_IsBool(autoSleep)) {
        config.auto_sleep_enabled = cJSON_IsTrue(autoSleep);
    }

    cJSON *brightness = cJSON_GetObjectItem(json, "brightness");
    if (brightness && cJSON_IsNumber(brightness)) {
        config.display_brightness = brightness->valueint;
    }

    cJSON *bluetooth = cJSON_GetObjectItem(json, "bluetooth");
    if (bluetooth && cJSON_IsBool(bluetooth)) {
        config.bluetooth_enabled = cJSON_IsTrue(bluetooth);
    }

    esp_err_t err = device_config_set(&config);
    cJSON_Delete(json);

    if (err != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\"}", 16);
    return ESP_OK;
}

static esp_err_t lora_settings_get_handler(httpd_req_t *req)
{
    lora_config_t config;
    if (lora_get_config(&config) != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "frequency", config.frequency);
    cJSON_AddNumberToObject(json, "spreadingFactor", config.spreading_factor);
    cJSON_AddNumberToObject(json, "bandwidth", config.bandwidth);
    cJSON_AddNumberToObject(json, "codingRate", config.coding_rate);
    cJSON_AddNumberToObject(json, "txPower", config.tx_power);

    char *json_string = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));

    free(json_string);
    cJSON_Delete(json);
    return ESP_OK;
}

static esp_err_t lora_settings_post_handler(httpd_req_t *req)
{
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    lora_config_t config;
    lora_get_config(&config);

    cJSON *freq = cJSON_GetObjectItem(json, "frequency");
    if (freq && cJSON_IsNumber(freq))
        config.frequency = freq->valueint;

    cJSON *sf = cJSON_GetObjectItem(json, "spreadingFactor");
    if (sf && cJSON_IsNumber(sf))
        config.spreading_factor = sf->valueint;

    cJSON *bw = cJSON_GetObjectItem(json, "bandwidth");
    if (bw && cJSON_IsNumber(bw))
        config.bandwidth = bw->valueint;

    cJSON *cr = cJSON_GetObjectItem(json, "codingRate");
    if (cr && cJSON_IsNumber(cr))
        config.coding_rate = cr->valueint;

    cJSON *power = cJSON_GetObjectItem(json, "txPower");
    if (power && cJSON_IsNumber(power))
        config.tx_power = power->valueint;

    esp_err_t err = lora_set_config(&config);
    cJSON_Delete(json);

    if (err != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\"}", 16);
    return ESP_OK;
}

static esp_err_t firmware_start_handler(httpd_req_t *req)
{
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *size = cJSON_GetObjectItem(json, "size");
    if (!cJSON_IsNumber(size)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing size parameter");
        return ESP_FAIL;
    }

    // Clean up any existing OTA session
    if (ota_handle) {
        esp_ota_abort(ota_handle);
        ota_handle = 0;
    }

    ota_total_size     = size->valueint;
    ota_received_bytes = 0;

    // Get next OTA partition
    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Begin OTA update
    esp_err_t err = esp_ota_begin(ota_partition, ota_total_size, &ota_handle);
    if (err != ESP_OK) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Ready for firmware data\"}", 50);

    return ESP_OK;
}

static esp_err_t firmware_data_handler(httpd_req_t *req)
{
    if (!ota_handle) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No active OTA session");
        return ESP_FAIL;
    }

    char content[2048];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';

    // Parse hex data from JSON
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *data_item = cJSON_GetObjectItem(json, "data");
    if (!cJSON_IsString(data_item)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing data parameter");
        return ESP_FAIL;
    }

    const char *hex_data = data_item->valuestring;
    size_t hex_len       = strlen(hex_data);

    if (hex_len % 2 != 0) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid hex data length");
        return ESP_FAIL;
    }

    size_t data_len      = hex_len / 2;
    uint8_t *binary_data = malloc(data_len);
    if (!binary_data) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Convert hex to binary
    for (size_t i = 0; i < data_len; i++) {
        if (sscanf(hex_data + i * 2, "%02hhx", &binary_data[i]) != 1) {
            free(binary_data);
            cJSON_Delete(json);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid hex data");
            return ESP_FAIL;
        }
    }

    esp_err_t err = esp_ota_write(ota_handle, binary_data, data_len);
    free(binary_data);
    cJSON_Delete(json);

    if (err != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ota_received_bytes += data_len;

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Data written\"}", 40);

    return ESP_OK;
}

static esp_err_t firmware_verify_handler(httpd_req_t *req)
{
    if (!ota_handle) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No active OTA session");
        return ESP_FAIL;
    }

    esp_err_t err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ota_handle = 0;
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Firmware verified\"}", 47);

    return ESP_OK;
}

static esp_err_t firmware_commit_handler(httpd_req_t *req)
{
    if (!ota_partition) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No OTA partition to commit");
        return ESP_FAIL;
    }

    esp_err_t err = esp_ota_set_boot_partition(ota_partition);
    if (err != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Firmware committed - rebooting\"}", 60);

    // Clean up
    ota_handle         = 0;
    ota_partition      = NULL;
    ota_received_bytes = 0;
    ota_total_size     = 0;

    // Restart after a short delay
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
}

static esp_err_t devices_list_handler(httpd_req_t *req)
{
    cJSON *devices_array = cJSON_CreateArray();

    for (uint16_t i = 0; i < 0xFFFF; i++) {
        paired_device_t device;
        if (device_registry_get(i, &device) == ESP_OK) {
            cJSON *device_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(device_obj, "id", device.device_id);
            cJSON_AddStringToObject(device_obj, "name", device.device_name);

            char mac_str[18];
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", device.mac_address[0],
                     device.mac_address[1], device.mac_address[2], device.mac_address[3], device.mac_address[4],
                     device.mac_address[5]);
            cJSON_AddStringToObject(device_obj, "mac", mac_str);
            cJSON_AddBoolToObject(device_obj, "active", device.is_active);

            cJSON_AddItemToArray(devices_array, device_obj);
        }
    }

    char *json_string = cJSON_Print(devices_array);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));
    free(json_string);
    cJSON_Delete(devices_array);
    return ESP_OK;
}

static esp_err_t devices_add_handler(httpd_req_t *req)
{
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *name       = cJSON_GetObjectItem(json, "name");
    const cJSON *mac  = cJSON_GetObjectItem(json, "mac");
    const cJSON *key  = cJSON_GetObjectItem(json, "key");

    if (!name || !mac || !key) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing parameters");
        return ESP_FAIL;
    }

    uint8_t mac_bytes[6];
    if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac_bytes[0], &mac_bytes[1],
               &mac_bytes[2], &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid MAC");
        return ESP_FAIL;
    }

    uint8_t aes_key[32];
    const char *key_str = key->valuestring;
    if (strlen(key_str) != 64) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid key length");
        return ESP_FAIL;
    }

    for (int i = 0; i < 32; i++) {
        if (sscanf(key_str + i * 2, "%02hhx", &aes_key[i]) != 1) {
            cJSON_Delete(json);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid key format");
            return ESP_FAIL;
        }
    }

    uint16_t device_id = (mac_bytes[4] << 8) | mac_bytes[5];
    esp_err_t err      = device_registry_add(device_id, name->valuestring, mac_bytes, aes_key);
    cJSON_Delete(json);

    if (err != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\"}", 16);
    return ESP_OK;
}

static esp_err_t devices_delete_handler(httpd_req_t *req)
{
    const char *id_str = strrchr(req->uri, '/');
    if (!id_str) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing device ID");
        return ESP_FAIL;
    }

    uint16_t device_id = (uint16_t)strtol(id_str + 1, NULL, 10);
    esp_err_t err      = device_registry_remove(device_id);

    if (err != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\"}", 16);
    return ESP_OK;

    return ESP_OK;
}

static esp_err_t system_info_handler(httpd_req_t *req)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "version", LORACUE_VERSION_FULL);
    cJSON_AddStringToObject(json, "commit", LORACUE_BUILD_COMMIT_SHORT);
    cJSON_AddStringToObject(json, "branch", LORACUE_BUILD_BRANCH);
    cJSON_AddStringToObject(json, "buildDate", LORACUE_BUILD_DATE);
    cJSON_AddNumberToObject(json, "uptime", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(json, "freeHeap", esp_get_free_heap_size());

    const esp_partition_t *running = esp_ota_get_running_partition();
    cJSON_AddStringToObject(json, "partition", running->label);

    char *json_string = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));
    free(json_string);
    cJSON_Delete(json);
    return ESP_OK;
}

static esp_err_t factory_reset_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Factory reset initiated\"}", 52);

    vTaskDelay(pdMS_TO_TICKS(1000));
    device_config_factory_reset();
    return ESP_OK;
}

esp_err_t config_wifi_server_start(void)
{
    if (server_running) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting WiFi AP and web server");

    // Initialize SPIFFS
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = "/spiffs", .partition_label = NULL, .max_files = 20, .format_if_mount_failed = true};
    esp_vfs_spiffs_register(&spiffs_conf);

    // Initialize WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Generate credentials
    char ssid[32];
    char password[16];
    generate_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password));

    // Configure AP
    wifi_config_t wifi_config = {
        .ap = {.channel = 1, .max_connection = 4, .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };

    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = strlen(ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Start HTTP server
    httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
    config.server_port      = 80;
    config.max_uri_handlers = 16;

    if (httpd_start(&server, &config) == ESP_OK) {
        // Static file handlers
        httpd_uri_t root_uri = {.uri = "/", .method = HTTP_GET, .handler = root_handler};
        httpd_register_uri_handler(server, &root_uri);

        httpd_uri_t static_uri = {.uri = "/*", .method = HTTP_GET, .handler = static_handler};
        httpd_register_uri_handler(server, &static_uri);

        // API handlers
        httpd_uri_t device_get_uri = {
            .uri = "/api/device/settings", .method = HTTP_GET, .handler = device_settings_get_handler};
        httpd_register_uri_handler(server, &device_get_uri);

        httpd_uri_t device_post_uri = {
            .uri = "/api/device/settings", .method = HTTP_POST, .handler = device_settings_post_handler};
        httpd_register_uri_handler(server, &device_post_uri);

        httpd_uri_t lora_get_uri = {
            .uri = "/api/lora/settings", .method = HTTP_GET, .handler = lora_settings_get_handler};
        httpd_register_uri_handler(server, &lora_get_uri);

        httpd_uri_t lora_post_uri = {
            .uri = "/api/lora/settings", .method = HTTP_POST, .handler = lora_settings_post_handler};
        httpd_register_uri_handler(server, &lora_post_uri);

        // Firmware update API endpoints
        httpd_uri_t fw_start_uri = {
            .uri = "/api/firmware/start", .method = HTTP_POST, .handler = firmware_start_handler};
        httpd_register_uri_handler(server, &fw_start_uri);

        httpd_uri_t fw_data_uri = {.uri = "/api/firmware/data", .method = HTTP_POST, .handler = firmware_data_handler};
        httpd_register_uri_handler(server, &fw_data_uri);

        httpd_uri_t fw_verify_uri = {
            .uri = "/api/firmware/verify", .method = HTTP_POST, .handler = firmware_verify_handler};
        httpd_register_uri_handler(server, &fw_verify_uri);

        httpd_uri_t fw_commit_uri = {
            .uri = "/api/firmware/commit", .method = HTTP_POST, .handler = firmware_commit_handler};
        httpd_register_uri_handler(server, &fw_commit_uri);

        // Device registry API endpoints
        httpd_uri_t devices_list_uri = {.uri = "/api/devices", .method = HTTP_GET, .handler = devices_list_handler};
        httpd_register_uri_handler(server, &devices_list_uri);

        httpd_uri_t devices_add_uri = {.uri = "/api/devices", .method = HTTP_POST, .handler = devices_add_handler};
        httpd_register_uri_handler(server, &devices_add_uri);

        httpd_uri_t devices_delete_uri = {
            .uri = "/api/devices/*", .method = HTTP_DELETE, .handler = devices_delete_handler};
        httpd_register_uri_handler(server, &devices_delete_uri);

        // System API endpoints
        httpd_uri_t system_info_uri = {.uri = "/api/system/info", .method = HTTP_GET, .handler = system_info_handler};
        httpd_register_uri_handler(server, &system_info_uri);

        httpd_uri_t factory_reset_uri = {
            .uri = "/api/system/factory-reset", .method = HTTP_POST, .handler = factory_reset_handler};
        httpd_register_uri_handler(server, &factory_reset_uri);

        server_running = true;
        ESP_LOGI(TAG, "WiFi AP started: %s / %s", ssid, password);
        ESP_LOGI(TAG, "Web server started on http://192.168.4.1");
    }

    return ESP_OK;
}

esp_err_t config_wifi_server_stop(void)
{
    if (!server_running) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping WiFi AP and web server");

    // Stop HTTP server
    if (server) {
        httpd_stop(server);
        server = NULL;
    }

    // Stop WiFi AP
    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi stop failed: %s", esp_err_to_name(ret));
    }

    ret = esp_wifi_deinit();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi deinit failed: %s", esp_err_to_name(ret));
    }

    // Destroy netif
    if (ap_netif) {
        esp_netif_destroy(ap_netif);
        ap_netif = NULL;
    }

    // Unmount SPIFFS
    esp_vfs_spiffs_unregister(NULL);

#ifndef SIMULATOR_BUILD
    // Only disable WiFi on real hardware for power saving
    esp_event_loop_delete_default();
    esp_netif_deinit();
#endif

    server_running = false;
    ESP_LOGI(TAG, "WiFi AP and web server stopped");

    return ESP_OK;
}

bool config_wifi_server_is_running(void)
{
    return server_running;
}
