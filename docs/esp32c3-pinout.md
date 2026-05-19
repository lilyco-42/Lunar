# ESP32-C3 引脚完整对照

22 个 GPIO（GPIO0~GPIO21），5 个不可用，直接路由或经 GPIO 矩阵到外设。

## 引脚总表

```
        ESP32-C3 引脚图
   ┌─────────────────────────┐
   │                     USB │── 烧录/串口
   │                         │
   │  GPIO0  ── ADC1_CH0     │  MAX9814 麦克风 ✓
   │  GPIO1  ── ADC1_CH1     │  MQ-7 一氧化碳 ✓
   │  GPIO2  ── strapping    │  [不可用：boot 电平]
   │  GPIO3  ── ADC1_CH3     │  MQ-135 空气质量 ✓
   │  GPIO4  ── I2C SCL      │  OLED / BH1750 / INA219 ✓
   │  GPIO5  ── I2C SDA      │  OLED / BH1750 / INA219 ✓
   │  GPIO6  ──              │  DS18B20 温度探头 ✓
   │  GPIO7  ──              │  DHT11 温湿度 ✓
   │  GPIO8  ── strapping    │  [不可用：boot 电平]
   │  GPIO9  ── BOOT         │  [不可用：烧录]
   │  GPIO10 ── FSPICS0      │  [不可用：内部 Flash]
   │  GPIO11 ──              │  [空闲]
   │  GPIO12 ──              │  [空闲]
   │  GPIO13 ──              │  [空闲]
   │  GPIO14 ──              │  [空闲]
   │  GPIO15 ──              │  [空闲]
   │  GPIO16 ──              │  [空闲]
   │  GPIO17 ──              │  [空闲]
   │  GPIO18 ── I2S BCK      │  PCM5102 ✓
   │  GPIO19 ── I2S LRCK     │  PCM5102 ✓
   │  GPIO20 ── I2S DOUT     │  PCM5102 ✓
   │  GPIO21 ──              │  [空闲]
   │                         │
   │  3.3V ── 供大部分设备     │
   │  5V   ── MQ-7/MQ-135/PAM8403
   │  GND  ── 全部共地
   └─────────────────────────┘
```

## 引脚速查

| GPIO | 功能 | 使用了什么 | 状态 |
|------|------|-----------|------|
| 0 | ADC1_CH0 | MAX9814 | ✅ 已用 |
| 1 | ADC1_CH1 | MQ-7 | ✅ 已用 |
| 2 | strapping | — | 🚫 不可用 |
| 3 | ADC1_CH3 | MQ-135 | ✅ 已用 |
| 4 | I2C SCL | OLED/BH1750/INA219 | ✅ 已用 |
| 5 | I2C SDA | OLED/BH1750/INA219 | ✅ 已用 |
| 6 | — | DS18B20 | ✅ 已用 |
| 7 | — | DHT11 | ✅ 已用 |
| 8 | strapping | — | 🚫 不可用 |
| 9 | BOOT | — | 🚫 不可用 |
| 10 | FSPICS0 (Flash) | — | 🚫 不可用 |
| 11 | — | — | ✅ 空闲 |
| 12 | — | — | ✅ 空闲 |
| 13 | — | — | ✅ 空闲 |
| 14 | — | — | ✅ 空闲 |
| 15 | — | — | ✅ 空闲 |
| 16 | — | — | ✅ 空闲 |
| 17 | — | — | ✅ 空闲 |
| 18 | I2S BCK | PCM5102 | ✅ 已用 |
| 19 | I2S LRCK | PCM5102 | ✅ 已用 |
| 20 | I2S DOUT | PCM5102 | ✅ 已用 |
| 21 | — | — | ✅ 空闲 |

## 不可用的 5 个引脚

| GPIO | 原因 |
|------|------|
| 2 | Strapping 引脚，决定启动电压 |
| 8 | Strapping 引脚，决定启动模式 |
| 9 | BOOT 按钮，烧录时用 |
| 10 | 内部 SPI Flash 片选（FSPICS0） |

> GPIO11 虽然空闲，但 GPSPI2 等内部信号可能复用其上。I2S 测试时发现 GPIO11 输出的信号 PCM5102 不识别，改用 GPIO20 后正常。建议优先用 GPIO20 做 I2S DIN。

## 当前占用统计

- 已用：13 个（含 5 个不可用）
- 空闲：9 个（GPIO11-17, GPIO21）
- 空闲 ADC：无（ADC1 全部用满，无法再加模拟传感器）
