#include "drv_sensors.h"
#include "board.h"
#include "app_events.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "drv_sensors";

#define SCAN_MS          10             /* 每 10 毫秒读一次 */
#define DEBOUNCE_MS      50             /* 状态连续 50 毫秒不变才算数 */
#define PIR_MIN_GAP_MS   2000           /* 两次"有人"事件至少隔 2 秒 */

typedef struct {
    int  pin;
    bool active_low;        /* true：低电平表示"有效"（比如按下） */
    bool state;             /* 当前确认的状态，true 表示"有效" */
    int  count;             /* 防抖计数 */
} input_t;

static input_t s_pir  = { .pin = PIN_PIR,        .active_low = false };   /* 有人 = 高 */
static input_t s_door = { .pin = PIN_DOOR,       .active_low = false };   /* 门开 = 高 */
static input_t s_btn  = { .pin = PIN_BTN_RECORD, .active_low = true  };   /* 按下 = 低 */

static void cfg_input(int pin, bool pull_up, bool pull_down)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << pin,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = pull_up   ? GPIO_PULLUP_ENABLE   : GPIO_PULLUP_DISABLE,
        .pull_down_en = pull_down ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
}

static bool read_active(const input_t *in)
{
    int level = gpio_get_level(in->pin);
    return in->active_low ? (level == 0) : (level == 1);
}

/* 防抖：状态连续 DEBOUNCE_MS 都和当前确认的状态不同，才改变，并返回 true */
static bool update(input_t *in)
{
    bool now = read_active(in);
    if (now == in->state) {
        in->count = 0;
        return false;
    }
    if (++in->count >= DEBOUNCE_MS / SCAN_MS) {
        in->state = now;
        in->count = 0;
        return true;
    }
    return false;
}

static void sensor_task(void *arg)
{
    int64_t last_pir_ms = -PIR_MIN_GAP_MS;

    while (1) {
        if (update(&s_pir) && s_pir.state) {            /* 只在"有人"的一瞬间发 */
            int64_t now = esp_timer_get_time() / 1000;
            if (now - last_pir_ms >= PIR_MIN_GAP_MS) {
                last_pir_ms = now;
                app_events_post(EVT_PIR_TRIGGER, 0);
            }
        }
        if (update(&s_door)) {
            app_events_post(s_door.state ? EVT_DOOR_OPEN : EVT_DOOR_CLOSE, 0);
        }
        if (update(&s_btn)) {
            app_events_post(s_btn.state ? EVT_BTN_REC_DOWN : EVT_BTN_REC_UP, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(SCAN_MS));
    }
}

esp_err_t drv_sensors_start(void)
{
    cfg_input(PIN_PIR,        false, true);     /* PIR：下拉，没接的时候不会乱触发 */
    cfg_input(PIN_DOOR,       true,  false);    /* 门磁：上拉 */
    cfg_input(PIN_BTN_RECORD, true,  false);    /* 按键：上拉 */

    /* 记下开机时的初始状态，不当成事件发出去 */
    s_pir.state  = read_active(&s_pir);
    s_door.state = read_active(&s_door);
    s_btn.state  = read_active(&s_btn);
    ESP_LOGI(TAG, "initial: pir=%d door_open=%d btn=%d",
             s_pir.state, s_door.state, s_btn.state);

    xTaskCreate(sensor_task, "drv_sensors", 3072, NULL, 5, NULL);
    return ESP_OK;
}

bool drv_sensors_door_open(void)  { return s_door.state; }
bool drv_sensors_pir_active(void) { return s_pir.state; }