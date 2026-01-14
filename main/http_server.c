#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "global_state.h"
#include "esp_timer.h"
#include <sys/stat.h>

static const char *TAG = "HTTP";

/* ================= UTILITIES ================= */

static uint32_t get_uptime_sec() {
    return (uint32_t)((esp_timer_get_time() - g_state.boot_time_us) / 1000000);
}

/* ================= API HANDLERS ================= */

// GET /api/system/info
static esp_err_t api_system_info_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    cJSON *root = cJSON_CreateObject();

    // Mining Data
    double hash_gh = g_state.hash_rate_khs / 1000000.0;
    cJSON_AddNumberToObject(root, "hashRate", hash_gh);
    cJSON_AddNumberToObject(root, "hashRate_1m", hash_gh); 
    cJSON_AddNumberToObject(root, "hashRate_1h", hash_gh);

    // Sensors
    cJSON_AddNumberToObject(root, "temp", g_state.asic_temp_c);
    cJSON_AddNumberToObject(root, "vrTemp", g_state.asic_temp_c - 8.0); // Simulated VR diff
    cJSON_AddNumberToObject(root, "power", g_state.power_w);
    cJSON_AddNumberToObject(root, "coreVoltage", g_state.core_voltage_v * 1000.0); // V to mV
    cJSON_AddNumberToObject(root, "frequency", g_state.asic_freq_target_mhz); 
    cJSON_AddNumberToObject(root, "fanRPM", g_state.fan_rpm);
    
    // Shares & System
    cJSON_AddNumberToObject(root, "sharesAccepted", g_state.valid_shares);
    cJSON_AddNumberToObject(root, "sharesRejected", g_state.invalid_shares);
    cJSON_AddNumberToObject(root, "uptimeSeconds", get_uptime_sec());

    // AxeOS Metadata (Masks the custom firmware)
    cJSON_AddStringToObject(root, "version", "2.1.0-ultra-custom");
    cJSON_AddStringToObject(root, "axeOSVersion", "2.13.0");
    cJSON_AddStringToObject(root, "boardVersion", "Bitaxe Ultra 204");
    cJSON_AddStringToObject(root, "wifiSSID", "Bitaxe_Network");
    cJSON_AddStringToObject(root, "hostname", "bitaxe-ultra");

    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
    
    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

// GET /api/theme
static esp_err_t api_theme_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    const char *resp = "{\"colorScheme\":\"dark\",\"accentColors\":{}}";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// GET /api/system/statistics (Graphs)
static esp_err_t api_stats_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    // Empty stats prevents UI crash
    const char *resp = "{\"currentTimestamp\":0,\"labels\":[\"hr\"],\"statistics\":[]}";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// OPTIONS (CORS)
static esp_err_t api_options_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/* ================= STATIC FILES (WWW) ================= */

// Helper: Get MIME type
static const char* get_content_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "text/plain";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0)  return "text/css";
    if (strcmp(ext, ".js") == 0)   return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0)  return "image/png";
    return "text/plain";
}

// Handler: Serve files from SPIFFS
static esp_err_t www_file_handler(httpd_req_t *req)
{
    char filepath[600];
    
    // 1. Map root to index.html
    if (strcmp(req->uri, "/") == 0) {
        strcpy(filepath, "/www/index.html");
    } else {
        snprintf(filepath, sizeof(filepath), "/www%s", req->uri);
    }

    // 2. Check if file exists
    struct stat st;
    if (stat(filepath, &st) != 0) {
        // 3. Fallback to index.html for SPA routing (except for API calls)
        if (strncmp(req->uri, "/api", 4) == 0) {
            httpd_resp_send_404(req);
            return ESP_OK;
        }
        strcpy(filepath, "/www/index.html");
    }

    FILE *fd = fopen(filepath, "r");
    if (!fd) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, get_content_type(filepath));

    char *chunk = malloc(4096);
    size_t chunksize;
    while ((chunksize = fread(chunk, 1, 4096, fd)) > 0) {
        httpd_resp_send_chunk(req, chunk, chunksize);
    }
    free(chunk);
    fclose(fd);
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

/* ================= STARTUP ================= */

static esp_err_t init_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/www",
        .partition_label = "www",
        .max_files = 10,
        .format_if_mount_failed = true
    };
    return esp_vfs_spiffs_register(&conf);
}

esp_err_t http_server_start(void)
{
    ESP_ERROR_CHECK(init_spiffs());

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 20;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        // APIs
        httpd_uri_t sys_info = { .uri = "/api/system/info", .method = HTTP_GET, .handler = api_system_info_handler };
        httpd_register_uri_handler(server, &sys_info);
        
        httpd_uri_t theme = { .uri = "/api/theme", .method = HTTP_GET, .handler = api_theme_handler };
        httpd_register_uri_handler(server, &theme);

        httpd_uri_t stats = { .uri = "/api/system/statistics", .method = HTTP_GET, .handler = api_stats_handler };
        httpd_register_uri_handler(server, &stats);

        httpd_uri_t options = { .uri = "/api/*", .method = HTTP_OPTIONS, .handler = api_options_handler };
        httpd_register_uri_handler(server, &options);

        // WWW Catch-all (Must be last)
        httpd_uri_t www = { .uri = "/*", .method = HTTP_GET, .handler = www_file_handler };
        httpd_register_uri_handler(server, &www);
        
        ESP_LOGI(TAG, "Web Server Started");
        return ESP_OK;
    }
    return ESP_FAIL;
}
