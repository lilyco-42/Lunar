# 设备接线指南

## I2C 总线（OLED + BH1750 + INA219 共享）

三个设备并联到同一对 SDA/SCL，I2C 地址各不相同。

### OLED 0.96" SSD1306（4 针）

| OLED 引脚 | 接到 |
|-----------|------|
| VCC | 3.3V |
| GND | GND |
| SCL | GPIO4 |
| SDA | GPIO5 |

### BH1750 光照传感器

| BH1750 引脚 | 接到 |
|-------------|------|
| VCC | 3.3V |
| GND | GND |
| SCL | GPIO4 |
| SDA | GPIO5 |
| ADDR | GND（地址 0x23） |

### INA219 电流/电压传感器

| INA219 引脚 | 接到 |
|------------|------|
| VCC | 3.3V |
| GND | GND |
| SCL | GPIO4 |
| SDA | GPIO5 |
| A0 | GND |
| A1 | GND（A0/A1 均接地，地址 0x40） |
| VIN+ | 被测电源正极（串联接入） |
| VIN- | 负载正极 |

> INA219 模块有两组 VIN+/VIN- 端子，内部是连通的，只用一组即可。
> I2C 总线在 SDA、SCL 各加一组 **4.7kΩ** 上拉到 3.3V（大部分模块自带）。

---

## ADC 模拟采集

### MAX9814 麦克风放大器

| MAX9814 引脚 | 接到 |
|-------------|------|
| VDD | 3.3V |
| GND | GND |
| OUT | GPIO0（ADC1_CH0） |
| GAIN | 悬空 = 60dB |
| AR | 悬空（自动增益恢复） |

### MQ-7 一氧化碳传感器

| MQ-7 引脚 | 接到 |
|-----------|------|
| VCC | **5V**（加热器） |
| GND | GND |
| AO | 经分压 → GPIO1（ADC1_CH1） |
| DO | 不接 |

### MQ-135 空气质量传感器

| MQ-135 引脚 | 接到 |
|-------------|------|
| VCC | **5V**（加热器） |
| GND | GND |
| AO | 经分压 → GPIO3（ADC1_CH3） |
| DO | 不接 |

### MQ 传感器分压电路

MQ 模块 5V 供电时 AO 输出 0\~5V，ESP32-C3 ADC 量程约 2.5V（11dB 衰减），**必须分压**，直连会烧 GPIO：

```
MQ AO ──┬── R1 ── GPIO (ADC)
         │
         R2
         │
        GND
```

R1 = R2，推荐 10kΩ，将 0\~5V 降为 0\~2.5V。MQ-7 和 MQ-135 各需一组分压。

---

## 数字传感器

### DHT11 温湿度（3 脚模块）

| DHT11 引脚 | 接到 |
|------------|------|
| VCC | 3.3V |
| GND | GND |
| DATA | GPIO7 |

> 3 脚模块自带板上拉电阻，无需外接。

### DS18B20 防水温度探头（3 线）

| 线色 | 接到 |
|------|------|
| 红 (VCC) | 3.3V |
| 黑 (GND) | GND |
| 黄 (DATA) | GPIO6 |

> **DATA 线必须接 4.7kΩ\~10kΩ 上拉电阻到 3.3V**，否则不通信：
> ```
> 3.3V ──[4.7kΩ]──┬── GPIO6
>                   │
>              DS18B20 DATA(黄)
> ```

---

## I2S 音频输出

音频链路：ESP32-C3 → I2S(数字) → PCM5102 → 模拟 → PAM8403 → 喇叭

### PCM5102 DAC 模块

| PCM5102 引脚 | 接到 |
|-------------|------|
| VCC | 3.3V |
| GND | GND |
| BCK | GPIO18 |
| LRCK | GPIO19 |
| DIN | GPIO12 |
| FMT | GND（I2S 标准格式） |
| SCK | GND（自动时钟，内部 PLL） |
| XSMT | 3.3V（取消静音，必须接！） |

### PAM8403 功放板

| PAM8403 引脚 | 接到 |
|-------------|------|
| VCC | **5V** |
| GND | GND |
| L-IN | PCM5102 L-OUT |
| R-IN | PCM5102 R-OUT |
| L+ / L- | 左喇叭 |
| R+ / R- | 右喇叭 |

---

## 接线总览

```
                    ESP32-C3 开发板
                ┌───────────────────┐
                │                   │
MAX9814 OUT ────┤ GPIO0  (ADC1_CH0) │  麦克风
MQ-7 AO (分压) ─┤ GPIO1  (ADC1_CH1) │  一氧化碳
        [空闲] ─┤ GPIO2             │
MQ-135 AO(分压)─┤ GPIO3  (ADC1_CH3) │  空气质量
 I2C SCL ───────┤ GPIO4             │── OLED/BH1750/INA219
 I2C SDA ───────┤ GPIO5             │── OLED/BH1750/INA219
DS18B20 DATA ───┤ GPIO6             │  温度探头 (4.7kΩ 上拉)
 DHT11 DATA ────┤ GPIO7             │  温湿度
        [不可用]┤ GPIO8  (strapping)│
        [不可用]┤ GPIO9  (BOOT)     │
        [不可用]┤ GPIO10 (Flash)    │  内部 flash 占用
        [空闲] ─┤ GPIO11            │
I2S DIN ────────┤ GPIO12            │── PCM5102
        [空闲] ─┤ GPIO13            │
        [空闲] ─┤ GPIO14            │
        [空闲] ─┤ GPIO15            │
        [空闲] ─┤ GPIO16            │
        [空闲] ─┤ GPIO17            │
I2S BCLK ───────┤ GPIO18            │── PCM5102
I2S LRCK ───────┤ GPIO19            │── PCM5102
        [空闲] ─┤ GPIO20            │
        [空闲] ─┤ GPIO21            │
                │                   │
                │ 3.3V ── OLED BH1750 INA219 MAX9814 DHT11 DS18B20 PCM5102
                │ 5V   ── MQ-7 MQ-135 PAM8403
                │ GND  ── 所有设备共地
                └───────────────────┘
```

---

## 供电分配

| 电压 | 设备 | 来源 |
|------|------|------|
| 3.3V | OLED, BH1750, INA219, MAX9814, DHT11, DS18B20, PCM5102 | 开发板 3.3V |
| 5V | MQ-7, MQ-135, PAM8403 | USB 5V 或外部电源 |

### 功耗估算

| 模块 | 电流 |
|------|------|
| ESP32-C3 | ~80mA |
| MQ-7 加热器 | ~150mA |
| MQ-135 加热器 | ~150mA |
| PAM8403（中音量） | ~200mA |
| OLED | ~15mA |
| 其他 | ~20mA |
| **合计** | **~615mA** |

> 建议使用 **5V / 2A 外部电源**。

---

## I2C 地址一览

| 设备 | 地址 |
|------|------|
| OLED SSD1306 | 0x3C |
| BH1750 | 0x23 |
| INA219 | 0x40 |

---

## 引脚速查

| GPIO | 功能 | 设备 |
|------|------|------|
| 0 | ADC1_CH0 | MAX9814 |
| 1 | ADC1_CH1 | MQ-7 |
| 2 | — | 空闲 |
| 3 | ADC1_CH3 | MQ-135 |
| 4 | I2C SCL | OLED/BH1750/INA219 |
| 5 | I2C SDA | OLED/BH1750/INA219 |
| 6 | OneWire | DS18B20 |
| 7 | OneWire | DHT11 |
| 8 | strapping | 不可用 |
| 9 | BOOT | 不可用 |
| 10 | Flash | 不可用 |
| 11 | — | 空闲 |
| 12 | I2S DIN | PCM5102 |
| 13-17 | — | 空闲 |
| 18 | I2S BCK | PCM5102 |
| 19 | I2S LRCK | PCM5102 |
| 20-21 | — | 空闲 |
