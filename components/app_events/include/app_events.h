#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/* 事件编号：按来源分组，以后新增事件往里加 */
typedef enum {
    EVT_NONE = 0,

    /* 传感器 */
    EVT_PIR_TRIGGER,        /* 门口有人靠近 */
    EVT_DOOR_OPEN,          /* 门打开 */
    EVT_DOOR_CLOSE,         /* 门关上 */

    /* 按键 */
    EVT_BUTTON_SHORT,       /* 短按，arg = 按键编号 */
    EVT_BUTTON_LONG,        /* 长按，arg = 按键编号 */

    /* 网络 */
    EVT_WIFI_CONNECTED,
    EVT_WIFI_LOST,
    EVT_TIME_SYNCED,
    EVT_WEATHER_UPDATED,

    /* 音频 */
    EVT_AUDIO_PLAY_DONE,    /* 一段语音播完了 */
    EVT_RECORD_DONE,        /* 录音结束 */

    /* 留言 */
    EVT_NEW_MESSAGE,        /* 有新留言，arg = 留言编号 */

    EVT_MAX
} app_event_id_t;

/* 一个事件：编号 + 一个整数参数 + 产生时间 */
typedef struct {
    app_event_id_t id;
    int32_t        arg;
    uint32_t       time_ms;     /* 开机后的毫秒数，投递时自动填写 */
} app_event_t;

/* 创建事件队列，要在所有任务启动之前调用一次 */
esp_err_t app_events_init(void);

/* 投递一个事件。不会阻塞；队列满了返回 false 并丢弃该事件 */
bool app_events_post(app_event_id_t id, int32_t arg);

/* 等待一个事件，最多等 timeout_ms 毫秒。取到返回 true，超时返回 false */
bool app_events_wait(app_event_t *out, uint32_t timeout_ms);