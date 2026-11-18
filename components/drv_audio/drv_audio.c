#include <math.h>
#include "drv_audio.h"
#include "board.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

static const char *TAG = "drv_audio";
static i2s_chan_handle_t s_tx = NULL;

#define CHUNK 128                       /* 每次送 128 个采样 */

esp_err_t drv_audio_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;         /* 没有数据时输出静音，避免噪声 */
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &s_tx, NULL));

    /* 用立体声格式，左右声道送一样的数据。
     * MAX98357A 默认输出左右声道的平均值，只送左声道的话音量会减半 */
    i2s_std_config_t cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                        I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = PIN_SPK_BCLK,
            .ws   = PIN_SPK_LRC,
            .dout = PIN_SPK_DIN,
            .din  = I2S_GPIO_UNUSED,
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(s_tx, &cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(s_tx));

    ESP_LOGI(TAG, "speaker ready: %d Hz, 16 bit", AUDIO_SAMPLE_RATE);
    return ESP_OK;
}

/* 把单声道数据乘上音量，复制成左右两个声道，送出去 */
static void write_mono(const int16_t *mono, int n, int volume)
{
    int16_t frame[CHUNK * 2];
    int gain = volume * 256 / 100;      /* 0 到 256 */

    for (int i = 0; i < n; i += CHUNK) {
        int m = (n - i < CHUNK) ? (n - i) : CHUNK;
        for (int j = 0; j < m; j++) {
            int16_t s = (int16_t)(((int)mono[i + j] * gain) >> 8);
            frame[2 * j]     = s;
            frame[2 * j + 1] = s;
        }
        size_t written = 0;
        i2s_channel_write(s_tx, frame, m * 2 * sizeof(int16_t), &written, 1000);
    }
}

void drv_audio_beep(int freq_hz, int ms, int volume)
{
    const int total = AUDIO_SAMPLE_RATE * ms / 1000;
    const int fade  = AUDIO_SAMPLE_RATE / 200;      /* 头尾各 5 毫秒渐变，避免"啪"声 */
    int16_t mono[CHUNK];

    for (int i = 0; i < total; i += CHUNK) {
        int m = (total - i < CHUNK) ? (total - i) : CHUNK;
        for (int j = 0; j < m; j++) {
            int k = i + j;
            float env = 1.0f;
            if (k < fade) {
                env = (float)k / fade;
            } else if (total - k < fade) {
                env = (float)(total - k) / fade;
            }
            float phase = 2.0f * (float)M_PI * freq_hz * k / AUDIO_SAMPLE_RATE;
            mono[j] = (int16_t)(12000.0f * env * sinf(phase));
        }
        write_mono(mono, m, volume);
    }
}

void drv_audio_play_pcm(const int16_t *pcm, int samples, int volume)
{
    write_mono(pcm, samples, volume);
}