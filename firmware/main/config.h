#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// I2C Bus
// ============================================================
#define I2C_MASTER_SDA_IO           GPIO_NUM_5
#define I2C_MASTER_SCL_IO           GPIO_NUM_4
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_PORT             I2C_NUM_0

// ============================================================
// I2C Device Addresses
// ============================================================
#define OLED_I2C_ADDR               0x3C
#define BH1750_I2C_ADDR             0x23
#define INA219_I2C_ADDR             0x40

// ============================================================
// ADC Pins
// ============================================================
#define ADC_MAX9814_PIN             GPIO_NUM_0    // Microphone amplifier
#define ADC_MQ7_PIN                 GPIO_NUM_1    // Carbon monoxide sensor
#define ADC_MQ135_PIN               GPIO_NUM_3    // Air quality sensor

// ============================================================
// OneWire / Temperature Sensor
// ============================================================
#define ONEWIRE_DS18B20_PIN         GPIO_NUM_6

// ============================================================
// DHT11 Temperature & Humidity
// ============================================================
#define DHT11_DATA_PIN              GPIO_NUM_7

// ============================================================
// I2S Audio Interface
// ============================================================
#define I2S_BCLK_PIN                GPIO_NUM_18
#define I2S_LRCLK_PIN               GPIO_NUM_19
#define I2S_DOUT_PIN                GPIO_NUM_20

// ============================================================
// WiFi Defaults (Access Point Mode)
// ============================================================
#define WIFI_AP_SSID                "VoiceAssistant"
#define WIFI_AP_PASSWORD            "12345678"
#define WIFI_MAX_RETRY              5

// ============================================================
// Cloud Relay
// ============================================================
#define CLOUD_RELAY_URL             "http://your-server:3000"

// ============================================================
// Voice / Wake Word
// ============================================================
#define WAKE_WORD_STRING            "小智小智"

#endif // CONFIG_H
