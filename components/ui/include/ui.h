#pragma once
#include "esp_err.h"

/* 创建所有界面控件。要在 drv_display_lvgl_start() 之后调用 */
esp_err_t ui_init(void);