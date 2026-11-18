#include <string.h>
#include "svc_audio.h"
#include "drv_audio.h"
#include "app_events.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "svc_audio";

/* 编译时嵌进固件的文件，符号名由文件名决定 */
extern const uint8_t chime_wav_start[]    asm("_binary_chime_wav_start");
extern const uint8_t chime_wav_end[]      asm("_binary_chime_wav_end");
extern const uint8_t umbrella_wav_start[] asm("_binary_umbrella_wav_start");
extern const uint8_t umbrella_wav_end[]   asm("_binary_umbrella_wav_end");

typedef struct {
    const uint8_t *start;
    const uint8_t *end;
    const char    *name;
} blob_t;

static const blob_t BLOBS[PROMPT_COUNT] = {
    [PROMPT_CHIME]    = { chime_wav_start,    chime_wav_end,    "chime"    },
    [PROMPT_UMBRELLA] = { umbrella_wav_start, umbrella_wav_end, "umbrella" },
};

static QueueHandle_t s_queue = NULL;
static volatile int  s_volume = 40;

static uint32_t rd32(const uint8_t *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint16_t rd16(const uint8_t *p)
{
    return p[0] | (p[1] << 8);
}

/* 从 WAV 文件里找到声音数据。只接受 16kHz、单声道、16 位的 PCM */
static bool wav_find_data(const uint8_t *buf, size_t len,
                          const uint8_t **data, size_t *data_len)
{
    if (len < 44 || memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) {
        ESP_LOGE(TAG, "not a WAV file");
        return false;
    }
    bool have_fmt = false;
    size_t pos = 12;
    while (pos + 8 <= len) {
        uint32_t csize = rd32(buf + pos + 4);
        const uint8_t *body = buf + pos + 8;

        if (memcmp(buf + pos, "fmt ", 4) == 0 && csize >= 16) {
            uint16_t format   = rd16(body);
            uint16_t channels = rd16(body + 2);
            uint32_t rate     = rd32(body + 4);
            uint16_t bits     = rd16(body + 14);
            if (format != 1 || channels != 1 || bits != 16 || rate != AUDIO_SAMPLE_RATE) {
                ESP_LOGE(TAG, "unsupported WAV: fmt=%u ch=%u rate=%u bits=%u "
                              "(need PCM, 1 ch, %d Hz, 16 bit)",
                         format, channels, (unsigned)rate, bits, AUDIO_SAMPLE_RATE);
                return false;
            }
            have_fmt = true;
        } else if (memcmp(buf + pos, "data", 4) == 0) {
            if (!have_fmt) {
                return false;
            }
            if (pos + 8 + csize > len) {
                csize = len - pos - 8;          /* 文件头里的长度不对就以实际长度为准 */
            }
            *data = body;
            *data_len = csize;
            return true;
        }
        pos += 8 + csize + (csize & 1);         /* 块的长度要凑成偶数 */
    }
    return false;
}

static void play_blob(const blob_t *b)
{
    const uint8_t *pcm;
    size_t bytes;
    if (!wav_find_data(b->start, b->end - b->start, &pcm, &bytes)) {
        return;
    }
    ESP_LOGI(TAG, "playing %s (%u ms)", b->name,
             (unsigned)(bytes / 2 * 1000 / AUDIO_SAMPLE_RATE));

    /* 分段复制到局部缓冲区再播放，避免直接读取可能没对齐的数据 */
    int16_t tmp[256];
    size_t off = 0;
    while (off < bytes) {
        size_t n = bytes - off;
        if (n > sizeof(tmp)) {
            n = sizeof(tmp);
        }
        memcpy(tmp, pcm + off, n);
        drv_audio_play_pcm(tmp, (int)(n / 2), s_volume);
        off += n;
    }
}

static void audio_task(void *arg)
{
    audio_prompt_t id;
    while (1) {
        if (xQueueReceive(s_queue, &id, portMAX_DELAY) == pdTRUE) {
            if (id < PROMPT_COUNT) {
                play_blob(&BLOBS[id]);
            }
            app_events_post(EVT_AUDIO_PLAY_DONE, (int32_t)id);
        }
    }
}

esp_err_t svc_audio_start(void)
{
    s_queue = xQueueCreate(4, sizeof(audio_prompt_t));
    if (s_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }
    ESP_ERROR_CHECK(drv_audio_init());
    xTaskCreate(audio_task, "svc_audio", 4096, NULL, 4, NULL);
    return ESP_OK;
}

bool svc_audio_play(audio_prompt_t id)
{
    return s_queue != NULL && xQueueSend(s_queue, &id, 0) == pdTRUE;
}

void svc_audio_set_volume(int volume)
{
    if (volume < 0)   volume = 0;
    if (volume > 100) volume = 100;
    s_volume = volume;
}