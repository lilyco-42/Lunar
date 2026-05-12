#include "bh1750.h"
#include "config.h"
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "bh1750";

#define BH1750_ADDR         BH1750_I2C_ADDR

/* Commands (no data, just send the opcode) */
#define BH1750_CMD_POWER_ON           0x01
#define BH1750_CMD_RESET              0x07
#define BH1750_CMD_CONT_HRES          0x10   /* 1 lx resolution, 120ms */
#define BH1750_CMD_CONT_HRES2         0x11   /* 0.5 lx resolution, 120ms */

int bh1750_init(void)
{
    int ret;

    /* Power on */
    ret = i2c_master_write_to_device(I2C_MASTER_PORT, BH1750_ADDR,
                                     (uint8_t[]){BH1750_CMD_POWER_ON}, 1,
                                     pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Power-on failed: %d", ret);
        return ret;
    }

    /* Set continuous high-res mode */
    ret = i2c_master_write_to_device(I2C_MASTER_PORT, BH1750_ADDR,
                                     (uint8_t[]){BH1750_CMD_CONT_HRES}, 1,
                                     pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mode set failed: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Initialized, addr 0x%02X", BH1750_ADDR);
    return 0;
}

float bh1750_read_lux(void)
{
    uint8_t buf[2] = {0};
    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_PORT, BH1750_ADDR,
                                                 buf, 2, pdMS_TO_TICKS(200));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %d", ret);
        return -1.0f;
    }

    uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
    /* In HRES mode: raw / 1.2 = lux */
    return raw / 1.2f;
}
