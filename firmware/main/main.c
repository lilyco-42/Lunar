#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "config.h"
#include "sensor_drivers/ssd1306.h"
#include "sensor_drivers/bh1750.h"
#include "sensor_drivers/ina219.h"
#include "sensor_drivers/adc_sensors.h"
#include "sensor_drivers/dht11.h"
#include "sensor_drivers/ds18b20.h"
#include "sensor_drivers/oled_emoji.h"
#include "audio.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "su03t.h"

static const char *TAG = "test";

static int g_dht_temp = 0, g_dht_hum = 0;

/* SU-03T command callback */
static void on_su03t_cmd(int msgid)
{
    ESP_LOGI(TAG, "SU-03T cmd=%d", msgid);
    /* msgid=1: wake word detected, speak sensors */
    su03t_speak_temp((float)g_dht_temp);
    vTaskDelay(pdMS_TO_TICKS(3000));
    su03t_speak_humi(g_dht_hum);
}

static void i2c_scan(void)
{
    ESP_LOGI(TAG, "=== I2C Bus Scan ===");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  Device found at 0x%02X", addr);
            found++;
        }
    }
    if (found == 0) {
        ESP_LOGE(TAG, "  NO I2C devices found - check wiring!");
    }
    ESP_LOGI(TAG, "=== Scan done: %d device(s) ===", found);
}

void app_main(void)
{
    /* ===== NVS ===== */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_LOGI(TAG, "NVS OK");

    /* ===== I2C Bus ===== */
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0));
    ESP_LOGI(TAG, "I2C bus ready: SDA=GPIO%d, SCL=GPIO%d, %d Hz",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);

    i2c_scan();

    /* ===== OLED ===== */
    oled_init();

    /* ===== BH1750 ===== */
    ESP_LOGI(TAG, "Initializing BH1750 at 0x%02X...", BH1750_I2C_ADDR);
    int bh_ok = (bh1750_init() == 0);
    if (bh_ok) ESP_LOGI(TAG, "BH1750 OK");
    else       ESP_LOGE(TAG, "BH1750 FAIL");

    /* ===== INA219 ===== */
    ESP_LOGI(TAG, "Initializing INA219 at 0x%02X...", INA219_I2C_ADDR);
    int ina_ok = (ina219_init() == 0);
    if (ina_ok) ESP_LOGI(TAG, "INA219 OK");
    else        ESP_LOGE(TAG, "INA219 FAIL");

    /* ===== ADC (MAX9814 mic) ===== */
    ESP_LOGI(TAG, "Initializing ADC (MAX9814 on GPIO0)...");
    int adc_ok = (adc_sensors_init() == 0);
    if (adc_ok) ESP_LOGI(TAG, "ADC OK");
    else        ESP_LOGE(TAG, "ADC FAIL");

    /* ===== DHT11 ===== */
    ESP_LOGI(TAG, "Initializing DHT11 on GPIO%d...", DHT11_DATA_PIN);
    dht11_init(DHT11_DATA_PIN);
    int dht_temp = 0, dht_hum = 0, dht_ok = 0;

    /* ===== DS18B20 ===== */
    ESP_LOGI(TAG, "Initializing DS18B20 on GPIO%d...", ONEWIRE_DS18B20_PIN);
    int ds_ok = (ds18b20_init() == 0);
    float ds_temp = 0;
    int ds_pending = 0, ds_age = 0;

    /* ===== Audio (PWM → PAM8403) ===== */
    ESP_LOGI(TAG, "Initializing audio (PWM)...");
    audio_init();
    ESP_LOGI(TAG, "PWM audio ready");

    /* ===== WiFi ===== */
    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_init();

    /* ===== Web Server ===== */
    ESP_LOGI(TAG, "Starting web server...");
    web_server_start();

    /* ===== SU-03T Voice Module ===== */
    ESP_LOGI(TAG, "Initializing SU-03T voice module...");
    su03t_init();
    su03t_start_monitor();
    ESP_LOGI(TAG, "SU-03T ready");

    /* Command callback: react to SU-03T commands */
    su03t_on_cmd(on_su03t_cmd);

    /* ===== Boot Splash ===== */
    oled_clear();
    oled_emoji_show(EMOJI_SMILE);
    oled_show_text(4, "^_^  Lunar v1.0");
    oled_show_text(5, "===================");
    oled_show_text(6, "github.com/lilyco-42");
    vTaskDelay(pdMS_TO_TICKS(1500));
    oled_emoji_clear();

    /* ===== Display Loop ===== */
    char line0[22], line1[22], line2[22], line3[22], line4[22], line5[22];
    char line6[22], line7[22];
    int tick = 0;
    while (1) {
        float lux = bh_ok ? bh1750_read_lux() : -1.0f;
        float bus_v = ina_ok ? ina219_read_bus_voltage() : -1.0f;
        float cur_ma = ina_ok ? ina219_read_current_ma() : -1.0f;
        float pwr_mw = ina_ok ? ina219_read_power_mw() : -1.0f;
        int mic  = adc_ok ? adc_max9814_read_raw() : -1;
        int co   = adc_ok ? adc_mq7_read_raw() : -1;
        int air  = adc_ok ? adc_mq135_read_raw() : -1;

        /* DHT11: read every 2s */
        if (tick % 10 == 0) {
            dht_ok = (dht11_read(&dht_temp, &dht_hum) == 0);
            g_dht_temp = dht_temp;
            g_dht_hum  = dht_hum;
        }

        /* DS18B20: start conversion every 2s, read ~800ms later */
        if (ds_ok && tick % 10 == 0) {
            ds18b20_start_conversion();
            ds_pending = 1;
            ds_age = 0;
        }
        if (ds_pending) {
            ds_age++;
            if (ds_age >= 4) {
                float t = ds18b20_read_temp();
                if (t > -900.0f) ds_temp = t;
                ds_pending = 0;
            }
        }

        /* Format display */
        snprintf(line0, sizeof(line0), "Lunar Dashboard");
        snprintf(line1, sizeof(line1), "Air  %dC  %d%%", dht_temp, dht_hum);
        if (ds_ok)
            snprintf(line2, sizeof(line2), "Water %.1fC", ds_temp);
        else
            snprintf(line2, sizeof(line2), "Water --");
        snprintf(line3, sizeof(line3), "Light %.0f lux  Mic %d", lux, mic);
        snprintf(line4, sizeof(line4), "%.2fV %.0fmA %.0fmW", bus_v, cur_ma, pwr_mw);
        snprintf(line5, sizeof(line5), "CO %d  Air %d", co, air);
        snprintf(line6, sizeof(line6), "WiFi: %s", wifi_get_ip_str());
        snprintf(line7, sizeof(line7), "8 sensors  PWM audio");

        if (tick % 15 == 0) audio_test_tone(800, 200);

        oled_clear();
        oled_show_text(0, line0);
        oled_show_text(1, line1);
        oled_show_text(2, line2);
        oled_show_text(3, line3);
        oled_show_text(4, line4);
        oled_show_text(5, line5);
        oled_show_text(6, line6);
        oled_show_text(7, line7);

        ESP_LOGI(TAG, "%s | %s | CO:%d Air:%d", line1, line2, co, air);

        tick++;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
