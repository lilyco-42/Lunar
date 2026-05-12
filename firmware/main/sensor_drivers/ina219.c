#include "ina219.h"
#include "config.h"
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "ina219";

#define INA219_ADDR         INA219_I2C_ADDR

/* Register addresses */
#define REG_CONFIG          0x00
#define REG_SHUNT_VOLTAGE   0x01
#define REG_BUS_VOLTAGE     0x02
#define REG_POWER           0x03
#define REG_CURRENT         0x04
#define REG_CALIBRATION     0x05

/* Calibration: 0.1Ω shunt, 3.2A max, LSB = 50μA per bit (12-bit ADC defaults) */
#define CAL_VALUE           4096
#define CURRENT_LSB_UA      50

/* ─── Helpers ─── */

static esp_err_t write_reg16(uint8_t reg, uint16_t val)
{
    uint8_t buf[3] = {reg, (val >> 8) & 0xFF, val & 0xFF};
    return i2c_master_write_to_device(I2C_MASTER_PORT, INA219_ADDR,
                                      buf, 3, pdMS_TO_TICKS(100));
}

static esp_err_t read_reg16(uint8_t reg, uint16_t *val)
{
    uint8_t buf[2] = {0};
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_PORT, INA219_ADDR,
                                                   &reg, 1, buf, 2,
                                                   pdMS_TO_TICKS(100));
    if (ret == ESP_OK) {
        *val = ((uint16_t)buf[0] << 8) | buf[1];
    }
    return ret;
}

/* ─── Public API ─── */

int ina219_init(void)
{
    /* Reset */
    int ret = write_reg16(REG_CONFIG, 0x8000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Reset failed: %d", ret);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(5));

    /* Set calibration */
    ret = write_reg16(REG_CALIBRATION, CAL_VALUE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Calibration write failed: %d", ret);
        return ret;
    }

    /* Config: 32V range, 12-bit ADC (both shunt+ bus), continuous */
    uint16_t cfg = 0x019F;   /* BRNG=1(32V), PG=00(±40mV), BADC=3, SADC=3, MODE=7 */
    ret = write_reg16(REG_CONFIG, cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Config write failed: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Initialized, addr 0x%02X", INA219_ADDR);
    return 0;
}

float ina219_read_bus_voltage(void)
{
    uint16_t raw;
    if (read_reg16(REG_BUS_VOLTAGE, &raw) != ESP_OK) return -1.0f;
    /* Lower 3 bits are status flags; shift right 3, x4mV LSB */
    return ((raw >> 3) * 4) / 1000.0f;
}

float ina219_read_shunt_voltage_mv(void)
{
    uint16_t raw;
    if (read_reg16(REG_SHUNT_VOLTAGE, &raw) != ESP_OK) return -1.0f;
    /* Two's complement 16-bit, 10μV per LSB */
    return ((int16_t)raw) * 0.01f;
}

float ina219_read_current_ma(void)
{
    int16_t raw;
    if (read_reg16(REG_CURRENT, (uint16_t *)&raw) != ESP_OK) return -1.0f;
    return raw * (CURRENT_LSB_UA / 1000.0f);
}

float ina219_read_power_mw(void)
{
    uint16_t raw;
    if (read_reg16(REG_POWER, &raw) != ESP_OK) return -1.0f;
    /* Power LSB = 20 × Current LSB = 1000μW = 1mW */
    return raw * (CURRENT_LSB_UA * 20.0f / 1000.0f);
}
