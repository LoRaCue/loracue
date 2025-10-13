#include "config_wifi_server.h"
#include "commands.h"
#include "cJSON.h"
#include "general_config.h"
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
#include "power_mgmt_config.h"
#include "version.h"
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "CONFIG_WIFI_SERVER";

static httpd_handle_t server = NULL;
static esp_netif_t *ap_netif = NULL;
static bool server_running   = false;

// Commands API bridge
static httpd_req_t *g_current_req = NULL;

static void http_response_callback(const char *response)
{
    if (!g_current_req || !response) return;
    httpd_resp_sendstr(g_current_req, response);
}

static esp_err_t commands_api_handle_get(httpd_req_t *req, const char *command)
{
    httpd_resp_set_type(req, "application/json");
    g_current_req = req;
    commands_execute(command, http_response_callback);
    g_current_req = NULL;
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
    g_current_req = req;
    commands_execute(command, http_response_callback);
    g_current_req = NULL;
    return ESP_OK;
}

static esp_err_t commands_api_handle_delete(httpd_req_t *req, const char *command_fmt)
{
    const char *uri = req->uri;
    const char *param = strrchr(uri, '/');
    if (!param || !*(param + 1)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing parameter");
        return ESP_FAIL;
    }

    char command[256];
    snprintf(command, sizeof(command), command_fmt, param + 1);

    httpd_resp_set_type(req, "application/json");
    g_current_req = req;
    commands_execute(command, http_response_callback);
    g_current_req = NULL;
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
    return commands_api_handle_post(req, "PAIR_DEVICE %s");
}

static esp_err_t api_devices_delete_handler(httpd_req_t *req)
{
    return commands_api_handle_delete(req, "UNPAIR_DEVICE %s");
}

// Firmware update API handlers (using commands system)
static esp_err_t api_firmware_start_handler(httpd_req_t *req)
{
    return commands_api_handle_post(req, "FW_UPDATE_START %s");
}

static esp_err_t api_firmware_data_handler(httpd_req_t *req)
{
    return commands_api_handle_post(req, "FW_UPDATE_DATA %s");
}

static esp_err_t api_firmware_verify_handler(httpd_req_t *req)
{
    return commands_api_handle_post(req, "FW_UPDATE_VERIFY");
}

static esp_err_t api_firmware_abort_handler(httpd_req_t *req)
{
    return commands_api_handle_post(req, "FW_UPDATE_ABORT");
}

static esp_err_t api_firmware_commit_handler(httpd_req_t *req)
{
    return commands_api_handle_post(req, "FW_UPDATE_COMMIT");
}

static esp_err_t api_device_info_handler(httpd_req_t *req)
{
    return commands_api_handle_get(req, "GET_DEVICE_INFO");
}

static esp_err_t factory_reset_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Factory reset initiated\"}", 52);

    vTaskDelay(pdMS_TO_TICKS(1000));
    general_config_factory_reset();
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
    config.max_uri_handlers = 32;

    if (httpd_start(&server, &config) == ESP_OK) {
        // Static file handlers
        httpd_uri_t root_uri = {.uri = "/", .method = HTTP_GET, .handler = root_handler};
        httpd_register_uri_handler(server, &root_uri);

        httpd_uri_t static_uri = {.uri = "/*", .method = HTTP_GET, .handler = static_handler};
        httpd_register_uri_handler(server, &static_uri);

        // New unified API endpoints (using commands_api)
        httpd_uri_t api_general_get = {.uri = "/api/general", .method = HTTP_GET, .handler = api_general_get_handler};
        httpd_register_uri_handler(server, &api_general_get);

        httpd_uri_t api_general_post = {.uri = "/api/general", .method = HTTP_POST, .handler = api_general_post_handler};
        httpd_register_uri_handler(server, &api_general_post);

        httpd_uri_t api_power_get = {.uri = "/api/power-management", .method = HTTP_GET, .handler = api_power_get_handler};
        httpd_register_uri_handler(server, &api_power_get);

        httpd_uri_t api_power_post = {.uri = "/api/power-management", .method = HTTP_POST, .handler = api_power_post_handler};
        httpd_register_uri_handler(server, &api_power_post);

        httpd_uri_t api_lora_get = {.uri = "/api/lora", .method = HTTP_GET, .handler = api_lora_get_handler};
        httpd_register_uri_handler(server, &api_lora_get);

        httpd_uri_t api_lora_post = {.uri = "/api/lora", .method = HTTP_POST, .handler = api_lora_post_handler};
        httpd_register_uri_handler(server, &api_lora_post);

        httpd_uri_t api_devices_get = {.uri = "/api/devices", .method = HTTP_GET, .handler = api_devices_get_handler};
        httpd_register_uri_handler(server, &api_devices_get);

        httpd_uri_t api_devices_post = {.uri = "/api/devices", .method = HTTP_POST, .handler = api_devices_post_handler};
        httpd_register_uri_handler(server, &api_devices_post);

        httpd_uri_t api_devices_delete = {.uri = "/api/devices/*", .method = HTTP_DELETE, .handler = api_devices_delete_handler};
        httpd_register_uri_handler(server, &api_devices_delete);

        httpd_uri_t api_device_info = {.uri = "/api/device/info", .method = HTTP_GET, .handler = api_device_info_handler};
        httpd_register_uri_handler(server, &api_device_info);

        // Firmware update API endpoints
        httpd_uri_t fw_start_uri = {
            .uri = "/api/firmware/start", .method = HTTP_POST, .handler = api_firmware_start_handler};
        httpd_register_uri_handler(server, &fw_start_uri);

        httpd_uri_t fw_data_uri = {.uri = "/api/firmware/data", .method = HTTP_POST, .handler = api_firmware_data_handler};
        httpd_register_uri_handler(server, &fw_data_uri);

        httpd_uri_t fw_verify_uri = {
            .uri = "/api/firmware/verify", .method = HTTP_POST, .handler = api_firmware_verify_handler};
        httpd_register_uri_handler(server, &fw_verify_uri);

        httpd_uri_t fw_abort_uri = {
            .uri = "/api/firmware/abort", .method = HTTP_POST, .handler = api_firmware_abort_handler};
        httpd_register_uri_handler(server, &fw_abort_uri);

        httpd_uri_t fw_commit_uri = {
            .uri = "/api/firmware/commit", .method = HTTP_POST, .handler = api_firmware_commit_handler};
        httpd_register_uri_handler(server, &fw_commit_uri);

        // System API endpoints
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
