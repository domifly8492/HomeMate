#pragma once

/* ============================================================
 * 板级配置：所有引脚和硬件参数都在这里。
 * 引脚用整数写，没接的写 -1。接线有变动只改这个文件。
 * ============================================================ */

/* ---------- 屏幕：2.4 寸 ST7789，SPI ---------- */
#define PIN_LCD_SCLK     12     /* 模块上标 SCL，是 SPI 时钟 */
#define PIN_LCD_MOSI     11     /* 模块上标 SDA，是 SPI 数据 */
#define PIN_LCD_CS       10
#define PIN_LCD_DC       9
#define PIN_LCD_RST      8
#define PIN_LCD_BL       (-1)   /* 背光直接接 3.3V，不占引脚 */

/* 屏幕原始分辨率是竖屏 240x320，本项目横着用 */
#define LCD_NATIVE_W     240
#define LCD_NATIVE_H     320
#define LCD_W            320    /* 横屏后的宽 */
#define LCD_H            240    /* 横屏后的高 */

/* 方向和颜色调节：点亮后如果方向或颜色不对，只改这几个 */
#define LCD_SWAP_XY      1      /* 横屏必须交换 XY */
#define LCD_MIRROR_X     0
#define LCD_MIRROR_Y     1
#define LCD_INVERT       1      /* ST7789 大多数模块需要反色 */
#define LCD_BGR          0      /* 红蓝颠倒就改成 1 */

#define LCD_SPI_HZ       (20 * 1000 * 1000)   /* 先用 20MHz，稳定后再提高 */

/* ---------- 喇叭：MAX98357A，I2S ---------- */
#define PIN_SPK_BCLK     5
#define PIN_SPK_LRC      4
#define PIN_SPK_DIN      6
#define PIN_SPK_SD       (-1)   /* 功放使能脚，没接就写 -1 */

/* ---------- 麦克风：INMP441，I2S ---------- */
#define PIN_MIC_SCK      4
#define PIN_MIC_WS       5
#define PIN_MIC_SD       6      /* L/R 脚接地，取左声道 */

/* ---------- 音频格式：喇叭和麦克风统一用这一种 ---------- */
#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_BITS          16

/* ---------- 传感器和按键 ---------- */
#define PIN_PIR          1      /* 人体红外，有人时输出高电平 */
#define PIN_DOOR         2      /* 门磁，门关上时接通，用内部上拉 */
#define PIN_BTN_RECORD   18     /* 录音键，按下接地 */

/* ============================================================
 * 编译期检查：用到了不能用的引脚，直接编译报错
 * 不能用：22~25（S3 上不存在）、19/20（USB）、26~37（Flash 和 PSRAM）、
 *         43/44（串口日志）
 * ============================================================ */
#define BOARD_PIN_BAD(p) ( ((p) >= 22 && (p) <= 37) || (p) == 19 || (p) == 20 || \
                           (p) == 43 || (p) == 44 || (p) > 48 )

#define BOARD_CHECK_PIN(p) \
    _Static_assert((p) < 0 || !BOARD_PIN_BAD(p), "board.h: reserved or invalid GPIO: " #p)

BOARD_CHECK_PIN(PIN_LCD_SCLK);
BOARD_CHECK_PIN(PIN_LCD_MOSI);
BOARD_CHECK_PIN(PIN_LCD_CS);
BOARD_CHECK_PIN(PIN_LCD_DC);
BOARD_CHECK_PIN(PIN_LCD_RST);
BOARD_CHECK_PIN(PIN_LCD_BL);
BOARD_CHECK_PIN(PIN_SPK_BCLK);
BOARD_CHECK_PIN(PIN_SPK_LRC);
BOARD_CHECK_PIN(PIN_SPK_DIN);
BOARD_CHECK_PIN(PIN_SPK_SD);
BOARD_CHECK_PIN(PIN_MIC_SCK);
BOARD_CHECK_PIN(PIN_MIC_WS);
BOARD_CHECK_PIN(PIN_MIC_SD);
BOARD_CHECK_PIN(PIN_PIR);
BOARD_CHECK_PIN(PIN_DOOR);
BOARD_CHECK_PIN(PIN_BTN_RECORD);