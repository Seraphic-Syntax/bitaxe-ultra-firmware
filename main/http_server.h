#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_http_server.h"
#include "cjson/cJSON.h"  // ESP-IDF json component

esp_err_t http_server_start(void);

#endif
