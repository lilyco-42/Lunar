# 设备接线指南

## I2C 总线（OLED + BH1750 + INA219 共享）

三个设备并联到同一对 SDA/SCL，I2C 地址各不相同，无需额外区分。

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
| A0 | GND（地址 0x40） |
| A1 | GND |
| VIN+ | 被测电路正极（串联接入） |
| VIN- | 被测负载端 |

> I2C 总线建议在 SDA、SCL 各加一组 **4.7kΩ 上拉电阻到 3.3V**（部分模块自带上拉，3 个设备共线时一组即可）。

---

## ADC 模拟采集

### MAX9814 麦克风放大器

| MAX9814 引脚 | 接到 |
|-------------|------|
| VCC | 3.3V |
| GND | GND |
| OUT | GPIO0（ADC1_CH0） |
| GAIN | 悬空 = 60dB / 接 VCC = 50dB / 接 GND = 40dB |
| AR | 悬空（默认自动增益恢复） |

### MQ-7 一氧化碳传感器

| MQ-7 模块引脚 | 接到 |
|--------------|------|
| VCC | 5V（加热器需要 5V） |
| GND | GND |
| AO | 经分压 → GPIO1（ADC1_CH1） |
| DO | 不接 |

### MQ-135 空气质量传感器

| MQ-135 模块引脚 | 接到 |
|----------------|------|
| VCC | 5V（加热器需要 5V） |
| GND | GND |
| AO | 经分压 → GPIO3（ADC1_CH3） |
| DO | 不接 |

### MQ 传感器分压电路

MQ 模块 5V 供电时，AO 输出 0\~5V，ESP32-C3 ADC 量程约 2.5V（11dB 衰减），需分压：

```
MQ AO ──┬── 10kΩ ── GPIO (ADC)
         │
        10kΩ
         │
        GND
```

两个等值电阻分压，将 0\~5V 降为 0\~2.5V。

---

## OneWire 温度传感器

### DS18B20（防水探头，3 线）

| 线色 | 引脚 | 接到 |
|------|------|------|
| 红 | VCC | 3.3V |
| 黑 | GND | GND |
| 黄 | DATA | GPIO6 |

> DATA 线需 **4.7kΩ 上拉到 3.3V**：
> ```
> 3.3V ── 4.7kΩ ──┬── GPIO10
>                   │
>               DS18B20 DATA
> ```

---

## I2S 音频输出

PAM8403 是模拟功放，ESP32-C3 无内置 DAC，需在中间加 I2S DAC 模块。以下为完整链路：

### PCM5102 DAC 模块

| PCM5102 引脚 | 接到 |
|-------------|------|
| VCC | 3.3V |
| GND | GND |
| BCK | GPIO18 |
| LRCK | GPIO19 |
| DIN | GPIO20 |
| FMT | GND（I2S 标准格式） |
| SCK | GND（自动时钟） |
| XSMT | 3.3V（取消静音） |

### PAM8403 功放板

| PAM8403 引脚 | 接到 |
|-------------|------|
| VCC | 5V |
| GND | GND |
| L-IN | PCM5102 L-OUT |
| R-IN | PCM5102 R-OUT |
| L-OUT+ / L-OUT- | 左喇叭 |
| R-OUT+ / R-OUT- | 右喇叭 |

---

## 接线总览

```
                    ESP32-C3 开发板
                ┌───────────────────┐
                │                   │
MAX9814 OUT ────┤ GPIO0  (ADC1_CH0) │
MQ-7 AO (分压) ─┤ GPIO1  (ADC1_CH1) │
        [空闲] ─┤ GPIO2  (strapping)│
MQ-135 AO(分压)─┤ GPIO3  (ADC1_CH3) │
 I2C SCL ───────┤ GPIO4             │──── OLED/BH1750/INA219
 I2C SDA ───────┤ GPIO5             │──── OLED/BH1750/INA219
        [空闲] ─┤ GPIO6             │
        [空闲] ─┤ GPIO7             │
        [空闲] ─┤ GPIO8  (strapping)│
        [BOOT] ─┤ GPIO9             │
DS18B20 DATA ───┤ GPIO10            │
                │                   │
I2S BCLK ───────┤ GPIO18            │──── PCM5102 BCK
I2S LRCLK ──────┤ GPIO19            │──── PCM5102 LRCK
I2S DOUT ───────┤ GPIO20            │──── PCM5102 DIN
        [空闲] ─┤ GPIO21 (UART TX)  │
                │                   │
                │ 3.3V ─── OLED/BH1750/INA219/MAX9814/DS18B20/PCM5102
                │ 5V ───── MQ-7/MQ-135/PAM8403
                │ GND ──── 所有设备共地
                └───────────────────┘
```

---

## 供电

| 电压 | 设备 | 来源 |
|------|------|------|
| 3.3V | OLED, BH1750, INA219, MAX9814, DS18B20, PCM5102 | 开发板 3.3V 输出 |
| 5V | MQ-7, MQ-135, PAM8403 | USB 5V 或外部电源 |

### 功耗估算

| 模块 | 电流 |
|------|------|
| ESP32-C3 | \~80mA |
| MQ-7 加热器 | \~150mA |
| MQ-135 加热器 | \~150mA |
| PAM8403（中音量） | \~200mA |
| OLED | \~15mA |
| 其他 | \~20mA |
| **合计** | **\~615mA** |

> 建议使用 **5V/2A 外部电源** 供电，避免仅靠 USB 供电导致加热器不足或功放削顶。
