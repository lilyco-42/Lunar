#pragma once

#include <stdint.h>

/**
 * @brief Initialize OneWire bus and verify DS18B20 presence.
 *
 * @return 0 on success (device detected), non-zero on failure.
 */
int ds18b20_init(void);

/**
 * @brief Trigger a temperature conversion (non-blocking).
 *
 * Conversion takes ~750ms at 12-bit resolution.
 * Call ds18b20_read_temp() after the conversion delay to get the result.
 */
void ds18b20_start_conversion(void);

/**
 * @brief Read the most recent temperature result.
 *
 * Must be called >= 750ms after ds18b20_start_conversion().
 *
 * @return Temperature in degrees Celsius, or -999.0f on error.
 */
float ds18b20_read_temp(void);
