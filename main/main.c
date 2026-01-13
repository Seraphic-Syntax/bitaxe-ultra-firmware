#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "global_state.h"
#include "i2c.h"
#include "thermal.h"
#include "vcore.h"
#include "mining.h"

static const char *TAG = "APP";

// Edit these:
#define WIFI_SSID "STOP 2"
#define WIFI_PASS "WIFI301095"

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
    ESP_ERROR_CHECK(nvs_flash_init());

    GlobalState_Init();

    ESP_ERROR_CHECK(i2c_init());
    ESP_ERROR_CHECK(thermal_init());
    ESP_ERROR_CHECK(vcore_init());
    mining_init();

    vcore_set_target(1.05f);

    // Optional: comment out if you want local-only bring-up
    wifi_start();

    xTaskCreate(worker_task, "worker", 8192, NULL, 10, NULL);
}
