# Lunar 固件功能说明

> 更新日期：2026-05-22
> 状态：8 传感器 + PWM 音频 + WiFi + Web 仪表盘 完成

## 模块总览

| 模块 | 文件 | 状态 |
|------|------|------|
| 配置中心 | `config.h` | 完成 |
| I2C 总线 + 设备扫描 | `main.c` | 完成 |
| OLED 显示 (SSD1306) | `sensor_drivers/ssd1306.c` | 完成 |
| 中文字库 (16x16) | `sensor_drivers/oled_cn_font.c` | 完成 |
| 表情系统 (32x32) | `sensor_drivers/oled_emoji.c` | 完成 |
| 光照传感器 (BH1750) | `sensor_drivers/bh1750.c` | 完成 |
| 电流监控 (INA219) | `sensor_drivers/ina219.c` | 完成 |
| 温湿度传感器 (DHT11) | `sensor_drivers/dht11.c` | 完成 |
| 温度传感器 (DS18B20) | `sensor_drivers/ds18b20.c` | 完成 |
| 模拟采集 (MQ-7/MQ-135/MAX9814) | `sensor_drivers/adc_sensors.c` | 完成 |
| PWM 音频输出 | `audio.c` (LEDC) | 完成 |
| WiFi 管理 | `wifi_manager.c` | 完成 |
| Web 服务器 + SPA 仪表盘 | `web_server.c` | 完成 |
| 传感器轮询任务 | `sensor_task.c` | 完成 |
| I2S 音频 (PCM5102) | — | 已放弃 |
| SU-03T 语音模块 | — | 待接入 |

---

## 1. 硬件引脚分配

| 外设 | 协议 | 引脚 |
|------|------|------|
| OLED SSD1306 | I2C | SDA=GPIO5, SCL=GPIO4 |
| BH1750 光照 | I2C | 同上 (0x23) |
| INA219 电流 | I2C | 同上 (0x40) |
| MAX9814 麦克风 | ADC | GPIO0 (ADC1_CH0) |
| MQ-7 CO | ADC | GPIO1 (ADC1_CH1) |
| MQ-135 空气质量 | ADC | GPIO3 (ADC1_CH3) |
| DHT11 温湿度 | OneWire | GPIO7 |
| DS18B20 温度 | OneWire | GPIO6 (4.7kΩ上拉) |
| PWM 音频输出 | LEDC | GPIO11 → PAM8403 L-IN |

## 2. PWM 音频 (`audio.c`)

放弃 PCM5102 I2S DAC 后采用 PWM 直连方案：

```
ESP32 LEDC PWM (100kHz) → GPIO11 → PAM8403 → 喇叭
```

- 8-bit 分辨率，~16kHz 采样率
- 100kHz PWM 载波（人耳听不到）
- API：`audio_test_tone(freq, ms)` 播放正弦波测试音

> **PCM5102 调试记录**：H4L(XSMT) 跳线默认接 L(静音)，改 H 时焊盘反复短路无法修复。PWM 音质够用，放弃 PCM5102。

## 3. WiFi (`wifi_manager.c`)

- 启动后优先尝试 STA 模式连接已存储 WiFi
- 30s 无连接 → AP 回退 (SSID: VoiceAssistant, 密码: 12345678)
- 断线自动重连 (最多 5 次)

## 4. Web 仪表盘 (`web_server.c`)

路由：

| 方法 | 路由 | 说明 |
|------|------|------|
| GET | `/` | 单文件 SPA 仪表盘 |
| GET | `/api/sensors` | JSON：全部传感器读数 |

## 5. 传感器驱动

### OLED SSD1306 (I2C 0x3C)
- 128x64 像素，5×7 ASCII 字体
- 16×16 中文字库框架（UTF-8 解码 + 动态注册）
- 32×32 表情系统（8 内置，16 自定义槽位）

### BH1750 (I2C 0x23)
- 连续高分辨率模式，1 lx 精度

### INA219 (I2C 0x40)
- 0.1Ω 分流，32V/2A 量程，12-bit

### DS18B20 (OneWire GPIO6)
- 软件 bit-bang，CRC8 校验，12-bit 精度
- 需 4.7kΩ 上拉

### DHT11 (GPIO7)
- 软件 bit-bang 单线协议，定时采样

### ADC 传感器 (ADC1)
- MAX9814 CH0 / MQ-7 CH1 / MQ-135 CH3
- oneshot 模式，12-bit，eFuse 校准
- MQ 系列需 5V 供电 + 分压 (5V→2.5V)

## 6. 表情系统 (`oled_emoji.c`)

```c
oled_emoji_show(EMOJI_SMILE);      // :)  微笑
oled_emoji_show(EMOJI_BIGSMILE);   // :D  大笑
oled_emoji_show(EMOJI_SURPRISED);  // :O  惊讶
oled_emoji_show(EMOJI_WINK);       // ;)  眨眼
oled_emoji_show(EMOJI_SAD);        // :(  难过
oled_emoji_show(EMOJI_ANGRY);      // >:( 生气
oled_emoji_show(EMOJI_COOL);       // B)  酷
oled_emoji_show(EMOJI_HEART);      // <3  爱心
```

## 7. OLED 仪表盘布局

```
┌──────────────────────┐
│ Lunar Dashboard      │
│ Air   27C  41%       │  DHT11
│ Water  26.8C         │  DS18B20
│ Light  2 lux  Mic 12 │  BH1750 + MAX9814
│ 3.28V 4mA 13mW      │  INA219
│ CO 1520  Air 1821    │  MQ-7 + MQ-135
│ WiFi: 192.168.4.1    │  网络状态
│ 8 sensors  PWM audio │
└──────────────────────┘
```

## 8. 待实现

| 功能 | 依赖 |
|------|------|
| SU-03T 语音模块 (TTS + 唤醒词) | UART |
| MQTT 客户端 | 公共 Broker |
| 云中继服务 (STT/LLM/TTS) | 独立服务器 |
