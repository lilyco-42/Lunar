#pragma once

#include "esp_wifi.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_AP_MODE
} wifi_state_t;

/**
 * @brief Initialize WiFi subsystem.
 *
 * Sequence:
 *   1. Init NVS, netif, event loop, WiFi stack.
 *   2. Try STA mode with credentials stored in NVS ("wifi_ssid" / "wifi_pass").
 *   3. If no stored credentials, start SmartConfig (ESP-TOUCH) with 120 s timeout.
 *   4. On SmartConfig timeout, fallback to AP mode.
 *   5. On disconnect, auto-reconnect up to WIFI_MAX_RETRY times.
 *
 * Must be called once during app_main after NVS init.
 */
void wifi_init(void);

/**
 * @brief Get current WiFi state (thread-safe).
 */
wifi_state_t wifi_get_state(void);

/**
 * @brief Get the current IP address as a null-terminated string.
 *
 * Returns "0.0.0.0" when disconnected, or the configured AP IP (192.168.4.1)
 * in AP mode.  The returned pointer is valid for the lifetime of the module.
 */
char *wifi_get_ip_str(void);

#ifdef __cplusplus
}
#endif
