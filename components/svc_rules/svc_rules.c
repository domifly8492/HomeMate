#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include "svc_rules.h"
#include "app_events.h"
#include "svc_audio.h"
#include "svc_net.h"
#include "svc_weather.h"
#include "sdkconfig.h"
#include "nvs.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "svc_rules";

#define GAP_MS  6000        /* 两条提醒之间隔 6 秒，让弹窗和语音各自播完 */

/* ================= 1. 提醒清单 =================
 * 数据驱动：一条提醒就是表里的一行，规则代码本身不用改。 */
typedef struct {
    const char     *key;        /* 在 NVS 里记录"哪天提醒过"用的名字，最多 15 个字符 */
    const char     *title;      /* 弹窗标题 */
    audio_prompt_t  prompt;     /* 播放哪一段语音 */
    bool          (*condition)(void);   /* 今天要不要提醒？NULL 表示每天都要 */
} reminder_def_t;

static bool cond_rain(void);    /* 先声明，下面的表要用 */

static const reminder_def_t DEFS[REMIND_COUNT] = {
    [REMIND_KEYS]     = { "keys",     "Take your keys",   PROMPT_CHIME,    NULL      },
    [REMIND_UMBRELLA] = { "umbrella", "Take an umbrella", PROMPT_UMBRELLA, cond_rain },
};

/* ================= 2. 条件：今天会下雨吗 ================= */
static bool is_rain_code(int c)
{
    /* WMO 代码：51 到 67 是毛毛雨和雨，80 到 82 是阵雨，95 以上是雷暴 */
    return (c >= 51 && c <= 67) || (c >= 80 && c <= 82) || (c >= 95 && c <= 99);
}

static bool cond_rain(void)
{
#if CONFIG_HM_RULES_TEST_MODE
    return true;                            /* 测试模式：不管天气，一律认为要下雨 */
#else
    svc_weather_t w;
    if (!svc_weather_get(&w)) {
        return false;                       /* 还没有天气数据，不乱提醒 */
    }
    /* 今天降水概率达到一半，或者现在就在下，都算要带伞 */
    return w.rain_prob >= 50 || is_rain_code(w.code);
#endif
}

/* ================= 3. 记住"今天提醒过没有" =================
 * NVS 是 Flash 里的一小块键值存储，断电不丢。
 * 我们存的是日期数字，比如 20261117，而不是"提醒过 / 没提醒过"：
 * 这样第二天日期变了，自然就当成"还没提醒"，不需要谁去清零。 */
static nvs_handle_t s_nvs;
static bool         s_nvs_ok = false;

static bool done_today(int i, int today)
{
    if (!s_nvs_ok) {
        return false;
    }
    int32_t v = 0;
    return nvs_get_i32(s_nvs, DEFS[i].key, &v) == ESP_OK && v == today;
}

static void mark_done(int i, int today)
{
#if !CONFIG_HM_RULES_TEST_MODE              /* 测试模式不写，省得反复擦写 Flash */
    if (s_nvs_ok) {
        nvs_set_i32(s_nvs, DEFS[i].key, today);
        nvs_commit(s_nvs);
    }
#endif
}

/* ================= 4. 有人在门口：逐条检查，该提醒的就提醒 ================= */
static void run_reminders(void)
{
    /* 检查 1：时间要是准的，否则不知道今天是几号 */
    if (!svc_net_time_valid()) {
        ESP_LOGW(TAG, "time not synced yet, skip");
        return;
    }
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);

#if !CONFIG_HM_RULES_TEST_MODE
    /* 检查 2：在不在提醒的时间窗口里（比如早上 7 点到 10 点） */
    if (t.tm_hour < CONFIG_HM_REMIND_START_HOUR || t.tm_hour >= CONFIG_HM_REMIND_END_HOUR) {
        ESP_LOGI(TAG, "outside the reminder window (hour=%d)", t.tm_hour);
        return;
    }
#endif

    int today = (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;

    for (int i = 0; i < REMIND_COUNT; i++) {
        const reminder_def_t *d = &DEFS[i];

        /* 检查 3：这条提醒的条件成立吗（比如要下雨） */
        if (d->condition != NULL && !d->condition()) {
            continue;
        }
        /* 检查 4：今天已经提醒过就不再提醒 */
#if !CONFIG_HM_RULES_TEST_MODE
        if (done_today(i, today)) {
            continue;
        }
#endif
        ESP_LOGI(TAG, "remind: %s", d->title);
        mark_done(i, today);                    /* 先记下，再提醒，万一中途断电也不会重复 */
        app_events_post(EVT_REMINDER, i);       /* 通知界面弹窗 */
        svc_audio_play(d->prompt);              /* 请音频服务播放语音 */
        vTaskDelay(pdMS_TO_TICKS(GAP_MS));      /* 等这一条播完、弹窗看完，再提醒下一条 */
    }
}

/* ================= 5. 规则任务 ================= */
static app_events_sub_t s_sub;

static void rules_task(void *arg)
{
    app_event_t evt;
    uint32_t ignore_before_ms = 0;

    while (1) {
        if (!app_events_wait(s_sub, &evt, 1000)) {
            continue;                           /* 1 秒没事件，回去继续等 */
        }
        /* 只关心"有人到门口"这两种事件 */
        if (evt.id != EVT_PIR_TRIGGER && evt.id != EVT_DOOR_OPEN) {
            continue;
        }
        /* 提醒进行期间堆积的旧事件作废，不然提醒完马上又来一轮 */
        if (evt.time_ms < ignore_before_ms) {
            continue;
        }
        ESP_LOGI(TAG, "someone at the door (%s)", app_events_name(evt.id));
        run_reminders();
        ignore_before_ms = (uint32_t)(esp_timer_get_time() / 1000);
    }
}

esp_err_t svc_rules_start(void)
{
    s_sub = app_events_subscribe(16);           /* 订阅事件，拿到自己的队列 */
    if (s_sub == NULL) {
        return ESP_FAIL;
    }
    if (nvs_open("rules", NVS_READWRITE, &s_nvs) == ESP_OK) {
        s_nvs_ok = true;
    } else {
        ESP_LOGW(TAG, "nvs open failed, reminders will not be remembered");
    }
    xTaskCreate(rules_task, "svc_rules", 4096, NULL, 4, NULL);
    return ESP_OK;
}

/* ================= 6. 给界面用的文字 ================= */
const char *svc_rules_title(reminder_id_t id)
{
    return (id < REMIND_COUNT) ? DEFS[id].title : "";
}

void svc_rules_detail(reminder_id_t id, char *buf, size_t n)
{
    switch (id) {
    case REMIND_KEYS:
        snprintf(buf, n, "Do not leave without them");
        break;
    case REMIND_UMBRELLA: {
        svc_weather_t w;
        if (svc_weather_get(&w)) {
            snprintf(buf, n, "Rain today - %d%% chance", w.rain_prob);
        } else {
            snprintf(buf, n, "Rain is expected today");
        }
        break;
    }
    default:
        buf[0] = '\0';
        break;
    }
}