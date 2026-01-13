#ifndef THERMAL_H
#define THERMAL_H

#include "esp_err.h"

esp_err_t thermal_init(void);
void thermal_update(void);

// EMC2101 temp sensor (common on Bitaxe-like builds)
float EMC2101_get_temp_c(void);

// ASIC frequency setter (stubbed until BM1370 register interface is confirmed)
esp_err_t BM1370_set_frequency_mhz(float mhz);

#endif // THERMAL_H
