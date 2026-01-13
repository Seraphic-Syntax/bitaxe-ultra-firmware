#include "mining.h"

#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "mbedtls/sha256.h"
#include "global_state.h"

static const char *TAG = "MINING";

static void sha256d(const uint8_t *data, size_t len, uint8_t out32[32])
{
    uint8_t tmp[32];
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, data, len);
    mbedtls_sha256_finish(&ctx, tmp);
    mbedtls_sha256_free(&ctx);

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, tmp, 32);
    mbedtls_sha256_finish(&ctx, out32);
    mbedtls_sha256_free(&ctx);
}

static bool meets_toy_target(const uint8_t h[32])
{
    // Toy “share target” for demo: require first byte == 0
    return (h[0] == 0x00);
}

void mining_init(void)
{
    ESP_LOGI(TAG, "Mining init (software demo loop; ASIC interface can be added next)");
}

void mining_loop_once(void)
{
    static uint32_t nonce = 0;

    uint8_t header[80];
    memset(header, 0, sizeof(header));
    memcpy(header, "bitaxe_ultra_demo_job", 21);

    header[76] = (uint8_t)(nonce);
    header[77] = (uint8_t)(nonce >> 8);
    header[78] = (uint8_t)(nonce >> 16);
    header[79] = (uint8_t)(nonce >> 24);

    uint8_t h[32];
    sha256d(header, sizeof(header), h);

    g_state.hash_count++;
    nonce++;

    if (meets_toy_target(h)) {
        g_state.valid_shares++;
        ESP_LOGW(TAG, "VALID (toy) share found nonce=%" PRIu32, nonce);
    }
}
