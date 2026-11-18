#pragma once
#include <stdbool.h>
#include "esp_err.h"

/* 启动传感器扫描任务：PIR、门磁、录音键。状态变化时通过事件通知。
 * 要在 app_events_init() 之后调用 */
esp_err_t drv_sensors_start(void);

bool drv_sensors_door_open(void);       /* 门现在是不是开着 */
bool drv_sensors_pir_active(void);      /* PIR 现在是不是检测到人 */