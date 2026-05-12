#pragma once

#include <stdint.h>

/**
 * @brief Initialize INA219 current/voltage/power sensor.
 *
 * Configures 32V/2A range with 0.1Ω shunt and 12-bit ADC resolution.
 * I2C bus must already be initialized.
 *
 * @return 0 on success, non-zero on I2C error.
 */
int ina219_init(void);

/**
 * @brief Read bus voltage.
 * @return Voltage in volts, or -1.0f on error.
 */
float ina219_read_bus_voltage(void);

/**
 * @brief Read shunt voltage.
 * @return Voltage in millivolts, or -1.0f on error.
 */
float ina219_read_shunt_voltage_mv(void);

/**
 * @brief Read load current.
 * @return Current in milliamps, or -1.0f on error.
 */
float ina219_read_current_ma(void);

/**
 * @brief Read load power.
 * @return Power in milliwatts, or -1.0f on error.
 */
float ina219_read_power_mw(void);
