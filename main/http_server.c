#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cjson/cJSON.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"

// Assume your GlobalState (extern or include)
extern struct GlobalState g_state;  // Map this to your actual global

static const char *TAG = "http_server";

#define CORS_ALLOW_ORIGIN "*"
#define CORS_ALLOW_METHODS "GET, POST, PUT, PATCH, DELETE, OPTIONS"
#define CORS_ALLOW_HEADERS "Content-Type"

// Helper: Send CORS headers + JSON
static esp_err_t send_json_response(httpd_req_t *req, cJSON *json_root) {
    char *json_str = cJSON_PrintUnformatted(json_root);
    if (!json_str) return ESP_FAIL;

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", CORS_ALLOW_ORIGIN);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", CORS_ALLOW_METHODS);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", CORS_ALLOW_HEADERS);
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");

    esp_err_t ret = httpd_resp_send(req, json_str, strlen(json_str));
    free(json_str);
    cJSON_Delete(json_root);
    return ret;
}

// OPTIONS preflight (CORS)
static esp_err_t cors_options_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "204");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", CORS_ALLOW_ORIGIN);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", CORS_ALLOW_METHODS);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", CORS_ALLOW_HEADERS);
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// /api/system/info (KEY endpoint)
static esp_err_t api_system_info_handler(httpd_req_t *req) {
    if (req->method == HTTP_METHOD_OPTIONS) return cors_options_handler(req);

    cJSON *root = cJSON_CreateObject();

    // Temps (map g_state)
    cJSON_AddNumberToObject(root, "temp", g_state.asic_temp_c);  // °C

    // Performance (GH/s!)
    cJSON_AddNumberToObject(root, "hashRate", g_state.hash_rate_khs / 1000000.0);
    cJSON_AddNumberToObject(root, "hashRate_1m", g_state.hash_rate_khs / 1000000.0);  // TODO: avg
    cJSON_AddNumberToObject(root, "hashRate_10m", g_state.hash_rate_khs / 1000000.0);
    cJSON_AddNumberToObject(root, "hashRate_1h", g_state.hash_rate_khs / 1000000.0);
    cJSON_AddNumberToObject(root, "expectedHashrate", 1.0);  // TODO

    cJSON_AddNumberToObject(root, "bestDiff", 0);
    cJSON_AddNumberToObject(root, "bestSessionDiff", 0);

    // Shares
    cJSON_AddNumberToObject(root, "sharesAccepted", g_state.valid_shares);
    cJSON_AddNumberToObject(root, "sharesRejected", g_state.invalid_shares);
    cJSON *reasons = cJSON_AddArrayToObject(root, "sharesRejectedReasons");
    cJSON_AddItemToArray(reasons, cJSON_CreateObject());  // Empty for now

    // Electrical (placeholders → TODO ADC)
    cJSON_AddNumberToObject(root, "power", 22.5);
    cJSON_AddNumberToObject(root, "voltage", 5.1875);  // Scaled
    cJSON_AddNumberToObject(root, "current", 4500.0);
    cJSON_AddNumberToObject(root, "coreVoltage", g_state.core_voltage_v * 1000);  // mV
    cJSON_AddNumberToObject(root, "coreVoltageActual", g_state.core_voltage_v * 1000);

    // Freq
    cJSON_AddNumberToObject(root, "frequency", g_state.asic_freq_target_mhz * 1000);  // kHz?

    // System
    cJSON_AddNumberToObject(root, "freeHeap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "uptimeSeconds", (uint64_t)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000));
    cJSON_AddStringToObject(root, "version", "2.13.0b1");  // Mimic stock
    cJSON_AddStringToObject(root, "axeOSVersion", "2.13.0b1");  // Match to hide banner

    // WiFi (example)
    uint8_t mac[6]; esp_efuse_mac_get_default(mac);
    char mac_str[18]; sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    cJSON_AddStringToObject(root, "macAddress", mac_str);

    // Extras (AxeOS ignores)
    cJSON_AddNumberToObject(root, "asicTempC", g_state.asic_temp_c);  // Custom

    return send_json_response(req, root);
}

// /api/theme (hardcode stock theme)
static esp_err_t api_theme_handler(httpd_req_t *req) {
    if (req->method == HTTP_METHOD_OPTIONS) return cors_options_handler(req);

    cJSON *root = cJSON_CreateObject();
    cJSON *colors = cJSON_AddObjectToObject(root, "accentColors");
    cJSON_AddStringToObject(colors, "--bg-primary", "#0a0a0a");
    cJSON_AddStringToObject(colors, "--bg-secondary", "#1a1a1a");
    // Add more from your stock /api/theme (hardcoded for now)
    cJSON_AddStringToObject(root, "colorScheme", "dark");

    return send_json_response(req, root);
}

// /api/system/statistics (minimal)
static esp_err_t api_statistics_handler(httpd_req_t *req) {
    if (req->method == HTTP_METHOD_OPTIONS) return cors_options_handler(req);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "currentTimestamp", (esp_timer_get_time() / 1000));
    cJSON *labels = cJSON_AddArrayToObject(root, "labels");
    cJSON_AddItemToArray(labels, cJSON_CreateString("hashrate"));
    cJSON_AddItemToArray(labels, cJSON_CreateString("power"));
    cJSON_AddItemToArray(labels, cJSON_CreateString("hashrate_1m"));
    cJSON_AddEmptyArray(root, "statistics");  // Empty OK

    return send_json_response(req, root);
}

// URI config
static const httpd_uri_t uri_info = { .uri = "/api/system/info", .method = HTTP_GET, .handler = api_system_info_handler };
static const httpd_uri_t uri_theme = { .uri = "/api/theme", .method = HTTP_GET, .handler = api_theme_handler };
static const httpd_uri_t uri_stats = { .uri = "/api/system/statistics", .method = HTTP_GET, .handler = api_statistics_handler };

esp_err_t http_server_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting HTTP server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_info);
        httpd_register_uri_handler(server, &uri_theme);
        httpd_register_uri_handler(server, &uri_stats);
        ESP_LOGI(TAG, "HTTP server started successfully");
        return ESP_OK;
    }
    ESP_LOGE(TAG, "Error starting server!");
    return ESP_FAIL;
}
