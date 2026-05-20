#include "audio.h"
#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

static const char *TAG = "audio";

#define PWM_FREQ_HZ      100000
#define PWM_RESOLUTION   LEDC_TIMER_8_BIT
#define PWM_GPIO          GPIO_NUM_11

static volatile bool g_playing = false;

/* ─── Init ─── */

void audio_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {
        .gpio_num   = PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 128,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));

    ESP_LOGI(TAG, "PWM audio: GPIO%d %dHz 8bit", PWM_GPIO, PWM_FREQ_HZ);
}

/* ─── Playback ─── */

void audio_play_pcm(const int16_t *data, size_t num_samples)
{
    if (!data || num_samples == 0) return;
    g_playing = true;

    for (size_t i = 0; i < num_samples; i++) {
        uint32_t duty = (uint32_t)(((int32_t)data[i] + 32768) * 255 / 65535);
        if (duty > 255) duty = 255;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        esp_rom_delay_us(62);
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    g_playing = false;
}

void audio_play_stop(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    g_playing = false;
}

bool audio_is_playing(void) { return g_playing; }

/* ─── Stubs ─── */
bool audio_record_start(void) { return false; }
void audio_record_stop(void) {}
int  audio_record_read(int16_t *b, size_t m, int t) { (void)b;(void)m;(void)t; return 0; }
void audio_test_loopback(int ms) { (void)ms; }

/* ─── Tone test ─── */

static volatile bool g_test_busy = false;
bool audio_test_busy(void) { return g_test_busy; }

static void tone_task(void *arg)
{
    int freq = ((int *)arg)[0];
    int dur  = ((int *)arg)[1];
    free(arg);

    ESP_LOGI(TAG, "Tone: %dHz %dms", freq, dur);

    int total = 16000 * dur / 1000;
    int16_t *buf = malloc(total * 2);
    if (!buf) { g_test_busy = false; vTaskDelete(NULL); return; }

    float phi = 0, dphi = 2.0f * M_PI * freq / 16000.0f;
    for (int i = 0; i < total; i++) {
        buf[i] = (int16_t)(20000.0f * sinf(phi));
        phi += dphi;
        if (phi >= 2.0f * M_PI) phi -= 2.0f * M_PI;
    }

    audio_play_pcm(buf, total);
    free(buf);
    ESP_LOGI(TAG, "Tone done");
    g_test_busy = false;
    vTaskDelete(NULL);
}

void audio_test_tone(int freq_hz, int duration_ms)
{
    if (g_test_busy) return;
    g_test_busy = true;
    int *p = malloc(2 * sizeof(int));
    if (!p) { g_test_busy = false; return; }
    p[0] = freq_hz; p[1] = duration_ms;
    xTaskCreate(tone_task, "tone", 4096, p, 5, NULL);
}
