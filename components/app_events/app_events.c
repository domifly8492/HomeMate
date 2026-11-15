#include "app_events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"

#define EVENT_QUEUE_LEN  16     /* 队列最多同时存 16 个事件 */

static const char *TAG = "app_events";
static QueueHandle_t s_queue = NULL;

esp_err_t app_events_init(void)
{
    if (s_queue != NULL) {
        return ESP_OK;                      /* 已经创建过了 */
    }
    s_queue = xQueueCreate(EVENT_QUEUE_LEN, sizeof(app_event_t));
    if (s_queue == NULL) {
        ESP_LOGE(TAG, "create queue failed");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

bool app_events_post(app_event_id_t id, int32_t arg)
{
    if (s_queue == NULL) {
        return false;
    }
    app_event_t evt = {
        .id      = id,
        .arg     = arg,
        .time_ms = (uint32_t)(esp_timer_get_time() / 1000),
    };
    if (xQueueSend(s_queue, &evt, 0) != pdTRUE) {   /* 等待时间 0：满了就放弃 */
        ESP_LOGW(TAG, "queue full, drop event %d", (int)id);
        return false;
    }
    return true;
}

bool app_events_wait(app_event_t *out, uint32_t timeout_ms)
{
    if (s_queue == NULL || out == NULL) {
        return false;
    }
    return xQueueReceive(s_queue, out, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}