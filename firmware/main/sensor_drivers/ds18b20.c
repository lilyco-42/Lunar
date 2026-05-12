#include "ds18b20.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ds18b20";

/* ─── OneWire low-level (bit-bang on ONEWIRE_DS18B20_PIN) ─── */

/* Disable interrupts briefly for timing-critical sections */
#define NO_INTR_BEGIN()    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; \
                           portENTER_CRITICAL(&mux)
#define NO_INTR_END()      portEXIT_CRITICAL(&mux)

#define DELAY_US(us)       esp_rom_delay_us(us)

static gpio_num_t ow_pin;

/* Switch between output low and floating input */
static inline void ow_set_output(void)
{
    gpio_set_direction(ow_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(ow_pin, 1);  /* release (pullup holds high) */
}

static inline void ow_set_input(void)
{
    gpio_set_direction(ow_pin, GPIO_MODE_INPUT);
}

/* ─── Protocol primitives ─── */

static int ow_reset(void)
{
    NO_INTR_BEGIN();
    ow_set_output();
    gpio_set_level(ow_pin, 0);
    DELAY_US(480);
    ow_set_input();
    DELAY_US(70);
    int presence = (gpio_get_level(ow_pin) == 0);
    DELAY_US(410);
    ow_set_output();
    NO_INTR_END();
    return presence;  /* 1 = device present */
}

static void ow_write_bit(int bit)
{
    NO_INTR_BEGIN();
    ow_set_output();
    gpio_set_level(ow_pin, 0);
    if (bit) {
        DELAY_US(6);     /* write 1: short low pulse */
        gpio_set_level(ow_pin, 1);
        DELAY_US(64);
    } else {
        DELAY_US(60);    /* write 0: long low pulse */
        gpio_set_level(ow_pin, 1);
        DELAY_US(10);
    }
    NO_INTR_END();
}

static int ow_read_bit(void)
{
    int bit;
    NO_INTR_BEGIN();
    ow_set_output();
    gpio_set_level(ow_pin, 0);
    DELAY_US(3);
    ow_set_input();
    DELAY_US(10);
    bit = gpio_get_level(ow_pin);
    DELAY_US(53);
    NO_INTR_END();
    return bit;
}

static void ow_write_byte(uint8_t b)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(b & 0x01);
        b >>= 1;
    }
}

static uint8_t ow_read_byte(void)
{
    uint8_t b = 0;
    for (int i = 0; i < 8; i++) {
        b >>= 1;
        if (ow_read_bit()) b |= 0x80;
    }
    return b;
}

static int ow_crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0;
    while (len--) {
        uint8_t inbyte = *data++;
        for (int i = 0; i < 8; i++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

/* ─── Public API ─── */

int ds18b20_init(void)
{
    ow_pin = ONEWIRE_DS18B20_PIN;

    /* Configure GPIO */
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << ow_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(ow_pin, 1);

    /* Check presence */
    if (!ow_reset()) {
        ESP_LOGE(TAG, "No DS18B20 detected on GPIO %d", ow_pin);
        return -1;
    }

    ESP_LOGI(TAG, "DS18B20 detected on GPIO %d", ow_pin);
    return 0;
}

void ds18b20_start_conversion(void)
{
    if (!ow_reset()) return;
    ow_write_byte(0xCC);  /* Skip ROM */
    ow_write_byte(0x44);  /* Start conversion */
}

float ds18b20_read_temp(void)
{
    if (!ow_reset()) return -999.0f;

    ow_write_byte(0xCC);  /* Skip ROM */
    ow_write_byte(0xBE);  /* Read scratchpad */

    uint8_t buf[9];
    for (int i = 0; i < 9; i++) {
        buf[i] = ow_read_byte();
    }

    if (ow_crc8(buf, 9) != 0) {
        ESP_LOGW(TAG, "CRC mismatch");
        return -999.0f;
    }

    int16_t raw = ((int16_t)buf[1] << 8) | buf[0];
    return raw / 16.0f;
}
