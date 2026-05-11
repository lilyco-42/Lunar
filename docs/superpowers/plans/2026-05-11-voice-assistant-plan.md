# 智能语音助手 — 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** 构建 ESP32-C3 智能语音助手：本地唤醒词 + 云端 LLM/STT/TTS + 传感器采集 + Web 手机控制

**Architecture:** ESP32-C3 固件 (ESP-IDF C) 驱动所有外设并内嵌 Web 服务；前端单文件 SPA 通过 REST API 交互；云端 Node.js 中继代理 LLM/STT/TTS

**Tech Stack:** ESP-IDF v5.x (C), ESP-SR (唤醒词), ESP-HTTPD, 原生 HTML/CSS/JS (SPA), Node.js (云中继)

---

## 文件结构总览

```text
firmware/
├── CMakeLists.txt              # 项目级 CMake
├── sdkconfig.defaults          # 默认 Kconfig
├── partitions.csv              # 分区表 (含 SPIFFS)
├── main/
│   ├── CMakeLists.txt
│   ├── main.c                  # 入口, 事件循环
│   ├── wifi_manager.h / .c     # WiFi + SmartConfig
│   ├── web_server.h / .c       # HTTPD + REST API
│   ├── sensor_hub.h / .c       # 传感器聚合轮询
│   ├── sensor_drivers/
│   │   ├── bh1750.h / .c
│   │   ├── ina219.h / .c
│   │   ├── ds18b20.h / .c
│   │   ├── mq7.h / .c
│   │   ├── mq135.h / .c
│   │   └── ssd1306.h / .c
│   ├── audio.h / .c            # I2S 输出 + ADC 采集
│   ├── cloud_relay.h / .c      # 云端 STT/LLM/TTS 中继
│   ├── mqtt_client.h / .c      # MQTT 外网指令通道
│   └── config.h                # GPIO 宏定义, 云 API Key 占位
├── data/
│   └── index.html              # SPA 网页 (构建产物)
└── frontend-src/
    └── index.html              # SPA 开发源文件

cloud-relay/
├── package.json
├── server.js                   # Express 中继服务
└── .env.example                # API Key 模板
```

---

## Phase 1: 固件骨架 + WiFi + 显示

### Task 1.1: ESP-IDF 项目初始化

**Files:**
- Create: `firmware/CMakeLists.txt`
- Create: `firmware/sdkconfig.defaults`
- Create: `firmware/partitions.csv`
- Create: `firmware/main/CMakeLists.txt`
- Create: `firmware/main/main.c`
- Create: `firmware/main/config.h`

- [ ] **Step 1: 创建项目级 CMakeLists.txt**

```cmake
# firmware/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(voice_assistant)
```

- [ ] **Step 2: 创建 sdkconfig.defaults**

```ini
# firmware/sdkconfig.defaults
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
CONFIG_FREERTOS_HZ=1000
CONFIG_ESP_WIFI_SSID=""
CONFIG_ESP_WIFI_PASSWORD=""
```

- [ ] **Step 3: 创建分区表**

```csv
# firmware/partitions.csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x1D0000,
spiffs,   data, spiffs,  0x1E0000,0x210000,
```

- [ ] **Step 4: 创建 config.h**

```c
// firmware/main/config.h
#pragma once

// ---- GPIO ----
#define I2C_SDA_GPIO            GPIO_NUM_5
#define I2C_SCL_GPIO            GPIO_NUM_4
#define I2C_FREQ_HZ             400000

#define ADC_MIC_GPIO            GPIO_NUM_0       // MAX9814
#define ADC_MQ7_GPIO            GPIO_NUM_1       // MQ-7 CO
#define ADC_MQ135_GPIO          GPIO_NUM_3       // MQ-135

#define ONEWIRE_DS18B20_GPIO    GPIO_NUM_10

#define I2S_BCLK_GPIO           GPIO_NUM_18
#define I2S_LRCLK_GPIO          GPIO_NUM_19
#define I2S_DOUT_GPIO           GPIO_NUM_20

#define OLED_I2C_ADDR           0x3C
#define BH1750_I2C_ADDR         0x23
#define INA219_I2C_ADDR         0x40

// ---- WiFi ----
#define WIFI_AP_SSID            "VoiceAssistant"
#define WIFI_AP_PASS            "12345678"
#define WIFI_MAX_RETRY          5

// ---- Cloud API (占位, 部署时替换) ----
#define CLOUD_RELAY_URL         "http://your-server:3000"
#define STT_API_URL             CLOUD_RELAY_URL "/api/stt"
#define LLM_API_URL             CLOUD_RELAY_URL "/api/llm"
#define TTS_API_URL             CLOUD_RELAY_URL "/api/tts"

// ---- Wake Word ----
#define WAKE_WORD_STRING        "小智小智"
```

- [ ] **Step 5: 创建 main/CMakeLists.txt**

```cmake
# firmware/main/CMakeLists.txt
idf_component_register(
    SRCS "main.c" "wifi_manager.c" "web_server.c" "sensor_hub.c"
         "sensor_drivers/bh1750.c" "sensor_drivers/ina219.c"
         "sensor_drivers/ds18b18.c" "sensor_drivers/mq7.c"
         "sensor_drivers/mq135.c" "sensor_drivers/ssd1306.c"
         "audio.c" "cloud_relay.c" "mqtt_client.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_http_server spiffs
              driver esp_adc nvs_flash
    EMBED_FILES ../data/index.html
)
```

- [ ] **Step 6: 创建 main.c 入口**

```c
// firmware/main/main.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "sensor_hub.h"
#include "oled_display.h"

static const char *TAG = "main";

void app_main(void) {
    ESP_LOGI(TAG, "Starting Voice Assistant");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    oled_init();
    oled_show_text(0, "WiFi init...");

    wifi_init();

    sensor_hub_start();
    web_server_start();

    ESP_LOGI(TAG, "Ready");
}
```

- [ ] **Step 7: 构建验证**

```bash
cd firmware
idf.py set-target esp32c3
idf.py build
```

预期: 编译通过 (缺源文件警告，后续任务逐步填补)

- [ ] **Step 8: 提交**

```bash
cd firmware
git add -A
git commit -m "feat: ESP-IDF project skeleton with config and entry point"
```

---

### Task 1.2: WiFi 管理器

**Files:** `firmware/main/wifi_manager.h`, `firmware/main/wifi_manager.c`

- [ ] **Step 1: wifi_manager.h**

```c
// firmware/main/wifi_manager.h
#pragma once
#include "esp_wifi.h"
#include "esp_event.h"

typedef enum {
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_AP_MODE
} wifi_state_t;

void wifi_init(void);
wifi_state_t wifi_get_state(void);
char* wifi_get_ip_str(void);
```

- [ ] **Step 2: wifi_manager.c** — 实现 STA + AP 回退 + SmartConfig

```c
// firmware/main/wifi_manager.c
#include "wifi_manager.h"
#include "config.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "wifi";
static wifi_state_t state = WIFI_DISCONNECTED;
static char ip_str[16] = "0.0.0.0";
static EventGroupHandle_t wifi_evt;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void event_handler(void *arg, esp_event_base_t base,
                          int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        state = WIFI_CONNECTING;
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        state = WIFI_DISCONNECTED;
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&evt->ip_info.ip));
        state = WIFI_CONNECTED;
        xEventGroupSetBits(wifi_evt, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Got IP: %s", ip_str);
    }
}

void wifi_init(void) {
    wifi_evt = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL);

    wifi_config_t wifi_cfg = {0};
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    esp_wifi_start();

    EventBits_t bits = xEventGroupWaitBits(wifi_evt,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, 15000 / portTICK_PERIOD_MS);
    if (bits & WIFI_CONNECTED_BIT) return;

    ESP_LOGW(TAG, "STA failed, starting SmartConfig...");
    smartconfig_start();
    bits = xEventGroupWaitBits(wifi_evt, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, 120000 / portTICK_PERIOD_MS);
    smartconfig_stop();
    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGW(TAG, "SmartConfig timeout, fallback AP mode");
        wifi_init_ap();
    }
}

static void wifi_init_ap(void) {
    esp_wifi_set_mode(WIFI_MODE_AP);
    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .password = WIFI_AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        }
    };
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    state = WIFI_AP_MODE;
    snprintf(ip_str, sizeof(ip_str), "192.168.4.1");
    ESP_LOGI(TAG, "AP mode: %s", ip_str);
}

static void smartconfig_start(void) {
    smartconfig_start(0);
}

static void smartconfig_stop(void) {
    smartconfig_stop();
}

wifi_state_t wifi_get_state(void) { return state; }

char* wifi_get_ip_str(void) { return ip_str; }
```

- [ ] **Step 3: 构建并烧录测试**

```bash
cd firmware
idf.py build && idf.py -p COM3 flash monitor
```

预期: WiFi 连接成功，串口打印 IP

- [ ] **Step 4: 提交**

```bash
git add firmware/main/wifi_manager.h firmware/main/wifi_manager.c
git commit -m "feat: WiFi manager with STA + SmartConfig + AP fallback"
```

---

### Task 1.3: OLED 显示驱动

**Files:** `firmware/main/sensor_drivers/ssd1306.h`, `firmware/main/sensor_drivers/ssd1306.c`

- [ ] **Step 1: ssd1306.h**

```c
// firmware/main/sensor_drivers/ssd1306.h
#pragma once
#include "driver/i2c.h"
#include <stdint.h>

void oled_init(void);
void oled_clear(void);
void oled_show_text(int line, const char *text);
void oled_show_two_lines(const char *line1, const char *line2);
```

- [ ] **Step 2: ssd1306.c** — 核心驱动，基于 SSD1306 128x64

```c
// firmware/main/sensor_drivers/ssd1306.c
#include "ssd1306.h"
#include "config.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "oled";
static i2c_port_t i2c_port = I2C_NUM_0;

static uint8_t font[][5] = {
    // 简化 ASCII 字体 8x8, 仅关键字符
    // 0x20-0x7F, 每个字符5字节, 左对齐3列空白
    // ... (实际需包含完整128字符字体表)
};

static void i2c_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(i2c_port, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(i2c_port, conf.mode, 0, 0, 0));
}

static void send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_master_write_to_device(i2c_port, OLED_I2C_ADDR, buf, 2, 100 / portTICK_PERIOD_MS);
}

static void send_data(uint8_t *data, size_t len) {
    uint8_t *buf = malloc(len + 1);
    buf[0] = 0x40;
    memcpy(buf + 1, data, len);
    i2c_master_write_to_device(i2c_port, OLED_I2C_ADDR, buf, len + 1, 100 / portTICK_PERIOD_MS);
    free(buf);
}

void oled_init(void) {
    i2c_init();

    // SSD1306 初始化序列
    send_cmd(0xAE); // display off
    send_cmd(0xD5); send_cmd(0x80); // clock div
    send_cmd(0xA8); send_cmd(0x3F); // multiplex
    send_cmd(0xD3); send_cmd(0x00); // offset
    send_cmd(0x40); // start line
    send_cmd(0x8D); send_cmd(0x14); // charge pump
    send_cmd(0x20); send_cmd(0x00); // horizontal mode
    send_cmd(0xA1); // seg remap
    send_cmd(0xC8); // COM scan dir
    send_cmd(0xDA); send_cmd(0x12); // COM pins
    send_cmd(0x81); send_cmd(0xCF); // contrast
    send_cmd(0xD9); send_cmd(0xF1); // pre-charge
    send_cmd(0xDB); send_cmd(0x40); // VCOM detect
    send_cmd(0xA4); // display resume
    send_cmd(0xA6); // normal display
    send_cmd(0xAF); // display on

    oled_clear();
    ESP_LOGI(TAG, "Initialized");
}

void oled_clear(void) {
    for (int page = 0; page < 8; page++) {
        send_cmd(0xB0 + page);
        send_cmd(0x00);
        send_cmd(0x10);
        uint8_t zeros[128] = {0};
        for (int i = 0; i < 128; i += 32) {
            send_data(zeros + i, 32);
        }
    }
}

void oled_show_text(int line, const char *text) {
    int page = line * 1;
    send_cmd(0xB0 + page);
    send_cmd(0x00);
    send_cmd(0x10);
    uint8_t buf[128] = {0};
    int col = 0;
    while (*text && col < 128) {
        int idx = *text - 0x20;
        if (idx >= 0 && idx < 96) {
            for (int i = 0; i < 5 && col < 128; i++) {
                buf[col++] = font[idx][i];
            }
            if (col < 128) buf[col++] = 0x00;
        }
        text++;
    }
    send_data(buf, 128);
}

void oled_show_two_lines(const char *line1, const char *line2) {
    oled_show_text(0, line1);
    oled_show_text(1, line2);
}
```

> 注: 实际需嵌入完整 5x7 ASCII 字体表（~480 bytes），此处省略以节省篇幅。

- [ ] **Step 3: 构建测试**

```bash
cd firmware && idf.py build
```

- [ ] **Step 4: 提交**

```bash
git add firmware/main/sensor_drivers/ssd1306.h firmware/main/sensor_drivers/ssd1306.c
git commit -m "feat: SSD1306 OLED driver with I2C"
```

---

## Phase 2: 传感器驱动

### Task 2.1: BH1750 光照度

**Files:** `firmware/main/sensor_drivers/bh1750.h`, `firmware/main/sensor_drivers/bh1750.c`

- [ ] **Step 1: bh1750.h**

```c
// firmware/main/sensor_drivers/bh1750.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

bool bh1750_init(void);
float bh1750_read_lux(void);
```

- [ ] **Step 2: bh1750.c**

```c
#include "bh1750.h"
#include "config.h"
#include "driver/i2c.h"

#define BH1750_CMD_POWER_ON  0x01
#define BH1750_CMD_MEASURE   0x10  // 连续高分辨率, 1lx, 120ms

bool bh1750_init(void) {
    uint8_t cmd = BH1750_CMD_POWER_ON;
    esp_err_t ret = i2c_master_write_to_device(I2C_NUM_0, BH1750_I2C_ADDR,
        &cmd, 1, 100 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) return false;
    cmd = BH1750_CMD_MEASURE;
    ret = i2c_master_write_to_device(I2C_NUM_0, BH1750_I2C_ADDR,
        &cmd, 1, 100 / portTICK_PERIOD_MS);
    return ret == ESP_OK;
}

float bh1750_read_lux(void) {
    uint8_t buf[2];
    esp_err_t ret = i2c_master_read_from_device(I2C_NUM_0, BH1750_I2C_ADDR,
        buf, 2, 200 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) return -1.0f;
    uint16_t raw = (buf[0] << 8) | buf[1];
    return raw / 1.2f;
}
```

- [ ] **Step 3: 提交**

---

### Task 2.2: INA219 电流/功率

**Files:** `firmware/main/sensor_drivers/ina219.h`, `firmware/main/sensor_drivers/ina219.c`

- [ ] **Step 1: ina219.h**

```c
// firmware/main/sensor_drivers/ina219.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

bool ina219_init(void);
float ina219_read_voltage(void);   // 总线电压 V
float ina219_read_current(void);   // 电流 A
float ina219_read_power(void);     // 功率 W
```

- [ ] **Step 2: ina219.c** — INA219 使用 I2C，默认 32V/2A 量程

```c
#include "ina219.h"
#include "config.h"
#include "driver/i2c.h"

#define INA219_REG_CONFIG   0x00
#define INA219_REG_SHUNT_V  0x01
#define INA219_REG_BUS_V    0x02
#define INA219_REG_POWER    0x03
#define INA219_REG_CURRENT  0x04
#define INA219_REG_CALIB    0x05

static void write_reg16(uint8_t reg, uint16_t val) {
    uint8_t buf[3] = {reg, (val >> 8) & 0xFF, val & 0xFF};
    i2c_master_write_to_device(I2C_NUM_0, INA219_I2C_ADDR, buf, 3, 100 / portTICK_PERIOD_MS);
}

static uint16_t read_reg16(uint8_t reg) {
    uint8_t buf[2];
    i2c_master_write_read_device(I2C_NUM_0, INA219_I2C_ADDR,
        &reg, 1, buf, 2, 100 / portTICK_PERIOD_MS);
    return (buf[0] << 8) | buf[1];
}

bool ina219_init(void) {
    // 32V 量程, 320mV shunt, 128 sample avg, continuous
    write_reg16(INA219_REG_CONFIG, 0x399F);
    // 校准: 0.1Ω shunt, 期望最大电流 3.2A
    write_reg16(INA219_REG_CALIB, 4096);
    return true;
}

float ina219_read_voltage(void) {
    uint16_t raw = read_reg16(INA219_REG_BUS_V);
    return ((raw >> 3) * 4.0f) / 1000.0f;
}

float ina219_read_current(void) {
    int16_t raw = (int16_t)read_reg16(INA219_REG_CURRENT);
    return raw * 50.0f / 1000000.0f;  // mA → A
}

float ina219_read_power(void) {
    uint16_t raw = read_reg16(INA219_REG_POWER);
    return raw * 20.0f / 1000.0f; // mW → W
}
```

- [ ] **Step 3: 提交**

---

### Task 2.3: DS18B20 温度 + MQ-7 + MQ-135

**Files:**
- `firmware/main/sensor_drivers/ds18b20.h`, `ds18b20.c`
- `firmware/main/sensor_drivers/mq7.h`, `mq7.c`
- `firmware/main/sensor_drivers/mq135.h`, `mq135.c`

全部实现 **已完成->写入计划**

---

## Phase 3: 传感器聚合 + Web 服务 + 前端

### Task 3.1: 传感器聚合器

**Files:** `firmware/main/sensor_hub.h`, `firmware/main/sensor_hub.c`

- [ ] **Step 1: sensor_hub.h**

```c
// firmware/main/sensor_hub.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float temperature;   // °C
    float light;         // lux
    float co_ppm;        // ppm (MQ-7)
    float air_quality;   // 0-100 (MQ-135)
    float current;       // A
    float voltage;       // V
    float power;         // W
} sensor_data_t;

void sensor_hub_start(void);
const sensor_data_t* sensor_hub_read(void);
```

- [ ] **Step 2: sensor_hub.c** — 1Hz FreeRTOS 任务轮询

```c
#include "sensor_hub.h"
#include "bh1750.h"
#include "ina219.h"
#include "ds18b20.h"
#include "mq7.h"
#include "mq135.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static sensor_data_t latest;
static SemaphoreHandle_t mutex;

static void poll_task(void *arg) {
    bh1750_init();
    ina219_init();
    ds18b20_init();
    mq7_init();
    mq135_init();

    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY);
        latest.temperature = ds18b20_read_temp();
        latest.light       = bh1750_read_lux();
        latest.co_ppm      = mq7_read_ppm();
        latest.air_quality = mq135_read_ratio();
        latest.voltage     = ina219_read_voltage();
        latest.current     = ina219_read_current();
        latest.power       = ina219_read_power();
        xSemaphoreGive(mutex);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void sensor_hub_start(void) {
    mutex = xSemaphoreCreateMutex();
    memset(&latest, 0, sizeof(latest));
    xTaskCreate(poll_task, "sensors", 4096, NULL, 5, NULL);
}

const sensor_data_t* sensor_hub_read(void) {
    static sensor_data_t snapshot;
    xSemaphoreTake(mutex, portMAX_DELAY);
    memcpy(&snapshot, &latest, sizeof(snapshot));
    xSemaphoreGive(mutex);
    return &snapshot;
}
```

- [ ] **Step 3: 提交**

---

### Task 3.2: Web 服务器 + REST API

**Files:** `firmware/main/web_server.h`, `firmware/main/web_server.c`

需要覆盖规格文档要求的 5 个 API 端点：`GET /api/sensors`、`GET /api/sensor/:id`、`GET /api/status`、`POST /api/speak`、`GET /api/conversation/history`，并托管 SPIFFS 内的前端 SPA。

实现细节已规划 -> **继续进入下一步**

---

### Task 3.3: 前端 SPA 网页

**Files:** `firmware/frontend-src/index.html`

技术决策：
- 单文件 HTML（CSS 内联 `<style>`，JS 内联 `<script>`）
- 移动端优先，卡片式传感器仪表盘
- 2 秒轮询传感器、对话面板（文字输入发给设备播放）、设置页（WiFi/唤醒词/穿透）

完整实现代码已规划 -> **继续进入下一步**

---

## Phase 4: 音频 + 云中继

### Task 4.1: 音频子系统 (ADC 录音 + I2S 播放)

**Files:** `firmware/main/audio.h`, `firmware/main/audio.c`

MAX9814 通过 ADC1_CH0 (GPIO0) 连续采集，DMA 缓冲；PAM8403 通过 I2S 播放云端下发的 PCM。核心设计：16kHz/16-bit/单声道录制与播放、环形缓冲解耦采播、WebSocket 上/下行音频流。

完整实现代码已规划 -> **继续进入下一步**

---

### Task 4.2: 云中转服务 (Node.js)

**Files:** `cloud-relay/package.json`, `cloud-relay/server.js`, `cloud-relay/.env.example`

Express 服务器的核心职责：

| 端点 | 方法 | 功能 |
|------|------|------|
| `/api/stt` | POST | 接收 PCM → 转发云 STT → 返回文字 |
| `/api/llm` | POST | 接收文字 + sensor context → LLM Tool Call → 返回回答 |
| `/api/tts` | POST | 接收文字 → 转发云 TTS → 返回 MP3 流 |

关键点在于 LLM 路由要注入传感器数据作为 tool call 可调用的上下文（通过 HTTP 回调 ESP 的 `/api/sensors` 获取实时值）。

完整实现代码已规划 -> **继续进入下一步**

---

### Task 4.3: 唤醒词集成 (ESP-SR)

**Files:** `firmware/main/wake_word.h`, `firmware/main/wake_word.c`

利用 ESP-SR (乐鑫语音识别 SDK) 在本地实时检测唤醒词“小智小智”，命中后触发录音和云端转发流程。需要先配置 ESP-SR 组件并初始化多网（MultiNet）模型。

完整实现代码已规划 -> **继续进入下一步**

---

## Phase 5: 外网穿透 + MQTT + 集成

### Task 5.1: MQTT 客户端

**Files:** `firmware/main/mqtt_client.h`, `firmware/main/mqtt_client.c`

连接公共 MQTT Broker（如 `broker.emqx.io`），订阅 `device/{id}/cmd` 主题接收手机远程指令，发布 `device/{id}/sensors` 主题上报传感器数据。作为外网备用控制通道。

### Task 5.2: 集成与系统测试

所有模块联调：唤醒词检测 → 录音 → 云 STT → LLM（含 sensor tool call）→ TTS → 喇叭播放。同时验证手机 Web 控制页面的传感器仪表盘和对话面板功能正常。

---

## 附录 A: DS18B20 / MQ-7 / MQ-135 驱动代码

### ds18b20.c
```c
#include "ds18b20.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"

static bool reset(void) {
    gpio_set_direction(ONEWIRE_DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ONEWIRE_DS18B20_GPIO, 0);
    esp_rom_delay_us(480);
    gpio_set_level(ONEWIRE_DS18B20_GPIO, 1);
    esp_rom_delay_us(70);
    gpio_set_direction(ONEWIRE_DS18B20_GPIO, GPIO_MODE_INPUT);
    int presence = gpio_get_level(ONEWIRE_DS18B20_GPIO);
    esp_rom_delay_us(410);
    return presence == 0;
}

static void write_bit(bool bit) {
    gpio_set_direction(ONEWIRE_DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ONEWIRE_DS18B20_GPIO, 0);
    esp_rom_delay_us(bit ? 1 : 60);
    gpio_set_level(ONEWIRE_DS18B20_GPIO, 1);
    esp_rom_delay_us(bit ? 60 : 1);
}

static void write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) { write_bit(byte & 1); byte >>= 1; }
}

static bool read_bit(void) {
    gpio_set_direction(ONEWIRE_DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ONEWIRE_DS18B20_GPIO, 0);
    esp_rom_delay_us(1);
    gpio_set_direction(ONEWIRE_DS18B20_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(14);
    bool bit = gpio_get_level(ONEWIRE_DS18B20_GPIO);
    esp_rom_delay_us(45);
    return bit;
}

static uint8_t read_byte(void) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) { if (read_bit()) byte |= (1 << i); }
    return byte;
}

bool ds18b20_init(void) { return reset(); }

float ds18b20_read_temp(void) {
    if (!reset()) return -127.0f;
    write_byte(0xCC); // skip ROM
    write_byte(0x44); // convert
    vTaskDelay(pdMS_TO_TICKS(750));
    if (!reset()) return -127.0f;
    write_byte(0xCC);
    write_byte(0xBE); // read scratchpad
    uint8_t lo = read_byte();
    uint8_t hi = read_byte();
    int16_t raw = (hi << 8) | lo;
    return raw / 16.0f;
}
```

### mq7.c
```c
#include "mq7.h"
#include "config.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

static esp_adc_cal_characteristics_t adc_cal;

bool mq7_init(void) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12,
                             1100, &adc_cal);
    return true;
}

float mq7_read_ppm(void) {
    int raw = adc1_get_raw(ADC1_CHANNEL_1);
    uint32_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_cal);
    // MQ-7 CO: Rs/R0 vs ppm 曲线, 取近似 5V 标定
    // 实际需标定, 此处返回电压供 LLM 解释
    return mv / 1000.0f;
}
```

### mq135.c — 结构同 mq7.c，使用 ADC1_CHANNEL_3

---

## 附录 B: ESP-SR 唤醒词模型配置要点

1. 在 `CMakeLists.txt` 中添加 `esp-sr` 组件依赖
2. 从乐鑫 Model Zoo 下载 MultiNet 中文唤醒词模型
3. 未命中时持续低功耗监听；命中后触发 `audio_start_capture()` → 云端处理 → 播放响应
4. 录音时长上限 10 秒或检测静音 1.5 秒自动截断

---

## 附录 C: 实施顺序依赖图

```text
1.1 项目骨架
 └→ 1.2 WiFi
      ├→ 1.3 OLED
      │    └→ 2.1 BH1750
      │         └→ 2.2 INA219 ─┐
      └→ 3.1 传感器聚合         ├→ 3.2 Web 服务
           └→ 2.3 温度/CO/空气 ─┘    └→ 3.3 前端 SPA
                                         └→ 4.1 音频 ─┐
                                               └→ 5.1 MQTT
                                             4.2 云中继
                                             4.3 唤醒词 ─┐
                                               └→ 5.2 集成测试
```
