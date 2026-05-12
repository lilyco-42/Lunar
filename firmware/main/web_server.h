#pragma once

/**
 * @brief Start the HTTP web server.
 *
 * Serves:
 *   GET  /              — Single-page dashboard (sensor cards + status)
 *   GET  /api/sensors   — JSON with all sensor readings
 *   GET  /api/status    — JSON with WiFi state, uptime, memory
 *
 * Must be called after WiFi is initialized and sensor task is running.
 */
void web_server_start(void);
