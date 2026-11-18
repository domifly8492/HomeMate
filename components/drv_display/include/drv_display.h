#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "lvgl.h"



/* 启动 LVGL 并绑定屏幕，要在 drv_display_init() 之后调用。
 * 启动之后不要再用上面的 fill 和 test_pattern，它们会和 LVGL 抢屏幕 */
lv_display_t *drv_display_lvgl_start(void);

/* 初始化 SPI 总线和 ST7789，横屏。成功后屏幕已经打开 */
esp_err_t drv_display_init(void);

/* 把 8 位 RGB 转成屏幕用的 565 格式（已处理字节顺序） */
uint16_t drv_display_color(uint8_t r, uint8_t g, uint8_t b);

/* 整屏填一种颜色 */
void drv_display_fill(uint16_t color);

/* 显示测试图案：红绿蓝白四条竖带，左上角有一个黑色方块，用来确认方向和颜色 */
void drv_display_test_pattern(void);