#include "global_state.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "GLOBAL";

GlobalState g_state;

void GlobalState_Init(void)
{
    memset(&g_state, 0, sizeof(g_state));

    g_state.asic_temp_c = NAN;
    g_state.asic_freq_target_mhz = 400.0f;
    g_state.core_voltage_v = 1.05f;

    g_state.mining_active = true;
    g_state.overheat_shutdown = false;

    // Placeholder for later pool work
    strncpy(g_state.pool_url, "stratum+tcp://example:3333", sizeof(g_state.pool_url) - 1);

    ESP_LOGI(TAG, "Init: freq=%.0f MHz vcore=%.2f V", g_state.asic_freq_target_mhz, g_state.core_voltage_v);
}

void GlobalState_Update(void)
{
    static uint64_t last_hash_count = 0;
    static TickType_t last_tick = 0;

    TickType_t now = xTaskGetTickCount();
    if (last_tick == 0) {
        last_tick = now;
        last_hash_count = g_state.hash_count;
        return;
    }

    const uint32_t dt_ticks = (uint32_t)(now - last_tick);
    if (dt_ticks == 0) return;

    const float dt_s = (dt_ticks * portTICK_PERIOD_MS) / 1000.0f;
    const uint64_t dh = g_state.hash_count - last_hash_count;

    // hashes/sec -> kH/s
    g_state.hash_rate_khs = (float)dh / dt_s / 1000.0f;

    // Placeholder “efficiency”: assume fixed 5W for now, refine later with real power telemetry
    const float assumed_watts = 5.0f;
    const float ths = (g_state.hash_rate_khs / 1000.0f) / 1000.0f; // kH/s -> MH/s -> GH/s -> TH/s (very rough)
    g_state.efficiency_avg_j_th = (ths > 0.0f) ? (assumed_watts / ths) : NAN;

    last_tick = now;
    last_hash_count = g_state.hash_count;
}
