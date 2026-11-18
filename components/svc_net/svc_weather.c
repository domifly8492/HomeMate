#include <string.h>
#include "svc_weather.h"
#include "svc_net.h"
#include "app_events.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "svc_weather";

#define WEATHER_URL \
    "https://api.open-meteo.com/v1/forecast?latitude=" CONFIG_HM_WEATHER_LAT \
    "&longitude=" CONFIG_HM_WEATHER_LON \
    "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day," \
    "weather_code,wind_speed_10m" \
    "&daily=precipitation_probability_max&forecast_days=1" \
    "&wind_speed_unit=ms&timezone=auto"


#define RESP_BUF_SIZE    2048
#define REFRESH_OK_MS    (2 * 60 * 60 * 1000)     /* 成功后 2 小时再取 */
#define REFRESH_FAIL_MS  (5 * 60 * 1000)          /* 失败后 5 分钟重试 */

static char s_resp[RESP_BUF_SIZE];
static int  s_resp_len = 0;

static svc_weather_t     s_data;
static bool              s_valid = false;
static SemaphoreHandle_t s_mutex = NULL;

/* 服务器的回复可能分几段到，每到一段就拼到缓冲区后面 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        int copy = evt->data_len;
        if (s_resp_len + copy > RESP_BUF_SIZE - 1) {
            copy = RESP_BUF_SIZE - 1 - s_resp_len;
        }
        if (copy > 0) {
            memcpy(s_resp + s_resp_len, evt->data, copy);
            s_resp_len += copy;
        }
    }
    return ESP_OK;
}

static esp_err_t parse_weather(const char *json, svc_weather_t *out)
{
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        return ESP_FAIL;
    }
    esp_err_t ret = ESP_FAIL;
    cJSON *cur = cJSON_GetObjectItem(root, "current");
    cJSON *t = cJSON_GetObjectItem(cur, "temperature_2m");
    cJSON *f = cJSON_GetObjectItem(cur, "apparent_temperature");
    cJSON *h = cJSON_GetObjectItem(cur, "relative_humidity_2m");
    cJSON *w = cJSON_GetObjectItem(cur, "wind_speed_10m");
    cJSON *c = cJSON_GetObjectItem(cur, "weather_code");
    cJSON *dj = cJSON_GetObjectItem(cur, "is_day");
    if (cJSON_IsNumber(t) && cJSON_IsNumber(f) && cJSON_IsNumber(h) &&
        cJSON_IsNumber(w) && cJSON_IsNumber(c)) {
        out->temp  = (float)t->valuedouble;
        out->feels = (float)f->valuedouble;
        out->humi  = h->valueint;
        out->wind  = (float)w->valuedouble;
        out->code  = c->valueint;
        out->is_day = cJSON_IsNumber(dj) ? dj->valueint : 1;     /* 取不到就当白天 */

            /* 今天的降水概率：daily 里的数组，第 0 项就是今天 */
        cJSON *daily = cJSON_GetObjectItem(root, "daily");
        cJSON *pp    = cJSON_GetObjectItem(daily, "precipitation_probability_max");
        cJSON *pp0   = cJSON_GetArrayItem(pp, 0);
        out->rain_prob = cJSON_IsNumber(pp0) ? pp0->valueint : 0;
        ret = ESP_OK;
    }
    cJSON_Delete(root);                         /* cJSON 用完要释放 */
    return ret;
}

static esp_err_t weather_fetch(void)
{
    s_resp_len = 0;
    s_resp[0] = '\0';
    ESP_LOGI(TAG, "requesting weather...");

    esp_http_client_config_t config = {
        .url               = WEATHER_URL,
        .event_handler     = http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,     /* 用证书包验证服务器 */
        .timeout_ms        = 10000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        s_resp[s_resp_len] = '\0';
        if (status == 200) {
            svc_weather_t d;
            if (parse_weather(s_resp, &d) == ESP_OK) {
                xSemaphoreTake(s_mutex, portMAX_DELAY);
                s_data  = d;
                s_valid = true;
                xSemaphoreGive(s_mutex);
            } else {
                ESP_LOGE(TAG, "parse failed");
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "HTTP status = %d", status);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

static void weather_task(void *arg)
{
    while (1) {
        /* 等 Wi-Fi 连上，并且尽量等对时完成；对时 20 秒还没成就不等了 */
        int waited = 0;
        while (!(svc_net_is_connected() && (svc_net_time_valid() || waited >= 20))) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            waited = svc_net_is_connected() ? waited + 1 : 0;
        }

        uint32_t delay_ms;
        if (weather_fetch() == ESP_OK) {
            app_events_post(EVT_WEATHER_UPDATED, 0);
            delay_ms = REFRESH_OK_MS;
        } else {
            delay_ms = REFRESH_FAIL_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

esp_err_t svc_weather_start(void)
{
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }
    /* HTTPS 握手占栈比较多，给 8KB */
    xTaskCreate(weather_task, "svc_weather", 8192, NULL, 3, NULL);
    return ESP_OK;
}

bool svc_weather_get(svc_weather_t *out)
{
    bool ok = false;
    if (s_mutex && out && xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (s_valid) {
            *out = s_data;
            ok = true;
        }
        xSemaphoreGive(s_mutex);
    }
    return ok;
}

const char *svc_weather_text(int code)
{
    switch (code) {
    case 0:  return "Clear";
    case 1:  return "Mainly Clear";
    case 2:  return "Partly Cloudy";
    case 3:  return "Overcast";
    case 45: return "Fog";
    case 48: return "Rime Fog";
    case 51: case 53: return "Light Rain";
    case 55: return "Rain";
    case 56: case 57: case 66: case 67: return "Freezing Rain";
    case 61: return "Light Rain";
    case 63: return "Rain";
    case 65: return "Heavy Rain";
    case 71: return "Light Snow";
    case 73: return "Snow";
    case 75: return "Heavy Snow";
    case 77: return "Snow Grains";
    case 80: return "Light Showers";
    case 81: return "Showers";
    case 82: return "Heavy Showers";
    case 85: case 86: return "Snow Showers";
    case 95: return "Thunderstorm";
    case 96: case 99: return "Hail Storm";
    default: return "Unknown";
    }
}