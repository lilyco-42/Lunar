#pragma once

#include <stdint.h>

/**
 * @brief Snapshot of all sensor readings at a point in time.
 */
typedef struct {
    float temperature;        /* °C (DS18B20) */
    float light_lux;          /* Lux (BH1750) */
    float bus_voltage;        /* V (INA219) */
    float current_ma;         /* mA (INA219) */
    float power_mw;           /* mW (INA219) */
    int   co_raw;             /* Raw ADC / mV (MQ-7) */
    int   air_raw;            /* Raw ADC / mV (MQ-135) */
    int   mic_level;          /* Raw ADC / mV (MAX9814) */
    int   uptime_sec;         /* Seconds since boot */
} sensor_data_t;

/**
 * @brief Start the sensor polling FreeRTOS task.
 *
 * Must be called after all individual sensor drivers are initialized.
 * The task polls at ~1Hz and caches the latest readings.
 */
void sensor_task_start(void);

/**
 * @brief Get the latest sensor snapshot (thread-safe copy).
 *
 * @param out  Pointer to caller-allocated struct to fill.
 */
void sensor_task_get(sensor_data_t *out);
