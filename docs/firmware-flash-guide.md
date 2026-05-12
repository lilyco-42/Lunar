# ESP32-C3 固件烧录指南

## 环境信息

| 项 | 值 |
|---|---|
| 开发板 | ESP32-C3 经典版 (CH343 USB-UART) |
| 框架 | ESP-IDF v5.4 |
| 下载工具 | esptool.py v4.11.0 |
| 芯片 | ESP32-C3 (QFN32) v0.4 |
| 晶振 | 40MHz |
| Flash | 4MB DIO |

---

## 日常使用（最简方式）

### 桌面快捷方式

双击桌面的 **ESP32-C3 Shell** 图标 → 打开命令行 → 直接输入命令：

```bat
build.bat build                  ← 编译
build.bat flash COM6             ← 烧录 + 查看串口
build.bat monitor COM6           ← 仅查看串口
```

### 全部命令

```bat
build.bat build         编译固件
build.bat flash COMx    烧录并打开串口监视
build.bat monitor COMx  仅串口监视
build.bat clean         清理编译产物
build.bat menuconfig    图形化配置菜单
build.bat size          查看固件各组件大小
build.bat shell         打开新的 ESP-IDF 环境窗口
```

---

## 环境架构

```
firmware/
├── idf_env.bat          ← 环境变量（所有路径集中管理）
├── build.bat            ← 日常编译/烧录入口
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
└── main/
```

- **`idf_env.bat`** — 定义了 IDF_PATH、工具链路径、Python 路径。被 build.bat 和桌面快捷方式引用。换电脑只需改这一个文件。
- **`build.bat`** — 封装 `idf.py` 的常用操作，自动引用 `idf_env.bat`。
- **桌面快捷方式** — 双击打开 cmd 窗口，自动加载环境，进入 firmware 目录。

---

## 换电脑后恢复环境

```bat
REM 1. 克隆 ESP-IDF
git clone --depth 1 -b v5.4 https://github.com/espressif/esp-idf.git %USERPROFILE%\esp\esp-idf-v5.4

REM 2. 安装工具链
%USERPROFILE%\esp\esp-idf-v5.4\install.bat esp32c3

REM 3. 更新 idf_env.bat 中的工具版本号（如果有变化）
```

---

## GPIO 引脚对照表

| 外设 | 协议 | 引脚 |
|---|---|---|
| SSD1306 OLED | I2C | SDA=GPIO5, SCL=GPIO4 |
| BH1750 光照 | I2C | 同上（共享总线） |
| INA219 电流 | I2C | 同上 |
| MAX9814 麦克风 | ADC | GPIO0 (ADC1_CH0) |
| MQ-7 CO | ADC | GPIO1 (ADC1_CH1) |
| MQ-135 空气质量 | ADC | GPIO3 (ADC1_CH3) |
| DS18B20 温度 | OneWire | GPIO10 |
| PAM8403 功放 | I2S | BCLK=18, LRCLK=19, DOUT=20 |

### I2C 设备地址

| 设备 | 地址 |
|---|---|
| OLED (SSD1306) | 0x3C |
| BH1750 | 0x23 |
| INA219 | 0x40 |

---

## 常见问题

### 烧录失败: PermissionError

关闭 Arduino IDE、串口助手等占用 COM 口的程序。

### 烧录失败: Invalid head of packet (0x49)

错误信息示例：`Failed to connect to ESP32-C3: Invalid head of packet (0x49): Possible serial noise or corruption.`

这表示 ESP32-C3 未进入下载模式（正在运行旧固件输出日志，0x49 = 字符 'I'）。即使开发板支持 DTR/RTS 自动复位，CH343 芯片在此板上自动下载不一定可靠。

**解决：** 手动进入下载模式：
1. 按住 **BOOT** 键不放
2. 点按一下 **EN** 键松开
3. 松开 **BOOT** 键
4. 立即执行烧录命令

### 烧录失败: COM 口无法打开

可能原因：
1. 设备状态异常（设备管理器中显示 Unknown）→ 拔出 USB 线等 3 秒再插入
2. 操作按键时 USB 线被碰松 → 检查连接，换 USB 口
3. 驱动问题 → 从 [WCH 官网](https://www.wch.cn/downloads/CH343SER_EXE.html) 重装 CH343 驱动

### 编译找不到 esp_smartconfig

ESP-IDF v5.x 中 `esp_smartconfig` 已移至组件管理器，需通过 `idf.py add-dependency espressif/esp_smartconfig` 单独安装。如不需要 SmartConfig 配网功能，可直接用 STA + AP 回退模式替代。

### 编译错误: esp_timer.h not found

`CMakeLists.txt` 的 `REQUIRES` 列表中需添加 `esp_timer` 组件。

### 编译错误: FreeRTOS.h must appear before task.h

`main.c` 中 `#include "freertos/FreeRTOS.h"` 必须写在 `#include "freertos/task.h"` 之前。

### 编译错误: implicit declaration of function 'memset'

`.c` 文件缺少 `#include <string.h>`。

### 烧录后屏幕不显示

1. 查看 I2C 扫描结果：`build.bat monitor COM6`
2. `Device found at 0x3C` → I2C 通信正常，检查 OLED 接线
3. `NO devices found` → 检查 SDA/SCL 是否接对（GPIO5/GPIO4）

### 编译找不到源文件

`CMakeLists.txt` 中列出的 `.c` 文件必须存在。新增模块后需要更新 `firmware/main/CMakeLists.txt` 的 `SRCS` 列表。
