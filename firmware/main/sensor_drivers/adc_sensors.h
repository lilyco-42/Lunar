#pragma once

#include <stdint.h>

/**
 * @brief Initialize ADC1 for MQ-7, MQ-135, and MAX9814.
 *
 * Configures ADC1 oneshot mode with 11dB attenuation (0~2.5V range).
 *
 * @return 0 on success.
 */
int adc_sensors_init(void);

/**
 * @brief Deinitialize ADC1 oneshot driver.
 *
 * Releases the ADC1 unit so that ADC continuous mode (audio recording) can
 * claim it.  Call adc_sensors_init() again after recording finishes.
 */
void adc_sensors_deinit(void);

/**
 * @brief Read raw ADC value from MQ-7 (CO sensor) on GPIO1 / ADC1_CH1.
 *        Range: 0–4095 (after 2:1 voltage divider: 0–5V input → 0–2.5V ADC)
 * @return 12-bit raw ADC value, or -1 on error.
 */
int adc_mq7_read_raw(void);

/**
 * @brief Read raw ADC value from MQ-135 (air quality) on GPIO3 / ADC1_CH3.
 *        Range: 0–4095 (after 2:1 voltage divider)
 * @return 12-bit raw ADC value, or -1 on error.
 */
int adc_mq135_read_raw(void);

/**
 * @brief Read raw ADC value from MAX9814 (microphone envelope) on GPIO0 / ADC1_CH0.
 *        Range: 0–4095
 * @return 12-bit raw ADC value, or -1 on error.
 */
int adc_max9814_read_raw(void);
