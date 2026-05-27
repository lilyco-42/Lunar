# 传感器变量对照表

| 英文变量名 | 中文设备名 | 传感器型号 | 接口 | 单位/说明 |
|-----------|-----------|-----------|------|----------|
| `dht11_temp` | 温湿度传感器 | DHT11 | OneWire GPIO7 | 摄氏度 (°C) |
| `dht11_humidity` | — | DHT11 | 同上 | 百分比 (%) |
| `ds18b20_temp` | 防水温度探头 | DS18B20 | OneWire GPIO6 | 摄氏度 (°C) |
| `bh1750_lux` | 光照传感器 | BH1750 | I2C 0x23 | 勒克斯 (lux) |
| `ina219_voltage` | 电源监测模块 | INA219 | I2C 0x40 | 伏特 (V) |
| `ina219_current` | — | INA219 | 同上 | 毫安 (mA) |
| `ina219_power` | — | INA219 | 同上 | 毫瓦 (mW) |
| `max9814_level` | 麦克风放大器 | MAX9814 | ADC GPIO0 | 原始值 (0-4095) |
| `mq7_raw` | 一氧化碳传感器 | MQ-7 | ADC GPIO1 | 原始值 (0-4095) |
| `mq135_raw` | 空气质量传感器 | MQ-135 | ADC GPIO3 | 原始值 (0-4095) |
| `ssd1306` | OLED 显示屏 | SSD1306 0.96寸 | I2C 0x3C | 128×64 像素 |
| `pam8403` | 数字功放板 | PAM8403 | PWM GPIO11 | 3W×2 |
| `su03t` | 离线语音模块 | SU-03T | UART GPIO18/19 | 唤醒+播报 |

## jx_firm 变量映射

| jx_firm 变量 | 映射到 |
|-------------|--------|
| `CS_41` (A41) | 温度整数部分 → `dht11_temp / ds18b20_temp` |
| `CS_42` (A42) | 温度小数部分 |
| `CS_3` (A3) | 湿度 → `dht11_humidity` |

## 代码速查

```c
#include "sensor_drivers/bh1750.h"     // 光照传感器
#include "sensor_drivers/ina219.h"     // 电源监测模块
#include "sensor_drivers/adc_sensors.h" // 麦克风放大器 + 传感器探头
#include "sensor_drivers/dht11.h"      // 温湿度传感器
#include "sensor_drivers/ds18b20.h"    // 防水温度探头
#include "sensor_drivers/ssd1306.h"    // OLED 显示屏
#include "audio.h"                      // 音频输出 (PWM)
#include "su03t.h"                      // 离线语音模块
```
