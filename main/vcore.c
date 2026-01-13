#include "vcore.h"

#include <math.h>
#include "esp_log.h"
#include "global_state.h"

static const char *TAG = "VCORE";

esp_err_t vcore_init(void)
{
    // Hardware-specific on Bitaxe (buck regulator / DAC / digipot).
    // For now, this is a safe software-side controller stub.
    return ESP_OK;
}

void vcore_set_target(float volts)
{
    // Conservative safe range placeholder
    g_state.core_voltage_v = CLAMP(volts, 0.90f, 1.15f);
    ESP_LOGI(TAG, "Target Vcore set: %.3f V (stub)", g_state.core_voltage_v);
}

void vcore_update(void)
{
    // Sustainable undervolt bias if cool (placeholder)
    if (!g_state.overheat_shutdown && !isnan(g_state.asic_temp_c) && g_state.asic_temp_c < 70.0f) {
        // nudge down very slightly (bounded)
        float v = g_state.core_voltage_v - 0.001f;
        vcore_set_target(v);
    }
}
