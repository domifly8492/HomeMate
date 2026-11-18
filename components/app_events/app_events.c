#include "app_events.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"

#define MAX_SUBSCRIBERS  8       /* 队列最多同时存 8 个事件 */

static const char *TAG = "app_events";

static QueueHandle_t     s_subs[MAX_SUBSCRIBERS];
static int               s_sub_count = 0;
static SemaphoreHandle_t s_lock = NULL;       /* 保护订阅者列表 */

esp_err_t app_events_init(void)
{
    if (s_lock != NULL) {
        return ESP_OK;
    }
    s_lock = xSemaphoreCreateMutex();
    return (s_lock != NULL) ? ESP_OK : ESP_ERR_NO_MEM;
}

app_events_sub_t app_events_subscribe(uint8_t queue_len)
{
    if (s_lock == NULL) {
        return NULL;
    }
    QueueHandle_t q = xQueueCreate(queue_len, sizeof(app_event_t));
    if (q == NULL) {
        return NULL;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_sub_count >= MAX_SUBSCRIBERS) {
        xSemaphoreGive(s_lock);
        vQueueDelete(q);
        ESP_LOGE(TAG, "too many subscribers (max %d)", MAX_SUBSCRIBERS);
        return NULL;
    }
    s_subs[s_sub_count++] = q;
    xSemaphoreGive(s_lock);
    return q;
}

bool app_events_post(app_event_id_t id, int32_t arg)
{
    if (s_lock == NULL) {
        return false;
    }
    app_event_t evt = {
        .id      = id,
        .arg     = arg,
        .time_ms = (uint32_t)(esp_timer_get_time() / 1000),
    };

    bool all_ok = true;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    for (int i = 0; i < s_sub_count; i++) {
        if (xQueueSend(s_subs[i], &evt, 0) != pdTRUE) {     /* 等待时间 0：满了就放弃 */
            ESP_LOGW(TAG, "subscriber %d queue full, drop event %d", i, (int)id);
            all_ok = false;
        }
    }
    xSemaphoreGive(s_lock);
    return all_ok;
}

bool app_events_wait(app_events_sub_t sub, app_event_t *out, uint32_t timeout_ms)
{
    if (sub == NULL || out == NULL) {
        return false;
    }
    return xQueueReceive(sub, out, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

const char *app_events_name(app_event_id_t id)
{
    switch (id) {
    case EVT_PIR_TRIGGER:     return "PIR_TRIGGER";
    case EVT_DOOR_OPEN:       return "DOOR_OPEN";
    case EVT_DOOR_CLOSE:      return "DOOR_CLOSE";
    case EVT_BUTTON_SHORT:    return "BUTTON_SHORT";
    case EVT_BUTTON_LONG:     return "BUTTON_LONG";

    case EVT_BTN_REC_DOWN:    return "BTN_REC_DOWN";
    case EVT_BTN_REC_UP:      return "BTN_REC_UP";
     
    case EVT_WIFI_CONNECTED:  return "WIFI_CONNECTED";
    case EVT_WIFI_LOST:       return "WIFI_LOST";
    case EVT_TIME_SYNCED:     return "TIME_SYNCED";
    case EVT_WEATHER_UPDATED: return "WEATHER_UPDATED";
    case EVT_AUDIO_PLAY_DONE: return "AUDIO_PLAY_DONE";
    case EVT_RECORD_DONE:     return "RECORD_DONE";
    case EVT_NEW_MESSAGE:     return "NEW_MESSAGE";
    case EVT_REMINDER:        return "REMINDER";
    default:                  return "?";
    }
}
