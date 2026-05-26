#include "esp_log.h"
#include "driver/uart.h"
#include "hal/gpio_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "audio.h"
#include <string.h>

static const char *TAG = "su03t";

#define SU03T_UART UART_NUM_1
#define SU03T_TX   GPIO_NUM_18
#define SU03T_RX   GPIO_NUM_19
#define SU03T_BAUD 9600

void su03t_init(void)
{
    uart_config_t cfg = {
        .baud_rate  = SU03T_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(SU03T_UART, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(SU03T_UART, SU03T_TX, SU03T_RX,
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(SU03T_UART, 512, 256, 0, NULL, 0));
    ESP_LOGI(TAG, "UART ready: TX=%d RX=%d", SU03T_TX, SU03T_RX);
}

void su03t_send(const uint8_t *data, size_t len)
{
    uart_write_bytes(SU03T_UART, data, len);
}

typedef struct { uint8_t msgid; uint8_t params[32]; int param_bytes; } su03t_msg_t;

static int parse_msg(const uint8_t *buf, int len, su03t_msg_t *out)
{
    const uint8_t *hdr = NULL;
    for (int i = 0; i < len - 1; i++) {
        if (buf[i] == 0xAA && buf[i+1] == 0x55) { hdr = buf + i; break; }
    }
    if (!hdr) return -1;
    int rem = (int)(buf + len - hdr);
    if (rem < 5) return -1;
    out->msgid = hdr[2];
    out->param_bytes = 0;
    for (int i = 3; i < rem - 2 && out->param_bytes < (int)sizeof(out->params); i++) {
        if (hdr[i] == 0x55 && hdr[i+1] == 0xAA) break;
        out->params[out->param_bytes++] = hdr[i];
    }
    return 0;
}

/* Callbacks set by main */
static void (*g_cmd_cb)(int msgid) = NULL;

void su03t_on_cmd(void (*cb)(int msgid))
{
    g_cmd_cb = cb;
}

/* jx_firm UART command helpers */
static void jx_send(uint8_t msgid, const uint8_t *param, int plen)
{
    uint8_t buf[32];
    buf[0] = 0xAA; buf[1] = 0x55;
    buf[2] = msgid;
    int total = 3;
    for (int i = 0; i < plen; i++) buf[total++] = param[i];
    buf[total++] = 0x55; buf[total++] = 0xAA;
    su03t_send(buf, total);
}

void su03t_speak_temp(float temp)
{
    int ti = (int)temp;
    int td = (int)((temp - ti) * 10 + 0.5f);
    if (td < 0) td = 0;
    if (td > 9) td = 9;
    uint8_t p[] = {(uint8_t)ti, (uint8_t)td};
    jx_send(1, p, 2);  /* msgid=1 speak_temp: "当前温度为：XX点X度" */
    ESP_LOGI(TAG, "Speak temp: %d.%d", ti, td);
}

void su03t_speak_humi(int humi)
{
    uint8_t p[] = {(uint8_t)humi};
    jx_send(2, p, 1);  /* msgid=2 speak_humi: "当前湿度为百分之：XX" */
    ESP_LOGI(TAG, "Speak humi: %d", humi);
}

static void su03t_monitor(void *arg)
{
    uint8_t buf[128];
    while (1) {
        /* Check GPIO pulses from SU-03T command output */
        /* Check UART from SU-03T */
        int n = uart_read_bytes(SU03T_UART, buf, sizeof(buf)-1, pdMS_TO_TICKS(50));
        if (n > 0) {
            ESP_LOGI(TAG, "RX %d bytes: %02X", n, buf[0]);
            if (g_cmd_cb) g_cmd_cb(buf[0]);
        }
    }
}

void su03t_start_monitor(void)
{
    xTaskCreate(su03t_monitor, "su03t_mon", 2560, NULL, 2, NULL);
}
