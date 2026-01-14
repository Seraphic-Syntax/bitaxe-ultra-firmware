#include <string.h>
#include <inttypes.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "http_server.h"
#include "esp_wifi.h"

#include "global_state.h"
#include "i2c.h"
#include "thermal.h"
#include "vcore.h"
#include "mining.h"


static const char *TAG = "APP";

// Edit these:
#define WIFI_SSID "WIFI NETWORK"
#define WIFI_PASS "WIFI PASSWORD"

static void wifi_start(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    (void)esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi started (SSID=%s)", WIFI_SSID);
}

static void worker_task(void *arg)
{
    (void)arg;

    while (1) {
        if (g_state.mining_active && !g_state.overheat_shutdown) {
            mining_loop_once();
        }

        thermal_update();
        vcore_update();
        GlobalState_Update();

        static int ctr = 0;
        ctr++;
        if (ctr % 4 == 0) { // ~ every 2 seconds
            ESP_LOGI(TAG,
                     "Rate=%.2f kH/s Temp=%.1fC Freq=%.0fMHz Vcore=%.3fV Shares=%" PRIu32 "/%" PRIu32,
                     g_state.hash_rate_khs,
                     g_state.asic_temp_c,
                     g_state.asic_freq_target_mhz,
                     g_state.core_voltage_v,
                     g_state.valid_shares,
                     g_state.invalid_shares);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // --- NEW CODE START ---
    ESP_ERROR_CHECK(g_state_init());
    
    // Capture boot time for Uptime Counter
    g_state.boot_time_us = esp_timer_get_time();
    
    // Set Default Fake Data (until sensors are real)
    g_state.power_w = 12.5; 
    g_state.fan_rpm = 4500;
    // --- NEW CODE END ---

    // ... (rest of your existing init code: temp, vcore, wifi, etc) ...
    
    // START WIFI
    wifi_start();

    // START SERVER
    http_server_start();
}
