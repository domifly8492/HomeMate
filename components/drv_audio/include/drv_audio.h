#pragma once
#include <stdint.h>
#include "esp_err.h"

/* 初始化喇叭的 I2S 输出，16kHz、16 位 */
esp_err_t drv_audio_init(void);

/* 播放一个提示音。volume 是 0 到 100，播完才返回 */
void drv_audio_beep(int freq_hz, int ms, int volume);

/* 播放一段 16 位单声道 PCM，采样率就是 AUDIO_SAMPLE_RATE，播完才返回 */
void drv_audio_play_pcm(const int16_t *pcm, int samples, int volume);