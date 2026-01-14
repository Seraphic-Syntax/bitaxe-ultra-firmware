#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // Mining Stats
    double hash_rate_khs;
    uint64_t valid_shares;
    uint64_t invalid_shares;

    // Hardware Stats
    float asic_temp_c;
    float asic_freq_target_mhz;
    float core_voltage_v;
    
    // NEW FIELDS FOR AXEOS
    float power_w;          // Calculated Power
    uint32_t fan_rpm;       // Fan speed
    int64_t boot_time_us;   // For uptime calculation

} GlobalState;

extern GlobalState g_state;

// Helper to init defaults
esp_err_t g_state_init(void);

#endif
