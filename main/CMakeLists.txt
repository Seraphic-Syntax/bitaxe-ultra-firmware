idf_component_register(
    SRCS
        "main.c"
        "global_state.c"
        "i2c.c"
        "thermal.c"
        "vcore.c"
        "mining.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        freertos
        nvs_flash
        esp_event
        esp_netif
        esp_wifi
        mbedtls
)
