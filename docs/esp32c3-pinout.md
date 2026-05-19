# ESP32-C3 引脚完整参考

> 来源：ESP-IDF v5.4 官方文档（ESP32-C3 专用）

## 基本规格

- 22 个物理 GPIO（GPIO0~GPIO21）
- 所有 GPIO 均可通过 **GPIO 矩阵** 路由到任意外设
- **IO_MUX**（直连）：低延迟，每个 GPIO 只能连固定的几个外设
- **GPIO 矩阵**：灵活路由，但增加约 25ns 延迟
- 部分引脚有 strapping / boot 限制

## 引脚限制

| 引脚 | 限制 | 原因 |
|------|------|------|
| GPIO2 | 🚫 Strapping 引脚 | 影响芯片启动模式 |
| GPIO8 | 🚫 Strapping 引脚 | 影响芯片启动模式 |
| GPIO9 | 🚫 Strapping 引脚 / BOOT 按钮 | 影响芯片启动模式 |
| GPIO10 | 🚫 Flash | 内部 SPI Flash 片选 (FSPICS0) |
| GPIO11 | ⚠️ Flash | 内部 SPI Flash 占用（部分板子） |
| GPIO12-17 | ⚠️ Flash | 内部 SPI Flash 数据线（部分板子） |
| GPIO18 | ⚠️ USB-JTAG (D-) | 默认 USB 占用，配给 I2S 后 USB-JTAG 失效 |
| GPIO19 | ⚠️ USB-JTAG (D+) | 默认 USB 占用，配给 I2S 后 USB-JTAG 失效 |
| GPIO0-5 | Deep-Sleep 唤醒 | 可唤醒深度睡眠（其他 GPIO 只能唤醒浅睡眠） |

> **注意**: Flash 引脚占用情况因板型而异。经典版 ESP32-C3 的 flash 在模组内部，**GPIO11-17 可能在部分板子上空闲**，但官方文档指出它们"不推荐作其他用途"。

## 当前本项目占用

```
        ESP32-C3 引脚图
   ┌─────────────────────────┐
   │                     USB │── 烧录/串口/CDC/JTAG
   │                         │
   │  GPIO0  ── ADC1_CH0     │  MAX9814 麦克风 ✓
   │  GPIO1  ── ADC1_CH1     │  MQ-7 一氧化碳 ✓
   │  GPIO2  🚫 strapping     │  [不可用]
   │  GPIO3  ── ADC1_CH3     │  MQ-135 空气质量 ✓
   │  GPIO4  ── I2C SCL      │  OLED / BH1750 / INA219 ✓
   │  GPIO5  ── I2C SDA      │  OLED / BH1750 / INA219 ✓
   │  GPIO6  ──              │  DS18B20 温度探头 ✓
   │  GPIO7  ──              │  DHT11 温湿度 ✓
   │  GPIO8  🚫 strapping     │  [不可用]
   │  GPIO9  🚫 BOOT 按钮     │  [不可用]
   │  GPIO10 🚫 FSPICS0       │  [不可用]
   │  GPIO11 ⚠️ Flash         │  [空闲，不推荐用]
   │  GPIO12 ⚠️ Flash         │  [空闲，不推荐用]
   │  GPIO13 ⚠️ Flash         │  [空闲，不推荐用]
   │  GPIO14 ⚠️ Flash         │  [空闲，不推荐用]
   │  GPIO15 ⚠️ Flash         │  [空闲，不推荐用]
   │  GPIO16 ⚠️ Flash         │  [空闲，不推荐用]
   │  GPIO17 ⚠️ Flash         │  [空闲，不推荐用]
   │  GPIO18 ── I2S BCK      │  PCM5102 ✓ (⚠️USB-JTAG失效)
   │  GPIO19 ── I2S LRCK     │  PCM5102 ✓ (⚠️USB-JTAG失效)
   │  GPIO20 ── I2S DOUT     │  PCM5102 ✓
   │  GPIO21 ──              │  [空闲]
   │                         │
   │  3.3V ── 供大部分设备     │
   │  5V   ── MQ-7/MQ-135/PAM8403
   │  GND  ── 全部共地
   └─────────────────────────┘
```

## 引脚速查表

| GPIO | 功能 | 项目使用 | 状态 |
|------|------|----------|------|
| 0 | ADC1_CH0 / 深度唤醒 | MAX9814 | ✅ 已用 |
| 1 | ADC1_CH1 / 深度唤醒 | MQ-7 | ✅ 已用 |
| 2 | Strapping | — | 🚫 不可用 |
| 3 | ADC1_CH3 / 深度唤醒 | MQ-135 | ✅ 已用 |
| 4 | ADC1_CH4 / 深度唤醒 | I2C SCL | ✅ 已用 |
| 5 | / 深度唤醒 | I2C SDA | ✅ 已用 |
| 6 | — | DS18B20 | ✅ 已用 |
| 7 | — | DHT11 | ✅ 已用 |
| 8 | Strapping | — | 🚫 不可用 |
| 9 | BOOT | — | 🚫 不可用 |
| 10 | FSPICS0 (Flash) | — | 🚫 不可用 |
| 11 | Flash (不推荐) | — | ⚠️ 空闲，不推荐 |
| 12 | Flash (不推荐) | — | ⚠️ 空闲，不推荐 |
| 13 | Flash (不推荐) | — | ⚠️ 空闲，不推荐 |
| 14 | Flash (不推荐) | — | ⚠️ 空闲，不推荐 |
| 15 | Flash (不推荐) | — | ⚠️ 空闲，不推荐 |
| 16 | Flash (不推荐) | — | ⚠️ 空闲，不推荐 |
| 17 | Flash (不推荐) | — | ⚠️ 空闲，不推荐 |
| 18 | USB D- / I2S BCK | PCM5102 BCK | ✅ 已用 |
| 19 | USB D+ / I2S LRCK | PCM5102 LRCK | ✅ 已用 |
| 20 | I2S DOUT | PCM5102 DIN | ✅ 已用 |
| 21 | — | — | ✅ 空闲 |

## I2S 音频调试笔记

GPIO11 连接 PCM5102 DIN 时，串口日志显示 I2S 初始化成功、数据写入正常，但 PCM5102 无模拟输出。改用 GPIO20 后问题可能解决。

**已知原因**：
- GPIO11 在 ESP32-C3 上与 SPI Flash 信号复用（官方文档标为 "不推荐作其他用途"）
- GPIO 矩阵路由增加约 25ns 延迟，I2S 高速时钟对此敏感
- GPIO20 是 I2S 数据输出常用引脚，无 Flash 复用冲突

## 资源占用统计

- **已用 GPIO**: 10 个（不含不可用的）
- **不可用 GPIO**: 3 个 (strapping/boot) + 1 个 (flash CS)
- **不推荐 GPIO**: 7 个 (flash 数据线 GPIO11-17)
- **空闲 GPIO**: 1 个 (GPIO21)
- **ADC 已满**: CH0/CH1/CH3 全部占用，无法再加模拟传感器
- **I2C 总线**: 3 个设备 (0x23, 0x3C, 0x40)，SDA/SCL 各需 4.7kΩ 上拉

## 关键注意事项

1. **GPIO11 不要用于 I2S/SPI 等高速外设**（Flash 复用 + GPIO 矩阵延迟）
2. GPIO18/19 配给 I2S 后 **USB-JTAG 失效**（不影响 USB 串口和烧录）
3. 如需扩展传感器，只能用 I2C（总线最多 127 个设备，地址不冲突即可）或 OneWire
