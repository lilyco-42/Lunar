#pragma once

#include <stdint.h>

/**
 * @brief Initialize BH1750 ambient light sensor.
 *
 * Powers on the sensor and sets continuous high-resolution mode.
 * I2C bus must already be initialized.
 *
 * @return 0 on success, non-zero on I2C error.
 */
int bh1750_init(void);

/**
 * @brief Read current light intensity.
 *
 * @return Light level in lux, or -1.0f on error.
 */
float bh1750_read_lux(void);
