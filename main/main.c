#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "app_events.h"
#include "drv_display.h"
#include "ui.h"
#include "svc_net.h"
#include "svc_audio.h"
#include "drv_sensors.h"
#include "svc_rules.h"


static const char *TAG = "main";

/* 调试用：把所有事件打印到串口，不需要时可以删掉 */
static void log_task(void *arg)
{
    app_events_sub_t sub = (app_events_sub_t)arg;
    app_event_t evt;
    while (1) {
        if (app_events_wait(sub, &evt, 5000)) {
            ESP_LOGI(TAG, "event %s arg=%d", app_events_name(evt.id), (int)evt.arg);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "HomeMate starting");

    ESP_ERROR_CHECK(app_events_init());                 /* 1. 事件系统最先 */

    app_events_sub_t dbg = app_events_subscribe(16);    /* 2. 调试订阅者 */
    xTaskCreate(log_task, "log", 3072, dbg, 2, NULL);

    ESP_ERROR_CHECK(drv_display_init());                /* 3. 屏幕和 LVGL */
    drv_display_lvgl_start();
    ESP_ERROR_CHECK(ui_init());                         /* 4. 界面 */
    
    ESP_ERROR_CHECK(svc_audio_start());                 /* 音频服务 */
    svc_audio_play(PROMPT_CHIME);                       /* 开机提示音 */

    ESP_ERROR_CHECK(drv_sensors_start());               /* 传感器 */
    ESP_ERROR_CHECK(svc_net_start());                   /* 5. 网络、对时、天气（里面会初始化 NVS） */
    ESP_ERROR_CHECK(svc_rules_start());                 /* 规则：要用 NVS，所以放在网络之后 */
}