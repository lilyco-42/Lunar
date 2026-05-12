# Lunar 固件功能说明

> 更新日期：2026-05-12
> 状态：基础框架完成，语音管线待实现

## 模块总览

| 模块 | 文件 | 状态 |
|------|------|------|
| 配置中心 | `config.h` | 完成 |
| I2C 总线 + 设备扫描 | `main.c` | 完成 |
| OLED 显示 (SSD1306) | `sensor_drivers/ssd1306.c` | 完成 |
| 光照传感器 (BH1750) | `sensor_drivers/bh1750.c` | 完成 |
| 电流监控 (INA219) | `sensor_drivers/ina219.c` | 完成 |
| 温度传感器 (DS18B20) | `sensor_drivers/ds18b20.c` | 完成 |
| 模拟采集 (MQ-7/MQ-135/MAX9814) | `sensor_drivers/adc_sensors.c` | 完成 |
| 传感器轮询任务 | `sensor_task.c` | 完成 |
| WiFi 管理 | `wifi_manager.c` | 完成 |
| Web 服务器 + SPA 仪表盘 | `web_server.c` | 完成 |
| I2S 音频播放 + ADC 录音 | `audio.c` | 完成 |
| 唤醒词检测 (ESP-SR) | — | 未实现 |
| MQTT 客户端 | — | 未实现 |
| 云中继通信 | — | 未实现 |

---

## 1. 配置中心 (`config.h`)

所有硬件引脚、I2C 地址、WiFi 默认值集中管理：

- **I2C 总线**：GPIO4(SCL) / GPIO5(SDA)，400kHz，共享 OLED + BH1750 + INA219
- **ADC 引脚**：GPIO0(MAX9814) / GPIO1(MQ-7) / GPIO3(MQ-135)，12-bit，11dB 衰减
- **OneWire 引脚**：GPIO10 (DS18B20)
- **I2S 引脚**：GPIO18(BCLK) / GPIO19(LRCLK) / GPIO20(DOUT)
- **WiFi AP 默认**：SSID `VoiceAssistant`，密码 `12345678`
- **唤醒词**：`小智小智`（待 ESP-SR 集成后生效）

---

## 2. 传感器驱动

### 2.1 OLED 显示屏 (SSD1306, I2C 0x3C)

- 128x64 像素，8 行 × 21 字符
- 内置 5×7 ASCII 字体（0x20–0x7F，96 个字符）
- API：`oled_init()` / `oled_clear()` / `oled_show_text(line, text)`
- 开机时显示 8 行仪表盘：WiFi 状态+IP、固件版本、温度、光照、电压/电流/功率、CO/空气质量原始值、麦克风电平、运行时长

### 2.2 光照传感器 (BH1750, I2C 0x23)

- 连续高分辨率模式（1 lx 分辨率，120ms 采样周期）
- API：`bh1750_init()` / `bh1750_read_lux()` → 返回 float lux 值

### 2.3 电流/电压监控 (INA219, I2C 0x40)

- 配置：32V 量程、12-bit ADC、±40mV 分流电压、连续采样
- 校准：0.1Ω 分流电阻，50μA/LSB
- API：`ina219_read_bus_voltage()` → V / `ina219_read_current_ma()` → mA / `ina219_read_power_mw()` → mW

### 2.4 温度传感器 (DS18B20, OneWire GPIO10)

- 软件模拟 OneWire 协议（bit-bang），含临界区时序保护
- 支持 CRC8 校验
- 分阶段读取：先启动转换 → 等 1s → 读取结果（在 sensor_task 中协调）
- API：`ds18b20_init()` / `ds18b20_start_conversion()` / `ds18b20_read_temp()` → °C

### 2.5 模拟传感器 (MQ-7 / MQ-135 / MAX9814)

- ADC1 oneshot 模式，支持 eFuse 校准（自动回退到原始值）
- MAX9814 麦克风包络 → GPIO0 (ADC1_CH0)，用于 VU 表显示
- MQ-7 一氧化碳传感器 → GPIO1 (ADC1_CH1)，经 2:1 分压（5V→2.5V）
- MQ-135 空气质量传感器 → GPIO3 (ADC1_CH3)，经 2:1 分压
- ADC 驱动可在 oneshot（传感器）和 continuous（录音）模式之间切换
- API：`adc_sensors_init()` / `adc_sensors_deinit()` / 各 `adc_xxx_read_raw()`

---

## 3. 传感器轮询任务 (`sensor_task.c`)

- 独立 FreeRTOS 任务，**1Hz 频率**轮询全部传感器
- 线程安全：内部 mutex 保护数据快照
- DS18B20 使用分阶段策略：读取上一次转换结果 → 启动下一次转换（避免阻塞）
- 通过 `sensor_task_get(sensor_data_t *out)` 供其他模块获取最新值

### 数据结构

```c
typedef struct {
    float temperature;   // °C (DS18B20)
    float light_lux;     // Lux (BH1750)
    float bus_voltage;   // V (INA219)
    float current_ma;    // mA (INA219)
    float power_mw;      // mW (INA219)
    int   co_raw;        // MQ-7 原始值
    int   air_raw;       // MQ-135 原始值
    int   mic_level;     // MAX9814 原始值
    int   uptime_sec;    // 运行秒数
} sensor_data_t;
```

---

## 4. WiFi 管理 (`wifi_manager.c`)

**启动流程：**
1. 初始化 TCP/IP 协议栈、WiFi 驱动、事件循环
2. 优先尝试 STA 模式 — 从 NVS (`wifi_creds` namespace) 读取已存储的 SSID/密码
3. 连接成功 → 保持 STA 模式
4. 无凭证或 30s 超时 → 回退到 **AP 模式**（SSID: `VoiceAssistant`，IP: `192.168.4.1`）

**断线重连：** 最多重试 `WIFI_MAX_RETRY`(5) 次，超限后切换 AP 模式。

**API：**
- `wifi_init()` — 初始化
- `wifi_get_state()` → `WIFI_DISCONNECTED | WIFI_CONNECTING | WIFI_CONNECTED | WIFI_AP_MODE`
- `wifi_get_ip_str()` → IP 地址字符串

---

## 5. Web 服务器 (`web_server.c`)

HTTP 服务运行在 80 端口，提供嵌入式 SPA 仪表盘和 REST API。

### 路由

| 方法 | 路由 | 说明 |
|------|------|------|
| GET | `/` | 单文件 SPA 仪表盘（完整 HTML/CSS/JS 内嵌在固件中） |
| GET | `/api/sensors` | JSON：全部传感器最新读数 |
| GET | `/api/status` | JSON：WiFi 状态、IP、可用内存 |
| POST | `/api/audio/tone` | 播放 440Hz 测试音 1s |
| POST | `/api/audio/loopback` | 录音 2s 后回放 |
| GET | `/api/audio/status` | JSON：音频测试状态 (idle/busy) |

### 仪表盘页面功能

- 深色主题，移动端优先的卡片式布局
- 8 张传感器卡片：温度、光照、CO、空气质量、麦克风电平、电压、电流、功率
- 实时 2s 轮询自动刷新
- 页头显示 WiFi 连接状态指示灯（绿/红）和 IP 地址
- 音频测试按钮：440Hz 正弦波测试音、录音回环测试
- 整页约 2KB，纯 HTML/CSS/JS，无外部依赖

---

## 6. I2S 音频 (`audio.c`)

### 播放链路

```
PCM 数据 → I2S 标准模式 → PCM5102 DAC → PAM8403 功放 → 喇叭
           16kHz 16bit    (BCLK=GPIO18
           stereo          LRCLK=GPIO19
                           DOUT=GPIO20)
```

### 录音链路

```
MAX9814 → ADC1_CH0 (GPIO0) → ADC continuous DMA → 16kHz 12bit → int16_t PCM
```

- 录音时将 ADC1 从 oneshot 模式切换到 continuous DMA 模式（传感器 ADC 读数会暂停）
- 录音结束后自动恢复 ADC oneshot 模式，传感器恢复正常

### API

- `audio_init()` — 初始化 I2S TX 通道
- `audio_play_pcm(data, samples)` — 播放 16-bit mono PCM（自动复制到双声道）
- `audio_play_stop()` — 停止播放
- `audio_record_start()` / `audio_record_stop()` / `audio_record_read()` — 录音三件套
- `audio_test_tone(freq, ms)` — 后台任务播放正弦波
- `audio_test_loopback(ms)` — 后台任务录音→回放

---

## 7. 启动流程 (`main.c`)

```
app_main()
 ├─ NVS 初始化
 ├─ WiFi 初始化 (wifi_init)
 ├─ I2C 总线初始化 + 设备扫描
 ├─ OLED 初始化 (oled_init)
 ├─ 传感器驱动初始化 (bh1750, ina219, ds18b20, adc_sensors)
 ├─ 传感器轮询任务启动 (sensor_task_start)
 ├─ I2S 音频初始化 (audio_init)
 ├─ Web 服务器启动 (web_server_start)
 └─ OLED 仪表盘任务启动 (oled_dashboard_task)
```

开机后 OLED 显示实时传感器数据，Web 仪表盘可通过 `http://<设备IP>` 访问。

---

## 8. 待实现功能

按照设计文档中的规格：

| 优先级 | 功能 | 依赖 |
|--------|------|------|
| P0 | ESP-SR 唤醒词检测 ("路娜") | ESP-SR 组件 |
| P0 | MQTT 客户端（外网指令通道） | 公共 Broker |
| P1 | 云中继服务（Node.js / STT / LLM / TTS 代理） | 独立服务器 |
| P1 | LLM Tool Call 传感器查询 | 云中继 + REST API |
| P1 | 语音问答完整链路 | 以上全部 |
| P2 | `/chat` 对话页面 | 前端 SPA |
| P2 | `/settings` 配置页面 | 前端 SPA |
| P2 | WiFi SmartConfig 配网 | ESP-IDF 组件 |
| P3 | frp 外网穿透 | 云服务器 |
