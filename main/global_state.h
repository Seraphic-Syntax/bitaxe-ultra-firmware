#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// Type-safe clamp (prevents warnings with uint8_t/uint32_t/float, etc.)
#ifndef CLAMP
#define CLAMP(x, lo, hi) ({                \
    __typeof__(x) _x  = (x);               \
    __typeof__(x) _lo = (lo);              \
    __typeof__(x) _hi = (hi);              \
    _x < _lo ? _lo : (_x > _hi ? _hi : _x);\
})
#endif

typedef struct {
    // Mining stats
    uint64_t hash_count;
    float    hash_rate_khs;     // kH/s (software loop estimate)
    uint32_t valid_shares;
    uint32_t invalid_shares;

    // Thermal / power / tuning
    float asic_temp_c;
    float asic_freq_target_mhz;
    float core_voltage_v;
    float efficiency_avg_j_th;  // placeholder model for now

    // Control flags
    bool mining_active;
    bool overheat_shutdown;

    // Placeholder for later stratum integration
    char pool_url[128];
} GlobalState;

extern GlobalState g_state;

void GlobalState_Init(void);
void GlobalState_Update(void);

#endif // GLOBAL_STATE_H
