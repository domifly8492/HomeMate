#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* 事件编号：按来源分组，以后新增事件往里加 */
typedef enum {
    EVT_NONE = 0,

    /* 传感器 */
    EVT_PIR_TRIGGER,     /* 门口有人靠近 */
    EVT_DOOR_OPEN,       /* 门打开 */
    EVT_DOOR_CLOSE,     /* 门关上 */

    /* 按键 */
    EVT_BUTTON_SHORT,       /* arg = 按键编号 */
    EVT_BUTTON_LONG,
    EVT_BTN_REC_DOWN,       /* 录音键按下 */
    EVT_BTN_REC_UP,         /* 录音键松开 */

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
    /* 提醒 */
    EVT_REMINDER,           /* 有提醒要播报，arg = 提醒编号（见 svc_rules.h） */
    EVT_MAX
} app_event_id_t;

typedef struct {
    app_event_id_t id;
    int32_t        arg;
    uint32_t       time_ms;     /* 开机后的毫秒数，投递时自动填写 */
} app_event_t;

/* 订阅者：其实就是它自己的一个队列 */
typedef QueueHandle_t app_events_sub_t;

/* 初始化，要在所有任务启动之前调用一次 */
esp_err_t app_events_init(void);

/* 订阅：返回这个订阅者自己的队列。queue_len 是它最多能积压多少个事件。
 * 要在投递事件之前订阅，之前投递的事件它收不到 */
app_events_sub_t app_events_subscribe(uint8_t queue_len);

/* 投递：事件会复制给所有订阅者，不会阻塞。
 * 某个订阅者的队列满了，只丢它那一份，其他人照常收到 */
bool app_events_post(app_event_id_t id, int32_t arg);

/* 等待：从自己的队列里取事件，最多等 timeout_ms 毫秒 */
bool app_events_wait(app_events_sub_t sub, app_event_t *out, uint32_t timeout_ms);

/* 事件名称，调试打印用 */
const char *app_events_name(app_event_id_t id);