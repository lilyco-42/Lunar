#include "sensor_task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

#include "sensor_drivers/bh1750.h"
#include "sensor_drivers/ina219.h"
#include "sensor_drivers/ds18b20.h"
#include "sensor_drivers/adc_sensors.h"

static const char *TAG = "sensor";

static sensor_data_t g_data;
static SemaphoreHandle_t g_mutex;
static int64_t g_boot_time;

static void sensor_poll_task(void *arg)
{
    bool ds_pending = false;  /* true when a conversion was started */

    while (1) {
        /* Phase 1: read all "fast" sensors */
        sensor_data_t snap;
        memset(&snap, 0, sizeof(snap));

        snap.light_lux    = bh1750_read_lux();
        snap.bus_voltage  = ina219_read_bus_voltage();
        snap.current_ma   = ina219_read_current_ma();
        snap.power_mw     = ina219_read_power_mw();
        snap.co_raw       = adc_mq7_read_raw();
        snap.air_raw      = adc_mq135_read_raw();
        snap.mic_level    = adc_max9814_read_raw();
        snap.uptime_sec   = (int)((esp_timer_get_time() - g_boot_time) / 1000000);

        /* Phase 2: DS18B20 — read last conversion result, start new one */
        if (ds_pending) {
            snap.temperature = ds18b20_read_temp();
        }
        ds18b20_start_conversion();
        ds_pending = true;

        /* Publish */
        if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            memcpy(&g_data, &snap, sizeof(g_data));
            xSemaphoreGive(g_mutex);
        }

        ESP_LOGI(TAG, "T=%.1fC Lux=%.0f V=%.2fV I=%.0fmA P=%.0fmW CO=%d Air=%d Mic=%d",
                 snap.temperature, snap.light_lux,
                 snap.bus_voltage, snap.current_ma, snap.power_mw,
                 snap.co_raw, snap.air_raw, snap.mic_level);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void sensor_task_start(void)
{
    g_mutex = xSemaphoreCreateMutex();
    memset(&g_data, 0, sizeof(g_data));
    g_boot_time = esp_timer_get_time();

    xTaskCreate(sensor_poll_task, "sensor_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Sensor task started (1 Hz)");
}

void sensor_task_get(sensor_data_t *out)
{
    if (!out || !g_mutex) return;
    if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(out, &g_data, sizeof(g_data));
        xSemaphoreGive(g_mutex);
    }
}
