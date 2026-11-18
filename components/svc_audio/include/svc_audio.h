#pragma once
#include <stdbool.h>
#include "esp_err.h"

/* 内置的提示音。新增一段就在这里加一项，再去 svc_audio.c 里加对应的文件 */
typedef enum {
    PROMPT_CHIME = 0,
    PROMPT_UMBRELLA,
    PROMPT_COUNT,
} audio_prompt_t;

/* 启动音频服务（会初始化喇叭）。要在 app_events_init() 之后调用 */
esp_err_t svc_audio_start(void);

/* 请求播放一段提示音，立刻返回，不会等它播完。队列满了返回 false */
bool svc_audio_play(audio_prompt_t id);

/* 音量 0 到 100 */
void svc_audio_set_volume(int volume);