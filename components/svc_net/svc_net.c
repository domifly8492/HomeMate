#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "svc_net.h"
#include "app_events.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "svc_weather.h"

static const char *TAG = "svc_net";

static volatile bool s_connected  = false;
static volatile bool s_time_valid = false;

/* Wi-Fi 和 IP 事件：驱动在后台通知我们 */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_connected) {
            s_connected = false;
            app_events_post(EVT_WIFI_LOST, 0);
        }
        ESP_LOGW(TAG, "wifi disconnected, reconnecting...");
        esp_wifi_connect();                         /* 一直重连 */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&e->ip_info.ip));
        s_connected = true;
        app_events_post(EVT_WIFI_CONNECTED, 0);
    }
}

/* 服务任务：连上网之后启动 SNTP，等到对时成功，发出 EVT_TIME_SYNCED */
static void net_task(void *arg)
{
    bool sntp_started = false;

    setenv("TZ", CONFIG_HM_TZ, 1);
    tzset();

    while (1) {
        if (s_connected && !sntp_started) {
            esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_HM_NTP_SERVER);
            if (esp_netif_sntp_init(&cfg) == ESP_OK) {
                sntp_started = true;
                ESP_LOGI(TAG, "SNTP started: %s", CONFIG_HM_NTP_SERVER);
            }
        }
        if (sntp_started && !s_time_valid) {
            if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(1000)) == ESP_OK) {
                s_time_valid = true;
                time_t now = time(NULL);
                struct tm t;
                localtime_r(&now, &t);
                ESP_LOGI(TAG, "time synced: %04d-%02d-%02d %02d:%02d:%02d",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                         t.tm_hour, t.tm_min, t.tm_sec);
                app_events_post(EVT_TIME_SYNCED, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

esp_err_t svc_net_start(void)
{
    /* 1. NVS：Wi-Fi 驱动要用 */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 2. 网络协议栈和默认事件循环（循环已存在也没关系） */
    ESP_ERROR_CHECK(esp_netif_init());
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }
    esp_netif_create_default_wifi_sta();

    /* 3. Wi-Fi 驱动 */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wc = { 0 };
    snprintf((char *)wc.sta.ssid, sizeof(wc.sta.ssid), "%s", CONFIG_HM_WIFI_SSID);
    snprintf((char *)wc.sta.password, sizeof(wc.sta.password), "%s", CONFIG_HM_WIFI_PASSWORD);
    wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wc.sta.pmf_cfg.capable  = true;
    wc.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 4. 服务任务 */
    xTaskCreate(net_task, "svc_net", 4096, NULL, 4, NULL);
    ESP_ERROR_CHECK(svc_weather_start());    //让 svc_net_start() 启动天气
    return ESP_OK;
}

bool svc_net_is_connected(void) { return s_connected; }
bool svc_net_time_valid(void)   { return s_time_valid; }

int svc_net_wifi_level(int *rssi_out)
{
    wifi_ap_record_t ap;
    if (!s_connected || esp_wifi_sta_get_ap_info(&ap) != ESP_OK) {
        return -1;
    }
    if (rssi_out) {
        *rssi_out = ap.rssi;
    }
    if (ap.rssi >= -55) return 4;
    if (ap.rssi >= -65) return 3;
    if (ap.rssi >= -75) return 2;
    if (ap.rssi >= -85) return 1;
    return 0;
}