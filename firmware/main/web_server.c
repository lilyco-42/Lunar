#include "web_server.h"
#include "config.h"
#include "sensor_task.h"
#include "wifi_manager.h"
#include "audio.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_chip_info.h"
#include <string.h>

static const char *TAG = "web";

/* ─── Embedded SPA (single-file dashboard) ─── */

static const char INDEX_HTML[] =
"<!DOCTYPE html>"
"<html lang=\"zh\">"
"<head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>Lunar Dashboard</title>"
"<style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;"
"background:#0f1117;color:#e1e4e8;min-height:100vh}"
".h{background:#161b22;border-bottom:1px solid#30363d;padding:12px 16px;"
"display:flex;justify-content:space-between;align-items:center}"
".h h1{font-size:18px;font-weight:600;color:#58a6ff}"
".h .meta{font-size:12px;color:#8b949e;text-align:right}"
".h .ip{color:#7ee787}"
".h .dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:4px}"
".h .dot.on{background:#3fb950}"
".h .dot.off{background:#f85149}"
".grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));"
"gap:8px;padding:16px;max-width:700px;margin:0 auto}"
".card{background:#161b22;border:1px solid#30363d;border-radius:8px;padding:12px 14px;"
"text-align:center}.card .v{font-size:24px;font-weight:700;color:#58a6ff;margin:6px 0}"
".card .l{font-size:11px;color:#8b949e;text-transform:uppercase;letter-spacing:.5px}"
".card .u{font-size:11px;color:#484f58;margin-left:2px}"
".err{color:#f85149;font-size:12px;text-align:center}"
"footer{text-align:center;font-size:11px;color:#484f58;padding:16px}"
"@media(max-width:400px){.grid{grid-template-columns:repeat(2,1fr);gap:6px;padding:10px}}"
".aud{max-width:700px;margin:0 auto;padding:0 16px 16px}"
".aud h2{font-size:14px;color:#8b949e;margin:8px 0;text-transform:uppercase;letter-spacing:.5px}"
".aud button{background:#21262d;color:#c9d1d9;border:1px solid#30363d;border-radius:6px;"
"padding:8px 16px;margin-right:8px;cursor:pointer;font-size:13px}"
".aud button:hover{background:#30363d}.aud button:disabled{opacity:.4;cursor:not-allowed}"
"#astat{font-size:12px;color:#58a6ff;margin-left:4px}"
"</style></head><body>"
"<div class=\"h\">"
"<h1>Lunar</h1>"
"<div class=\"meta\">"
"<div><span class=\"dot\" id=\"dot\"></span><span id=\"wstate\">--</span></div>"
"<div class=\"ip\" id=\"ip\">--</div></div></div>"
"<div class=\"grid\" id=\"grid\"></div>"
"<div class=\"aud\"><h2>Audio Tests</h2>"
"<button id=\"bt\" onclick=\"testTone()\">Tone 440Hz</button>"
"<button id=\"bl\" onclick=\"testLoop()\">Loopback 2s</button>"
"<span id=\"astat\"></span></div>"
"<footer>Lunar Voice Assistant</footer>"
"<script>"
"async function load(){"
"try{"
"let[ss,st]=await Promise.all(["
"fetch('/api/sensors').then(r=>r.json()),"
"fetch('/api/status').then(r=>r.json())]);"
"render(ss,st);"
"}catch(e){document.getElementById('grid').innerHTML="
"'<div class=err>Disconnected</div>'}"
"document.getElementById('wstate').textContent='WiFi offline';"
"document.getElementById('dot').className='dot off'}"
"function render(s,t){"
"document.getElementById('wstate').textContent=t.wifi_state;"
"document.getElementById('dot').className='dot '+(t.wifi_state=='Connected'?'on':'off');"
"document.getElementById('ip').textContent=t.ip;"
"var g=document.getElementById('grid');"
"var c=["
"{l:'Temperature',v:s.temperature,u:'&deg;C'},"
"{l:'Light',v:s.light_lux,u:'lx'},"
"{l:'CO (raw)',v:s.co_raw,u:''},"
"{l:'Air Q (raw)',v:s.air_raw,u:''},"
"{l:'Mic Level',v:s.mic_level,u:''},"
"{l:'Bus Voltage',v:s.bus_voltage,u:'V'},"
"{l:'Current',v:s.current_ma,u:'mA'},"
"{l:'Power',v:s.power_mw,u:'mW'},"
"];"
"g.innerHTML=c.map(function(i){"
"var v=i.v==null||i.v<-900?'--':Number(i.v).toFixed(1);"
"return'<div class=card><div class=l>'+i.l+'</div><div class=v>'+v+'<span class=u>'+i.u+'</span></div></div>'"
"}).join('')}"
"setInterval(load,2000);load();"
"function setBtns(d){document.getElementById('bt').disabled=d;"
"document.getElementById('bl').disabled=d}"
"function showSt(t){document.getElementById('astat').textContent=t}"
"function pollDone(){"
"setTimeout(async function(){try{"
"var r=await fetch('/api/audio/status');"
"var j=await r.json();"
"if(j.audio=='busy'){showSt('Playing...');pollDone()}"
"else{showSt('Done');setBtns(false);setTimeout(function(){showSt('')},2000)}"
"}catch(e){setBtns(false);showSt('')}},500)}"
"async function testTone(){"
"setBtns(true);showSt('Playing tone...');"
"try{var r=await fetch('/api/audio/tone',{method:'POST'});"
"var j=await r.json();if(!r.ok){showSt(j.error);setBtns(false);return}"
"setTimeout(function(){showSt('Done');setBtns(false);"
"setTimeout(function(){showSt('')},2000)},1500)"
"}catch(e){showSt('Error');setBtns(false)}}"
"async function testLoop(){"
"setBtns(true);showSt('Recording 2s...');"
"try{var r=await fetch('/api/audio/loopback',{method:'POST'});"
"var j=await r.json();if(!r.ok){showSt(j.error);setBtns(false);return}"
"setTimeout(function(){showSt('Playing back...');pollDone()},2200)"
"}catch(e){showSt('Error');setBtns(false)}}"
"</script></body></html>";

/* ─── REST API handlers ─── */

static esp_err_t handle_get_index(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, INDEX_HTML, sizeof(INDEX_HTML) - 1);
    return ESP_OK;
}

static esp_err_t handle_get_sensors(httpd_req_t *req)
{
    sensor_data_t s;
    sensor_task_get(&s);

    char buf[512];
    int len = snprintf(buf, sizeof(buf),
        "{\"temperature\":%.1f,\"light_lux\":%.1f,"
        "\"bus_voltage\":%.2f,\"current_ma\":%.1f,\"power_mw\":%.1f,"
        "\"co_raw\":%d,\"air_raw\":%d,\"mic_level\":%d,"
        "\"uptime_sec\":%d}",
        s.temperature, s.light_lux,
        s.bus_voltage, s.current_ma, s.power_mw,
        s.co_raw, s.air_raw, s.mic_level,
        s.uptime_sec);

    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_send(req, buf, len);
    return ESP_OK;
}

static esp_err_t handle_get_status(httpd_req_t *req)
{
    wifi_state_t w = wifi_get_state();
    char *ip = wifi_get_ip_str();
    const char *wifi_str = "Disconnected";
    if (w == WIFI_CONNECTED)   wifi_str = "Connected";
    else if (w == WIFI_CONNECTING) wifi_str = "Connecting";
    else if (w == WIFI_AP_MODE)    wifi_str = "AP Mode";

    /* Free heap */
    int heap = esp_get_free_heap_size();

    char buf[256];
    int len = snprintf(buf, sizeof(buf),
        "{\"wifi_state\":\"%s\",\"ip\":\"%s\",\"free_heap\":%d}",
        wifi_str, ip, heap);

    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_send(req, buf, len);
    return ESP_OK;
}

/* ─── Audio test handlers ─── */

static esp_err_t handle_post_audio_tone(httpd_req_t *req)
{
    if (audio_test_busy()) {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\":\"test in progress\"}");
        return ESP_OK;
    }
    audio_test_tone(440, 1000);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"playing\",\"freq\":440,\"ms\":1000}");
    return ESP_OK;
}

static esp_err_t handle_post_audio_loopback(httpd_req_t *req)
{
    if (audio_test_busy()) {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\":\"test in progress\"}");
        return ESP_OK;
    }
    audio_test_loopback(2000);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"recording\",\"ms\":2000}");
    return ESP_OK;
}

static esp_err_t handle_get_audio_status(httpd_req_t *req)
{
    const char *s = audio_test_busy() ? "busy" : "idle";
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "{\"audio\":\"%s\"}", s);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, len);
    return ESP_OK;
}

/* ─── Public API ─── */

void web_server_start(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 8;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }

    /* Routes */
    httpd_uri_t uri_index = {
        .uri = "/", .method = HTTP_GET,
        .handler = handle_get_index, .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &uri_index);

    httpd_uri_t uri_sensors = {
        .uri = "/api/sensors", .method = HTTP_GET,
        .handler = handle_get_sensors, .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &uri_sensors);

    httpd_uri_t uri_status = {
        .uri = "/api/status", .method = HTTP_GET,
        .handler = handle_get_status, .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &uri_status);

    httpd_uri_t uri_tone = {
        .uri = "/api/audio/tone", .method = HTTP_POST,
        .handler = handle_post_audio_tone, .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &uri_tone);

    httpd_uri_t uri_loopback = {
        .uri = "/api/audio/loopback", .method = HTTP_POST,
        .handler = handle_post_audio_loopback, .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &uri_loopback);

    httpd_uri_t uri_audio_status = {
        .uri = "/api/audio/status", .method = HTTP_GET,
        .handler = handle_get_audio_status, .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &uri_audio_status);

    ESP_LOGI(TAG, "HTTP server started on port %d", cfg.server_port);
}
