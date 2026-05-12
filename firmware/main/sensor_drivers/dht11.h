#pragma once

#include <stdint.h>

/**
 * @brief Initialize DHT11 temperature/humidity sensor.
 *
 * GPIO is configured for open-drain output + input with internal pull-up.
 *
 * @param gpio  GPIO pin connected to DHT11 DATA line.
 */
void dht11_init(int gpio);

/**
 * @brief Read temperature and humidity from DHT11.
 *
 * @param temperature  Output: temperature in Celsius (integer).
 * @param humidity     Output: relative humidity in % (integer).
 * @return 0 on success, non-zero on checksum error or timeout.
 */
int dht11_read(int *temperature, int *humidity);
