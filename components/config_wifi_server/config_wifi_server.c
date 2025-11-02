#include "config_wifi_server.h"
#include "cJSON.h"
#include "commands.h"
#include "device_registry.h"
#include "esp_app_format.h"
#include "esp_crc.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "general_config.h"
#include "lora_driver.h"
#include "ota_engine.h"
#include "power_mgmt_config.h"
#include "version.h"
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "CONFIG_WIFI_SERVER";

static httpd_handle_t server = NULL;
static esp_netif_t *ap_netif = NULL;
static bool server_running   = false;

// Commands API bridge
static httpd_req_t *s_current_req = NULL;

static void http_response_callback(const char *response)
{
    if (!s_current_req || !response)
        return;
    httpd_resp_sendstr(s_current_req, response);
}

static esp_err_t commands_api_handle_get(httpd_req_t *req, const char *command)
{
    httpd_resp_set_type(req, "application/json");
    s_current_req = req;
    commands_execute(command, http_response_callback);
    s_current_req = NULL;
    return ESP_OK;
}

static esp_err_t commands_api_handle_post(httpd_req_t *req, const char *command_fmt)
{
    char content[16384];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';

    char command[16512];
    snprintf(command, sizeof(command), command_fmt, content);

    httpd_resp_set_type(req, "application/json");
    s_current_req = req;
    commands_execute(command, http_response_callback);
    s_current_req = NULL;
    return ESP_OK;
}

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

// Serve static files from LittleFS
static esp_err_t serve_static_file(httpd_req_t *req, const char *filepath)
{
    ESP_LOGI(TAG, "Attempting to serve: %s", filepath);
    FILE *file = fopen(filepath, "r");

    // If file not found and path doesn't end with .html/.js/.css, try index.html
    if (!file) {
        const char *ext = strrchr(filepath, '.');
        if (!ext || (strcmp(ext, ".html") != 0 && strcmp(ext, ".js") != 0 && strcmp(ext, ".css") != 0 &&
                     strcmp(ext, ".json") != 0)) {
            char index_path[512];
            snprintf(index_path, sizeof(index_path), "%s%sindex.html", filepath,
                     (filepath[strlen(filepath) - 1] == '/') ? "" : "/");
            file = fopen(index_path, "r");
            if (file) {
                ESP_LOGI(TAG, "Serving index: %s", index_path);
            }
        }
    }

    if (!file) {
        ESP_LOGW(TAG, "File not found: %s", filepath);
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
static esp_err_t static_handler(httpd_req_t *req)
{
    char filepath[512];

    // If root is requested, serve index.html
    if (strcmp(req->uri, "/") == 0) {
        return serve_static_file(req, "/storage/index.html");
    }

    if (strlen(req->uri) > 500) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(filepath, sizeof(filepath), "/storage%s", req->uri);
#pragma GCC diagnostic pop
    return serve_static_file(req, filepath);
}

// New API handlers using commands_api
static esp_err_t api_general_get_handler(httpd_req_t *req)
{
    return commands_api_handle_get(req, "GET_GENERAL");
}

static esp_err_t api_general_post_handler(httpd_req_t *req)
{
    return commands_api_handle_post(req, "SET_GENERAL %s");
}

static esp_err_t api_power_get_handler(httpd_req_t *req)
{
    return commands_api_handle_get(req, "GET_POWER_MANAGEMENT");
}

static esp_err_t api_power_post_handler(httpd_req_t *req)
{
    return commands_api_handle_post(req, "SET_POWER_MANAGEMENT %s");
}

static esp_err_t api_lora_get_handler(httpd_req_t *req)
{
    return commands_api_handle_get(req, "GET_LORA");
}

static esp_err_t api_lora_post_handler(httpd_req_t *req)
{
    return commands_api_handle_post(req, "SET_LORA %s");
}

static esp_err_t api_devices_get_handler(httpd_req_t *req)
{
    return commands_api_handle_get(req, "GET_PAIRED_DEVICES");
}

static esp_err_t api_devices_post_handler(httpd_req_t *req)
{
    char content[16384];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';

    // Parse JSON to check if device exists (determine PAIR vs UPDATE)
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    const cJSON *mac = cJSON_GetObjectItem(json, "mac");
    bool is_update   = false;

    if (mac && cJSON_IsString(mac)) {
        // Parse MAC to get device_id
        uint8_t mac_bytes[6];
        if (sscanf(mac->valuestring, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac_bytes[0], &mac_bytes[1],
                   &mac_bytes[2], &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) == 6) {
            uint16_t device_id = (mac_bytes[4] << 8) | mac_bytes[5];
            paired_device_t existing;
            is_update = (device_registry_get(device_id, &existing) == ESP_OK);
        }
    }

    cJSON_Delete(json);

    // Use appropriate command
    char command[16512];
    snprintf(command, sizeof(command), is_update ? "UPDATE_PAIRED_DEVICE %s" : "PAIR_DEVICE %s", content);

    httpd_resp_set_type(req, "application/json");
    s_current_req = req;
    commands_execute(command, http_response_callback);
    s_current_req = NULL;
    return ESP_OK;
}

static esp_err_t api_devices_delete_handler(httpd_req_t *req)
{
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing MAC address");
        return ESP_FAIL;
    }
    content[ret] = '\0';

    char command[768];
    snprintf(command, sizeof(command), "UNPAIR_DEVICE %s", content);

    httpd_resp_set_type(req, "application/json");
    s_current_req = req;
    commands_execute(command, http_response_callback);
    s_current_req = NULL;
    return ESP_OK;
}

// Streaming firmware upload handler (OTA engine)
static esp_err_t api_firmware_upload_handler(httpd_req_t *req)
{
    size_t content_length = req->content_len;

    if (content_length == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No content");
        return ESP_FAIL;
    }

    esp_err_t ret = ota_engine_start(content_length);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA start failed: %s", esp_err_to_name(ret));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA start failed");
        return ESP_FAIL;
    }

    uint8_t buffer[4096];
    size_t received = 0;

    while (received < content_length) {
        int len = httpd_req_recv(req, (char *)buffer, sizeof(buffer));
        if (len <= 0) {
            if (len == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(TAG, "Receive failed");
            ota_engine_abort();
            return ESP_FAIL;
        }

        ret = ota_engine_write(buffer, len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(ret));
            ota_engine_abort();
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
            return ESP_FAIL;
        }

        received += len;
    }

    ret = ota_engine_finish();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(ret));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validation failed");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "{\"status\":\"success\"}");

    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();

    return ESP_OK;
}

// OTA progress endpoint
static esp_err_t api_firmware_progress_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    size_t progress   = ota_engine_get_progress();
    ota_state_t state = ota_engine_get_state();

    char response[128];
    snprintf(response, sizeof(response), "{\"progress\":%zu,\"state\":%d}", progress, state);

    httpd_resp_sendstr(req, response);
    return ESP_OK;
}

static esp_err_t api_device_info_handler(httpd_req_t *req)
{
    return commands_api_handle_get(req, "GET_DEVICE_INFO");
}

static esp_err_t factory_reset_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Factory reset initiated\"}", 52);

    vTaskDelay(pdMS_TO_TICKS(500));
    general_config_factory_reset();
    return ESP_OK;
}

esp_err_t config_wifi_server_start(void)
{
    if (server_running) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting WiFi AP and web server");

    // Initialize LittleFS
    esp_vfs_littlefs_conf_t littlefs_conf = {
        .base_path = "/storage", .partition_label = "storage", .format_if_mount_failed = true, .dont_mount = false};
    esp_err_t ret = esp_vfs_littlefs_register(&littlefs_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(ret));
    } else {
        size_t total = 0, used = 0;
        ret = esp_littlefs_info("storage", &total, &used);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "LittleFS: %zu KB total, %zu KB used", total / 1024, used / 1024);
        }
    }

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
    config.max_uri_handlers = 32;
    config.uri_match_fn     = httpd_uri_match_wildcard; // Enable wildcard matching
    config.stack_size       = 12288;                    // Increase to 12KB for file serving + WiFi operations

    if (httpd_start(&server, &config) == ESP_OK) {
        // API endpoints first (specific routes)
        httpd_uri_t api_general_get = {.uri = "/api/general", .method = HTTP_GET, .handler = api_general_get_handler};
        httpd_register_uri_handler(server, &api_general_get);

        httpd_uri_t api_general_post = {
            .uri = "/api/general", .method = HTTP_POST, .handler = api_general_post_handler};
        httpd_register_uri_handler(server, &api_general_post);

        httpd_uri_t api_power_get = {
            .uri = "/api/power-management", .method = HTTP_GET, .handler = api_power_get_handler};
        httpd_register_uri_handler(server, &api_power_get);

        httpd_uri_t api_power_post = {
            .uri = "/api/power-management", .method = HTTP_POST, .handler = api_power_post_handler};
        httpd_register_uri_handler(server, &api_power_post);

        httpd_uri_t api_lora_get = {.uri = "/api/lora*", .method = HTTP_GET, .handler = api_lora_get_handler};
        httpd_register_uri_handler(server, &api_lora_get);

        httpd_uri_t api_lora_post = {.uri = "/api/lora*", .method = HTTP_POST, .handler = api_lora_post_handler};
        httpd_register_uri_handler(server, &api_lora_post);

        httpd_uri_t api_devices_get = {.uri = "/api/devices", .method = HTTP_GET, .handler = api_devices_get_handler};
        httpd_register_uri_handler(server, &api_devices_get);

        httpd_uri_t api_devices_post = {
            .uri = "/api/devices", .method = HTTP_POST, .handler = api_devices_post_handler};
        httpd_register_uri_handler(server, &api_devices_post);

        httpd_uri_t api_devices_delete = {
            .uri = "/api/devices/*", .method = HTTP_DELETE, .handler = api_devices_delete_handler};
        httpd_register_uri_handler(server, &api_devices_delete);

        httpd_uri_t api_device_info = {
            .uri = "/api/device/info", .method = HTTP_GET, .handler = api_device_info_handler};
        httpd_register_uri_handler(server, &api_device_info);

        // Streaming firmware upload (OTA engine)
        httpd_uri_t fw_upload_uri = {
            .uri = "/api/firmware/upload", .method = HTTP_POST, .handler = api_firmware_upload_handler};
        httpd_register_uri_handler(server, &fw_upload_uri);

        httpd_uri_t fw_progress_uri = {
            .uri = "/api/firmware/progress", .method = HTTP_GET, .handler = api_firmware_progress_handler};
        httpd_register_uri_handler(server, &fw_progress_uri);

        // System API endpoints
        httpd_uri_t factory_reset_uri = {
            .uri = "/api/system/factory-reset", .method = HTTP_POST, .handler = factory_reset_handler};
        httpd_register_uri_handler(server, &factory_reset_uri);

        // Static file handler LAST - catch-all for everything else
        httpd_uri_t static_files = {.uri = "/*", .method = HTTP_GET, .handler = static_handler};
        httpd_register_uri_handler(server, &static_files);

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

    // Unmount LittleFS
    esp_vfs_littlefs_unregister("storage");

    // Only disable WiFi on real hardware for power saving
    esp_event_loop_delete_default();
    esp_netif_deinit();

    server_running = false;
    ESP_LOGI(TAG, "WiFi AP and web server stopped");

    return ESP_OK;
}

bool config_wifi_server_is_running(void)
{
    return server_running;
}
