# Lunar — ESP32-C3 传感器仪表盘

基于 ESP32-C3 的 9 模块传感器采集终端，8 个环境传感器实时读数显示在 0.96" OLED 屏幕上。

## 硬件

| 传感器 | 型号 | 接口 | 引脚 | 状态 |
|--------|------|------|------|------|
| 显示屏 | SSD1306 0.96" OLED | I2C (0x3C) | SCL=GPIO4, SDA=GPIO5 | ✅ |
| 光照 | BH1750 | I2C (0x23) | 同上 | ✅ |
| 电源监测 | INA219 | I2C (0x40) | 同上 | ✅ |
| 麦克风 | MAX9814 | ADC | GPIO0 | ✅ |
| 温湿度 | DHT11 | OneWire | GPIO7 | ✅ |
| 温度探头 | DS18B20 | OneWire | GPIO6 | ✅ |
| 一氧化碳 | MQ-7 | ADC | GPIO1 | ✅ |
| 空气质量 | MQ-135 | ADC | GPIO3 | ✅ |
| 音频输出 | PCM5102 + PAM8403 | I2S | BCK=18, LRCK=19, DIN=11 | ✅ |

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
DS18B20 DATA ───┤ GPIO6             │  温度探头 (4.7kΩ上拉)
 DHT11 DATA ────┤ GPIO7             │  温湿度
        [不可用]┤ GPIO8-10          │  strapping/flash
I2S DIN ────────┤ GPIO12            │── PCM5102
I2S BCK ────────┤ GPIO18            │── PCM5102
I2S LRCK ───────┤ GPIO19            │── PCM5102
                │                   │
                │ 3.3V → OLED/BH1750/INA219/MAX9814/DHT11/DS18B20/PCM5102
                │ 5V   → MQ-7/MQ-135/PAM8403
                │ GND  → 所有设备共地
                └───────────────────┘
```

## OLED 显示

```
┌──────────────────────┐
│ Lunar Dashboard      │
│ Air   27C  41%       │  DHT11
│ Water  26.8C         │  DS18B20
│ Light  2 lux          │  BH1750
│ 3.28V  4mA  13mW     │  INA219
│ Mic 1200             │  MAX9814
│ CO 1520  Air 1821    │  MQ-7  MQ-135
│ 8 sensors OK         │
└──────────────────────┘
```

支持 16x16 中文点阵渲染框架（`oled_cn_font.c`），用 PCtoLCD2002 生成字形即可扩展。

## 编译 & 烧录

```bash
# PowerShell: 加载 ESP-IDF
. .\source.ps1

# 编译
cd firmware
idf.py build

# 烧录 + 监视
idf.py -p COM6 flash monitor

# 或 CMD
build.bat build
build.bat flash COM6
```

## 项目结构

```
firmware/
├── main/
│   ├── config.h                    # 引脚定义、I2C 地址
│   ├── main.c                      # 入口，初始化 + 仪表盘循环
│   ├── CMakeLists.txt              # 编译源文件列表
│   ├── audio.c/h                   # I2S 音频播放+录音
│   ├── sensor_task.c/h             # 传感器采集任务
│   ├── wifi_manager.c/h            # WiFi 管理
│   ├── web_server.c/h              # Web 服务器
│   └── sensor_drivers/
│       ├── ssd1306.c/h             # OLED SSD1306 驱动 + 中文渲染
│       ├── oled_cn_font.c/h        # 16x16 中文字库
│       ├── bh1750.c/h              # 光照传感器
│       ├── ina219.c/h              # 电压/电流/功率
│       ├── adc_sensors.c/h         # MAX9814 + MQ 传感器
│       ├── dht11.c/h               # DHT11 温湿度
│       └── ds18b20.c/h             # DS18B20 温度探头
├── sdkconfig.defaults              # ESP-IDF 默认配置
├── build.bat                       # Windows 构建脚本
├── idf_env.bat                     # ESP-IDF 环境变量
└── .gitignore
```

## 中文字库

字库框架已就绪，当前仅含示例字符 `一`。添加新字步骤：

1. 用 [PCtoLCD2002](https://sourceforge.net/projects/pctolcd2002/) 生成 16x16 字形
2. 配置：列行式、阴码、逆向（bit0=上）
3. 粘贴 32 字节到 `oled_cn_font.c`
4. 添加对应的 Unicode 码点到 `cn_font_codepoint[]`

## License

MIT
