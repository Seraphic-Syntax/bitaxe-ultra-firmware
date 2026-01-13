idf_component_register(SRCS "main.c" "http_server.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_http_server json esp_wifi nvs_flash driver)
