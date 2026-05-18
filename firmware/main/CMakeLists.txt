idf_component_register(
    SRCS "main.c"
         "sensor_drivers/ssd1306.c"
         "sensor_drivers/oled_cn_font.c"
         "sensor_drivers/bh1750.c"
         "sensor_drivers/ina219.c"
         "sensor_drivers/ds18b20.c"
         "sensor_drivers/dht11.c"
         "sensor_drivers/adc_sensors.c"
         "sensor_task.c"
         "wifi_manager.c"
         "web_server.c"
         "audio.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash driver esp_driver_i2s esp_wifi esp_netif esp_http_server esp_adc esp_timer
)
