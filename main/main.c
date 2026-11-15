#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "app_events.h"
#include "board.h"

static const char *TAG = "main";

/* 测试任务 1：每秒投递一个事件 */
static void producer_task(void *arg)
{
    int n = 0;
    while (1) {
        app_events_post(EVT_PIR_TRIGGER, n++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* 测试任务 2：等待事件并打印 */
static void consumer_task(void *arg)
{
    app_event_t evt;
    while (1) {
        if (app_events_wait(&evt, 3000)) {
            ESP_LOGI(TAG, "event id=%d arg=%d at %u ms",
                     (int)evt.id, (int)evt.arg, (unsigned)evt.time_ms);
        } else {
            ESP_LOGI(TAG, "no event for 3 s");
        }
    }
}

void app_main(void)
{
        ESP_LOGI(TAG, "LCD  : SCLK=%d MOSI=%d CS=%d DC=%d RST=%d BL=%d",
             PIN_LCD_SCLK, PIN_LCD_MOSI, PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_BL);
    ESP_LOGI(TAG, "I2S  : SPK(%d,%d,%d) MIC(%d,%d,%d)",
             PIN_SPK_BCLK, PIN_SPK_LRC, PIN_SPK_DIN, PIN_MIC_SCK, PIN_MIC_WS, PIN_MIC_SD);  
    ESP_ERROR_CHECK(app_events_init());
    xTaskCreate(consumer_task, "consumer", 3072, NULL, 5, NULL);
    xTaskCreate(producer_task, "producer", 2048, NULL, 5, NULL);
}