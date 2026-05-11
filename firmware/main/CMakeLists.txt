idf_component_register(
    SRCS "main.c" "wifi_manager.c" "web_server.c" "sensor_hub.c"
         "sensor_drivers/bh1750.c" "sensor_drivers/ina219.c"
         "sensor_drivers/ds18b20.c" "sensor_drivers/mq7.c"
         "sensor_drivers/mq135.c" "sensor_drivers/ssd1306.c"
         "audio.c" "cloud_relay.c" "mqtt_client.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_http_server spiffs
              driver esp_adc
    EMBED_FILES ../data/index.html
)
