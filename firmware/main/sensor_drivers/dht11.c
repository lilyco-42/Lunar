#include "dht11.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "dht11";
static int dht_gpio = -1;

void dht11_init(int gpio)
{
    dht_gpio = gpio;
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(gpio, 1);
    ESP_LOGI(TAG, "Initialized on GPIO%d", gpio);
}

/*
 * Poll while pin stays at given level, with 1us steps.
 * Returns 0 when level changes, -1 on timeout.
 */
static int wait_level_change(int level, int timeout_us)
{
    for (int i = 0; i < timeout_us; i++) {
        if (gpio_get_level(dht_gpio) != level) return 0;
        esp_rom_delay_us(1);
    }
    return -1;
}

int dht11_read(int *temperature, int *humidity)
{
    uint8_t data[5] = {0};
    int t, h;

    portDISABLE_INTERRUPTS();

    /* --- Start: drive low 18ms, release 30us --- */
    gpio_set_direction(dht_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_gpio, 0);
    esp_rom_delay_us(18000);
    gpio_set_level(dht_gpio, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(dht_gpio, GPIO_MODE_INPUT);

    /* Wait for DHT11 to pull low (respond within ~80us) */
    if (wait_level_change(1, 100) < 0) goto timeout;

    /* DHT11 holds low ~80us then releases ~80us then pulls low for data start */
    if (wait_level_change(0, 100) < 0) goto timeout;
    if (wait_level_change(1, 100) < 0) goto timeout;

    /* Read 40 bits */
    for (int i = 0; i < 40; i++) {
        /* Wait for low→high (bit value start, ~50us low from DHT11) */
        if (wait_level_change(0, 80) < 0) goto timeout;
        /* Sample at ~35us: if still high → 1, if already low → 0 */
        esp_rom_delay_us(35);
        data[i / 8] <<= 1;
        if (gpio_get_level(dht_gpio)) {
            data[i / 8] |= 1;
            /* Consume remaining high → wait for low (next bit start) */
            if (wait_level_change(1, 80) < 0) goto timeout;
        }
        /* If bit was 0, pin is already low — next iteration waits for low→high */
    }

    portENABLE_INTERRUPTS();

    /* Checksum */
    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) {
        ESP_LOGW(TAG, "Checksum err: %02x+%02x+%02x+%02x != %02x",
                 data[0], data[1], data[2], data[3], data[4]);
        return -2;
    }

    h = data[0];
    t = data[2];
    ESP_LOGI(TAG, "OK T=%dC H=%d%%", t, h);
    *temperature = t;
    *humidity    = h;
    return 0;

timeout:
    portENABLE_INTERRUPTS();
    ESP_LOGW(TAG, "Timeout at level=%d", gpio_get_level(dht_gpio));
    return -1;
}
