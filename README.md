# Lunar — ESP32-C3 多传感器仪表盘

[![ESP-IDF v5.4](https://img.shields.io/badge/ESP--IDF-v5.4-blue)](https://docs.espressif.com/projects/esp-idf/en/v5.4/)
[![ESP32-C3](https://img.shields.io/badge/chip-ESP32--C3-green)](https://www.espressif.com/en/products/socs/esp32-c3)
[![License](https://img.shields.io/badge/license-MIT-lightgrey)](LICENSE)

基于 ESP32-C3 的 9 模块传感器采集终端，8 路环境传感器 + I2S 音频输出，0.96" OLED 实时仪表盘。

## 功能

- **环境感知** — 空气温湿度 (DHT11)、水温 (DS18B20)、光照 (BH1750)、电压/电流/功率 (INA219)、声音强度 (MAX9814)、一氧化碳 (MQ-7)、空气质量 (MQ-135)
- **OLED 仪表盘** — 所有读数实时刷新，8 行信息同屏显示
- **中文渲染框架** — 16x16 点阵字库，UTF-8 自动解码，PCtoLCD2002 即可扩字
- **WiFi / Web / 语音** — 框架已预留，待接入

## 硬件清单

| # | 模块 | 接口 | I2C 地址 / 引脚 | 状态 |
|---|------|------|----------------|------|
| 1 | SSD1306 0.96" OLED | I2C | `0x3C` | ✅ |
| 2 | BH1750 光照传感器 | I2C | `0x23` | ✅ |
| 3 | INA219 电压电流 | I2C | `0x40` | ✅ |
| 4 | MAX9814 麦克风 | ADC | GPIO0 | ✅ |
| 5 | DHT11 温湿度 | OneWire | GPIO7 | ✅ |
| 6 | DS18B20 防水温度 | OneWire | GPIO6 | ✅ |
| 7 | MQ-7 一氧化碳 | ADC | GPIO1 | ✅ |
| 8 | MQ-135 空气质量 | ADC | GPIO3 | ✅ |
| 9 | PCM5102 DAC | I2S | GPIO11/18/19 | ✅ |
| 10 | PAM8403 功放 + 2喇叭 | Analog | — | ✅ |

I2C 总线三个设备并联 SDA/SCL，地址不冲突。DS18B20 需外接 4.7kΩ 上拉电阻。

## 接线图

```
                   ESP32-C3 开发板
              ┌──────────────────────┐
MAX9814 OUT ──┤ GPIO0  (ADC1_CH0)    │
MQ-7 AO ──────┤ GPIO1  (ADC1_CH1)    │   (需 5V + 分压)
MQ-135 AO ────┤ GPIO3  (ADC1_CH3)    │   (需 5V + 分压)
I2C SCL ──────┤ GPIO4               │─── OLED/BH1750/INA219
I2C SDA ──────┤ GPIO5               │─── OLED/BH1750/INA219
DS18B20 DATA ─┤ GPIO6               │   (4.7kΩ 上拉到 3.3V)
DHT11 DATA ───┤ GPIO7               │
I2S BCK ──────┤ GPIO18              │─── PCM5102
I2S LRCK ─────┤ GPIO19              │─── PCM5102
I2S DIN ──────┤ GPIO12              │─── PCM5102
              │                     │
              │ 3.3V → OLED BH1750 INA219 MAX9814 DHT11 DS18B20
              │ 5V   → MQ-7 MQ-135 PAM8403
              │ GND  → 所有设备共地
              └──────────────────────┘
```

> **注意事项**：MQ 传感器 AO 输出 0~5V，需两个等值电阻分压到 0~2.5V 再接 GPIO。GPIO10 被内部 Flash 占用，不可用。

## OLED 仪表盘

```
┌──────────────────────┐
│ Lunar Dashboard      │
│ Air   27C  41%       │  DHT11 空气温湿度
│ Water  26.8C         │  DS18B20 水温
│ Light  2 lux          │  BH1750 光照
│ 3.28V  4mA  13mW     │  INA219 电压/电流/功率
│ Mic 1200             │  MAX9814 麦克风
│ CO 1520  Air 1821    │  MQ-7 / MQ-135
│ 8 sensors OK         │
└──────────────────────┘
```

## 快速开始

### 1. 安装 ESP-IDF

下载 Windows 离线安装器（推荐 v5.4），安装到默认路径：

https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/get-started/

安装器会自动配置 Python virtualenv 和 RISC-V 工具链。

### 2. 克隆仓库

```bash
git clone https://github.com/lilyco-42/Lunar.git
cd Lunar
```

### 3. 编译

```bash
# PowerShell: 加载 ESP-IDF 环境
. .\source.ps1

# 编译固件
cd firmware
idf.py build
```

如果 `source.ps1` 找不到 ESP-IDF，手动设置环境变量后直接运行 `idf.py build`，或用 CMD 执行：

```bash
# CMD 方式
cd firmware
build.bat build
```

### 4. 烧录 + 监视

```bash
# COM6 为例，改成你的端口
idf.py -p COM6 flash monitor

# 或 CMD
build.bat flash COM6
```

> **烧录失败？** 某些 CH343 开发板需手动下载模式：按住 BOOT → 按一下 RST → 松开 BOOT。详见 `docs/firmware-flash-guide.md`。

## 项目结构

```
├── firmware/
│   ├── main/
│   │   ├── config.h                      # 所有引脚、地址宏定义
│   │   ├── main.c                        # 入口 + 仪表盘循环
│   │   ├── CMakeLists.txt                # 源文件列表
│   │   ├── sensor_drivers/
│   │   │   ├── ssd1306.c/h               # OLED 驱动 + 中文 UTF-8 渲染
│   │   │   ├── oled_cn_font.c/h          # 16×16 中文字库
│   │   │   ├── bh1750.c/h                # 光照传感器
│   │   │   ├── ina219.c/h                # 电压/电流/功率
│   │   │   ├── adc_sensors.c/h           # MAX9814 + MQ-x
│   │   │   ├── dht11.c/h                 # DHT11 温湿度 (bit-bang)
│   │   │   └── ds18b20.c/h              # DS18B20 (OneWire bit-bang)
│   │   ├── wifi_manager.c/h             # WiFi STA/AP (WIP)
│   │   ├── web_server.c/h               # HTTP 仪表盘 (WIP)
│   │   ├── audio.c/h                    # I2S 音频 (WIP)
│   │   └── sensor_task.c/h             # FreeRTOS 采集任务 (WIP)
│   ├── sdkconfig.defaults               # ESP-IDF 默认配置
│   ├── build.bat                         # Windows 一键构建
│   ├── idf_env.bat                       # ESP-IDF 环境变量
│   └── README.md                         # 固件编译说明
└── docs/
    ├── wiring-guide.md                   # 逐模块接线详解
    ├── firmware-features.md              # 固件功能清单
    ├── firmware-flash-guide.md           # 烧录排错
    └── project-intro.md                  # 项目设计文档
```

## 传感器驱动 API

### OLED SSD1306

```c
oled_init();                              // 初始化 (I2C 总线需先配置)
oled_clear();                             // 清屏
oled_show_text(page, "text");             // 5×7 ASCII，第 page 行
oled_show_glyph16(page, col, glyph);      // 16×16 字形
oled_show_utf8(page, "UTF-8 text", 0);    // 混合中英文渲染
```

### 传感器读取

```c
float lux = bh1750_read_lux();            // 光照 (lux)，错误返回 -1
float v = ina219_read_bus_voltage();      // 总线电压 (V)
float i = ina219_read_current_ma();       // 电流 (mA)
float p = ina219_read_power_mw();         // 功率 (mW)
int mic = adc_max9814_read_raw();         // 麦克风 ADC 原始值 (0-4095)
int t, h;
dht11_read(&t, &h);                       // DHT11 温湿度
ds18b20_start_conversion();               // DS18B20 启动转换 (750ms)
float t = ds18b20_read_temp();            // DS18B20 温度 (°C)
```

## 中文字库

16x16 点阵渲染框架已就绪，仅需添加字形数据：

1. 下载 [PCtoLCD2002](https://sourceforge.net/projects/pctolcd2002/)
2. 配置：**列行式、阴码、逆向**（bit0 对应页顶端像素）
3. 生成 32 字节字形数据
4. 在 `oled_cn_font.c` 中：
   - `cn_font_codepoint[]` 添加 Unicode 码点
   - `cn_font_16x16[][32]` 添加 32 字节字形

## 故障排查

| 现象 | 可能原因 |
|------|----------|
| I2C 扫描 0 个设备 | SDA/SCL 接反、上拉电阻缺失、设备未供电 |
| DS18B20 未检测 | 无上拉电阻、GPIO10 被占用（改用 GPIO6） |
| DHT11 超时 | 模块未上电、DATA 线接触不良 |
| OLED 花屏 | I2C 地址不匹配（常见 0x3C 或 0x3D） |
| 烧录失败 COM port busy | 先关闭串口监视器再烧 |

## License

MIT
