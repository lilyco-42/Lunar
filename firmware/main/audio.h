#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_BITS          16

/**
 * @brief Initialize I2S TX channel for playback (PCM5102 DAC → PAM8403).
 *
 * Call once during startup. The I2S channel stays initialized permanently.
 */
void audio_init(void);

/**
 * @brief Start continuous ADC capture on MAX9814 (GPIO0) at 16 kHz.
 *
 * WARNING: This deinitializes the ADC oneshot driver used by adc_sensors.
 * Sensor ADC reads (MQ-7, MQ-135, MAX9814) will fail until recording stops.
 *
 * @return true on success.
 */
bool audio_record_start(void);

/**
 * @brief Stop ADC capture and reinitialize the ADC oneshot driver for sensors.
 */
void audio_record_stop(void);

/**
 * @brief Read captured PCM samples (16-bit signed, centered around 0).
 *
 * @param buf         Output buffer for PCM samples.
 * @param max_samples Maximum number of samples to read.
 * @param timeout_ms  Timeout in milliseconds.
 * @return Number of samples actually read, or 0 on timeout/error.
 */
int audio_record_read(int16_t *buf, size_t max_samples, int timeout_ms);

/**
 * @brief Play mono PCM data through I2S (duplicated to stereo for PAM8403).
 *
 * Blocks until all samples are written.
 *
 * @param data        Mono 16-bit signed PCM samples.
 * @param num_samples Number of samples to play.
 */
void audio_play_pcm(const int16_t *data, size_t num_samples);

/**
 * @brief Stop playback and flush the I2S buffer.
 */
void audio_play_stop(void);

/**
 * @brief Check if audio is currently being played.
 */
bool audio_is_playing(void);

/**
 * @brief Check if an audio test is currently running.
 */
bool audio_test_busy(void);

/**
 * @brief Play a sine wave tone (non-blocking, runs in background task).
 *
 * @param freq_hz    Frequency in Hz (e.g. 440 for A4).
 * @param duration_ms Duration in milliseconds.
 */
void audio_test_tone(int freq_hz, int duration_ms);

/**
 * @brief Record from microphone then play back (non-blocking, background task).
 *
 * @param record_ms  How long to record in milliseconds (max ~3000 due to RAM).
 */
void audio_test_loopback(int record_ms);
