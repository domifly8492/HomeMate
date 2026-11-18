#pragma once
#include <stdint.h>
#include "lvgl.h"
#include "app_events.h"
#include <stdbool.h>

#define UI_COL_BG       0x0A0F1A
#define UI_COL_CARD     0x121A2B
#define UI_COL_TEXT     0xE8F1FF
#define UI_COL_DIM      0x8FA3BF
#define UI_COL_ACCENT   0x38D0FF
#define UI_COL_RED      0xFF5C6C
#define UI_COL_AMBER    0xFFB84D
#define UI_COL_PILL     0x1B2A44
#define UI_COL_DOT_OFF  0x2A3A58

#define UI_DEG "\xC2\xB0"       /* 度数符号 ° */

/* 步骤 2 做完后改成 1，主页会用真正的天气图标，不再是占位框 */
#define UI_WEATHER_ICON 1

typedef enum {
    UI_PAGE_HOME = 0,
    UI_PAGE_COUNT,
} ui_page_id_t;

/* 一个页面需要提供的三个函数 */
typedef struct {
    lv_obj_t *(*create)(void);                   /* 创建页面，返回它的屏幕对象 */
    void (*on_event)(const app_event_t *evt);    /* 收到事件，可为 NULL */
    void (*on_tick)(void);                       /* 每秒一次，可为 NULL */
} ui_page_t;

extern const ui_page_t ui_page_home;

lv_obj_t *ui_make_label(lv_obj_t *parent, const lv_font_t *font,
                        uint32_t color, const char *text);
void ui_goto(ui_page_id_t id);

/* 状态栏（放在 LVGL 顶层，换页不会消失） */
void ui_status_init(void);
void ui_status_set_page(int page, int count);
void ui_status_set_unread(int n);
void ui_status_on_event(const app_event_t *evt);
void ui_status_tick(void);
/* 提醒弹窗（放在 LVGL 顶层，盖在所有页面上） */
void ui_popup_on_event(const app_event_t *evt);

#if UI_WEATHER_ICON
lv_obj_t *ui_weather_icon_create(lv_obj_t *parent);
void ui_weather_icon_set(lv_obj_t *icon, int code, bool is_day);
#endif