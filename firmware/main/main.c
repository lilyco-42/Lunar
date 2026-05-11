#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "config.h"

static const char *TAG = "main";

/*
 * TODO: Uncomment these includes as drivers are implemented:
 * #include "oled.h"          // OLED/SSD1306 display driver
 * #include "wifi_manager.h"  // WiFi AP + STA manager
 * #include "web_server.h"    // HTTP server for SPA / REST API
 * #include "sensor_hub.h"    // Sensor polling coordinator
 */

void app_main(void)
{
    // Initialize NVS flash (required by WiFi stack)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS flash initialized");

    /*
     * TODO: Initialize OLED display and show boot splash
     *
     * oled_init();
     * oled_clear();
     * oled_show_text("Booting...");
     */

    /*
     * TODO: Initialize WiFi (AP mode by default)
     *
     * wifi_init();
     */

    /*
     * TODO: Start sensor hub and web server
     *
     * sensor_hub_start();
     * web_server_start();
     */

    ESP_LOGI(TAG, "Ready");
}
