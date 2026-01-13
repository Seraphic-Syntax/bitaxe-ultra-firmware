#include "i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "I2C";

// Adjust if your Bitaxe variant uses different pins
#define I2C_PORT        I2C_NUM_0
#define I2C_SDA_GPIO    21
#define I2C_SCL_GPIO    22
#define I2C_FREQ_HZ     400000
#define I2C_TIMEOUT_MS  1000

esp_err_t i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
        .clk_flags = 0,
    };

    esp_err_t err = i2c_param_config(I2C_PORT, &conf);
    if (err != ESP_OK) return err;

    err = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "I2C init OK (SDA=%d SCL=%d %dHz)", I2C_SDA_GPIO, I2C_SCL_GPIO, I2C_FREQ_HZ);
    }
    return err;
}

esp_err_t i2c_write(uint8_t devaddr, const uint8_t *data, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devaddr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, (uint8_t *)data, size, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_read(uint8_t devaddr, const uint8_t *reg, size_t reg_size, uint8_t *out, size_t out_size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Write register pointer
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devaddr << 1) | I2C_MASTER_WRITE, true);
    if (reg && reg_size) {
        i2c_master_write(cmd, (uint8_t *)reg, reg_size, true);
    }

    // Re-start for read
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devaddr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, out, out_size, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

