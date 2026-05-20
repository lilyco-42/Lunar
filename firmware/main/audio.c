#include "audio.h"
#include "config.h"
#include "sensor_drivers/adc_sensors.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_adc/adc_continuous.h"
#include "hal/adc_types.h"
#include <string.h>
#include <math.h>

static const char *TAG = "audio";

/* ─── I2S playback state ─── */
static i2s_chan_handle_t g_tx_chan = NULL;
static volatile bool g_playing = false;

/* ─── ADC recording state ─── */
static adc_continuous_handle_t g_adc_handle = NULL;
static volatile bool g_recording = false;

/* ─── I2S init (playback) ─── */

void audio_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &g_tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_PIN,
            .ws   = I2S_LRCLK_PIN,
            .dout = I2S_DOUT_PIN,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(g_tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(g_tx_chan));

    ESP_LOGI(TAG, "I2S TX OK: %d Hz, BCK=%d LRCK=%d DIN=%d",
             AUDIO_SAMPLE_RATE, I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DOUT_PIN);
}

/* ─── Recording ─── */

bool audio_record_start(void)
{
    if (g_recording) return true;

    adc_sensors_deinit();

    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = 8192,
        .conv_frame_size    = 512,
    };
    esp_err_t ret = adc_continuous_new_handle(&handle_cfg, &g_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_new_handle failed: %s", esp_err_to_name(ret));
        adc_sensors_init();
        return false;
    }

    adc_digi_pattern_config_t pattern = {
        .atten    = ADC_ATTEN_DB_12,
        .channel  = ADC_CHANNEL_0,
        .unit     = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12,
    };
    adc_continuous_config_t dig_cfg = {
        .pattern_num    = 1,
        .adc_pattern    = &pattern,
        .sample_freq_hz = AUDIO_SAMPLE_RATE,
        .conv_mode      = ADC_CONV_SINGLE_UNIT_1,
        .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };
    ret = adc_continuous_config(g_adc_handle, &dig_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_config failed: %s", esp_err_to_name(ret));
        adc_continuous_deinit(g_adc_handle);
        g_adc_handle = NULL;
        adc_sensors_init();
        return false;
    }

    ret = adc_continuous_start(g_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_start failed: %s", esp_err_to_name(ret));
        adc_continuous_deinit(g_adc_handle);
        g_adc_handle = NULL;
        adc_sensors_init();
        return false;
    }

    g_recording = true;
    ESP_LOGI(TAG, "Recording started (ADC continuous, %d Hz)", AUDIO_SAMPLE_RATE);
    return true;
}

void audio_record_stop(void)
{
    if (!g_recording) return;

    adc_continuous_stop(g_adc_handle);
    adc_continuous_deinit(g_adc_handle);
    g_adc_handle = NULL;
    g_recording = false;

    adc_sensors_init();

    ESP_LOGI(TAG, "Recording stopped, ADC oneshot restored");
}

int audio_record_read(int16_t *buf, size_t max_samples, int timeout_ms)
{
    if (!g_recording || !g_adc_handle || !buf || max_samples == 0) return 0;

    uint8_t dma_buf[1024];
    uint32_t bytes_read = 0;
    esp_err_t ret = adc_continuous_read(g_adc_handle, dma_buf, sizeof(dma_buf),
                                        &bytes_read, timeout_ms);
    if (ret != ESP_OK) return 0;

    int count = 0;
    for (uint32_t i = 0; i + 3 < bytes_read && (size_t)count < max_samples; i += 4) {
        adc_digi_output_data_t *p = (adc_digi_output_data_t *)&dma_buf[i];
        if (p->type2.channel != ADC_CHANNEL_0) continue;
        uint32_t raw12 = p->type2.data;
        buf[count++] = (int16_t)((int32_t)raw12 - 2048) * 16;
    }
    return count;
}

/* ─── Playback ─── */

void audio_play_pcm(const int16_t *data, size_t num_samples)
{
    if (!g_tx_chan || !data || num_samples == 0) return;

    g_playing = true;

    /* Convert mono → interleaved stereo */
    int16_t stereo[512];
    size_t pos = 0;

    while (pos < num_samples) {
        size_t chunk = num_samples - pos;
        if (chunk > 256) chunk = 256;

        for (size_t i = 0; i < chunk; i++) {
            stereo[i * 2]     = data[pos + i];  /* L */
            stereo[i * 2 + 1] = data[pos + i];  /* R */
        }

        size_t bytes_written = 0;
        i2s_channel_write(g_tx_chan, stereo, chunk * 4, &bytes_written, portMAX_DELAY);
        pos += chunk;
    }

    g_playing = false;
}

void audio_play_stop(void)
{
    if (!g_tx_chan) return;
    i2s_channel_disable(g_tx_chan);
    i2s_channel_enable(g_tx_chan);
    g_playing = false;
    ESP_LOGI(TAG, "Playback stopped");
}

bool audio_is_playing(void)
{
    return g_playing;
}

/* ─── Audio tests ─── */

static volatile bool g_test_busy = false;

bool audio_test_busy(void)
{
    return g_test_busy;
}

typedef struct {
    int freq_hz;
    int duration_ms;
} tone_params_t;

static void tone_task(void *arg)
{
    tone_params_t p = *(tone_params_t *)arg;
    free(arg);

    ESP_LOGI(TAG, "Tone: %d Hz, %d ms", p.freq_hz, p.duration_ms);

    int total_samples = (AUDIO_SAMPLE_RATE * p.duration_ms) / 1000;
    int16_t *buf = malloc(total_samples * sizeof(int16_t));
    if (!buf) { g_test_busy = false; vTaskDelete(NULL); return; }

    float phase = 0.0f;
    float phase_inc = 2.0f * (float)M_PI * p.freq_hz / AUDIO_SAMPLE_RATE;

    for (int i = 0; i < total_samples; i++) {
        buf[i] = (int16_t)(16000.0f * sinf(phase));
        phase += phase_inc;
        if (phase >= 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
    }

    audio_play_pcm(buf, total_samples);
    free(buf);

    ESP_LOGI(TAG, "Tone done: %d Hz, %d samples", p.freq_hz, total_samples);
    g_test_busy = false;
    vTaskDelete(NULL);
}

void audio_test_tone(int freq_hz, int duration_ms)
{
    if (g_test_busy) return;
    g_test_busy = true;

    tone_params_t *p = malloc(sizeof(tone_params_t));
    if (!p) { g_test_busy = false; return; }
    p->freq_hz = freq_hz;
    p->duration_ms = duration_ms;

    xTaskCreate(tone_task, "tone_test", 4096, p, 5, NULL);
}

typedef struct {
    int record_ms;
} loopback_params_t;

static void loopback_task(void *arg)
{
    loopback_params_t lp = *(loopback_params_t *)arg;
    free(arg);

    int max_samples = (AUDIO_SAMPLE_RATE * lp.record_ms) / 1000;
    int16_t *rec_buf = malloc(max_samples * sizeof(int16_t));
    if (!rec_buf) {
        ESP_LOGE(TAG, "Loopback: malloc failed (%d bytes)", max_samples * 2);
        g_test_busy = false;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Loopback: recording %d ms (%d samples)...", lp.record_ms, max_samples);

    if (!audio_record_start()) {
        ESP_LOGE(TAG, "Loopback: record_start failed");
        free(rec_buf);
        g_test_busy = false;
        vTaskDelete(NULL);
        return;
    }

    int total = 0;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(lp.record_ms);
    while (total < max_samples && xTaskGetTickCount() < deadline) {
        int n = audio_record_read(rec_buf + total, max_samples - total, 100);
        total += n;
    }
    audio_record_stop();

    ESP_LOGI(TAG, "Loopback: captured %d samples, playing back...", total);
    if (total > 0) {
        audio_play_pcm(rec_buf, total);
    }

    free(rec_buf);
    ESP_LOGI(TAG, "Loopback test done");
    g_test_busy = false;
    vTaskDelete(NULL);
}

void audio_test_loopback(int record_ms)
{
    if (g_test_busy) return;
    if (record_ms > 3000) record_ms = 3000;
    g_test_busy = true;

    loopback_params_t *p = malloc(sizeof(loopback_params_t));
    if (!p) { g_test_busy = false; return; }
    p->record_ms = record_ms;

    xTaskCreate(loopback_task, "loopback", 4096, p, 5, NULL);
}
