#include "wifi_manager.h"
#include "config.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "wifi";

/* Event group bits */
#define WIFI_STA_CONNECTED_BIT  BIT0
#define WIFI_STA_FAIL_BIT       BIT1

static EventGroupHandle_t g_wifi_events;
static wifi_state_t g_state = WIFI_DISCONNECTED;
static char g_ip_str[16] = "0.0.0.0";
static esp_netif_t *g_sta_netif = NULL;
static esp_netif_t *g_ap_netif  = NULL;
static int g_retry_count = 0;

static void wifi_ap_start(void);

/* ─── Event handler ─── */

static void event_handler(void *arg, esp_event_base_t base,
                          int32_t id, void *event_data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *d = event_data;
        ESP_LOGW(TAG, "STA disconnected, reason=%d", d->reason);
        g_state = WIFI_CONNECTING;
        if (g_retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            g_retry_count++;
            ESP_LOGI(TAG, "Retry %d/%d", g_retry_count, WIFI_MAX_RETRY);
        } else {
            ESP_LOGE(TAG, "Max retries, switching to AP mode");
            xEventGroupSetBits(g_wifi_events, WIFI_STA_FAIL_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = event_data;
        snprintf(g_ip_str, sizeof(g_ip_str), IPSTR, IP2STR(&ev->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", g_ip_str);
        g_retry_count = 0;
        g_state = WIFI_CONNECTED;
        xEventGroupSetBits(g_wifi_events, WIFI_STA_CONNECTED_BIT);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *ev = event_data;
        ESP_LOGI(TAG, "AP: station " MACSTR " joined", MAC2STR(ev->mac));
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *ev = event_data;
        ESP_LOGI(TAG, "AP: station " MACSTR " left", MAC2STR(ev->mac));
    }
}

/* ─── STA mode ─── */

static void wifi_sta_start(void)
{
    g_state = WIFI_CONNECTING;

    /* Check NVS for stored credentials */
    nvs_handle_t nvs;
    bool have_creds = false;
    wifi_config_t cfg = {0};

    if (nvs_open("wifi_creds", NVS_READONLY, &nvs) == ESP_OK) {
        size_t sz = sizeof(cfg.sta.ssid);
        if (nvs_get_str(nvs, "wifi_ssid", (char *)cfg.sta.ssid, &sz) == ESP_OK) {
            ESP_LOGI(TAG, "Found stored SSID: %s", cfg.sta.ssid);
            have_creds = true;
        }
        sz = 64;
        nvs_get_blob(nvs, "wifi_pass", cfg.sta.password, &sz);
        nvs_close(nvs);
    }

    if (have_creds) {
        ESP_LOGI(TAG, "Trying stored credentials...");
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));

        EventBits_t bits = xEventGroupWaitBits(g_wifi_events,
            WIFI_STA_CONNECTED_BIT | WIFI_STA_FAIL_BIT,
            pdFALSE, pdFALSE, pdMS_TO_TICKS(30000));

        if (bits & WIFI_STA_CONNECTED_BIT) {
            ESP_LOGI(TAG, "Connected!");
            return;
        }
    }

    /* No stored creds or connection failed — start AP mode */
    ESP_LOGW(TAG, "STA unavailable, starting AP mode");
    wifi_ap_start();
}

/* ─── AP mode ─── */

static void wifi_ap_start(void)
{
    esp_wifi_disconnect();
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(500));

    esp_wifi_set_mode(WIFI_MODE_AP);

    wifi_config_t cfg = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .password = WIFI_AP_PASSWORD,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 4,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Get AP IP */
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(g_ap_netif, &ip_info);
    snprintf(g_ip_str, sizeof(g_ip_str), IPSTR, IP2STR(&ip_info.ip));

    g_state = WIFI_AP_MODE;
    ESP_LOGI(TAG, "AP mode: SSID=%s, IP=%s", WIFI_AP_SSID, g_ip_str);
}

/* ─── Public API ─── */

void wifi_init(void)
{
    g_wifi_events = xEventGroupCreate();

    /* Init TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    g_sta_netif = esp_netif_create_default_wifi_sta();
    g_ap_netif  = esp_netif_create_default_wifi_ap();

    esp_netif_ip_info_t ap_ip = {
        .ip = { .addr = esp_ip4addr_aton("192.168.4.1") },
        .netmask = { .addr = esp_ip4addr_aton("255.255.255.0") },
        .gw = { .addr = esp_ip4addr_aton("192.168.4.1") },
    };
    esp_netif_dhcps_stop(g_ap_netif);
    esp_netif_set_ip_info(g_ap_netif, &ap_ip);
    esp_netif_dhcps_start(g_ap_netif);

    /* WiFi init */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register event handler for all WiFi + IP events */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
        &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
        &event_handler, NULL, NULL));

    /* Start in APSTA mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi subsystem initialized");
    wifi_sta_start();
}

wifi_state_t wifi_get_state(void)
{
    return g_state;
}

char *wifi_get_ip_str(void)
{
    return g_ip_str;
}
