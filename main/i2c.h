#ifndef I2C_HELPER_H
#define I2C_HELPER_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

esp_err_t i2c_init(void);
esp_err_t i2c_write(uint8_t devaddr, const uint8_t *data, size_t size);
esp_err_t i2c_read(uint8_t devaddr, const uint8_t *reg, size_t reg_size, uint8_t *out, size_t out_size);

#endif // I2C_HELPER_H
