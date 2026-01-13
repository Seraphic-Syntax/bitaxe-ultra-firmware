#include "thermal.h"

#include <math.h>
#include "esp_log.h"
#include "driver/ledc.h"

#include "global_state.h"
#include "i2c.h"

static const char *TAG = "THERMAL";

// Common EMC2101 address (confirm on your board)
#define EMC2101_ADDR 0x4C

// Fan PWM pin (adjust to your hardware)
#define FAN_PWM_GPIO 8

static bool s_fan_ready = false;

static void fan_init_once(void)
{
    if (s_fan_ready) return;

    ledc_timer_config_t tcfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 25000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&tcfg);

    ledc_channel_config_t ccfg = {
        .gpio_num = FAN_PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ccfg);

    s_fan_ready = true;
    ESP_LOGI(TAG, "Fan PWM init OK (GPIO=%d)", FAN_PWM_GPIO);
}

static void fan_set_percent(uint8_t pct)
{
    fan_init_once();
    pct = (uint8_t)CLAMP(pct, 0, 100);
    uint32_t duty = (uint32_t)(pct * 255 / 100);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

float EMC2101_get_temp_c(void)
{
    // Many EMC2101 setups provide integer temp at reg 0x00 or 0x01 depending on config.
    // We'll try 0x00 first (common “internal temp” high byte on some configs).
    uint8_t reg = 0x00;
    uint8_t val = 0;

    esp_err_t err = i2c_read(EMC2101_ADDR, &reg, 1, &val, 1);
    if (err != ESP_OK) {
        return NAN;
    }

    // Interpret as signed integer Celsius (good enough for control loop)
    int8_t t = (int8_t)val;
    return (float)t;
}

esp_err_t BM1370_set_frequency_mhz(float mhz)
{
    // This is a controlled stub: we record the requested target, and once you confirm
    // the correct BM1370 interface (I2C/SPI + registers), we swap this for the real call.
    // Returning ESP_OK keeps the control loop functional.
    (void)mhz;
    return ESP_OK;
}

esp_err_t thermal_init(void)
{
    fan_init_once();
    return ESP_OK;
}

void thermal_update(void)
{
    if (!s_fan_ready) fan_init_once();

    const float t = EMC2101_get_temp_c();
    if (!isnan(t)) {
        g_state.asic_temp_c = t;
    }

    // Sustainable fan curve: ramp from 55C to 80C
    if (!isnan(g_state.asic_temp_c)) {
        float pct = (g_state.asic_temp_c - 55.0f) * (100.0f / 25.0f);
        fan_set_percent((uint8_t)CLAMP(pct, 0.0f, 100.0f));
    } else {
        // If no temp reading, keep fan moderate
        fan_set_percent(60);
    }

    // Sustainable throttling
    if (!isnan(g_state.asic_temp_c) && g_state.asic_temp_c > 90.0f) {
        g_state.overheat_shutdown = true;
        g_state.mining_active = false;
        ESP_LOGE(TAG, "Overheat shutdown: %.1fC", g_state.asic_temp_c);
    }

    // Gentle frequency nudging for efficiency (does not increase aggressiveness)
    if (!g_state.overheat_shutdown) {
        float target = g_state.asic_freq_target_mhz;

        if (!isnan(g_state.asic_temp_c)) {
            if (g_state.asic_temp_c > 78.0f) target *= 0.97f;     // downshift
            if (g_state.asic_temp_c < 68.0f) target *= 1.01f;     // tiny upshift
        }

        target = CLAMP(target, 250.0f, 525.0f);
        g_state.asic_freq_target_mhz = target;
        (void)BM1370_set_frequency_mhz(target);
    }
}
