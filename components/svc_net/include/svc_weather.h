#pragma once
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    float temp;     /* 温度，摄氏度 */
    float feels;    /* 体感温度，摄氏度 */
    int   humi;     /* 相对湿度，% */
    float wind;     /* 风速，米/秒 */
    int   code;     /* WMO 天气代码 */
    int   is_day;   /* 1 白天，0 夜晚 */
    int   rain_prob;   /* 今天的降水概率最大值，0 到 100 */
} svc_weather_t;

/* 启动天气任务。由 svc_net_start() 调用，一般不用自己调 */
esp_err_t svc_weather_start(void);

/* 取最近一次的天气。还没有有效数据时返回 false */
bool svc_weather_get(svc_weather_t *out);

/* 天气代码转成英文短语 */
const char *svc_weather_text(int code);