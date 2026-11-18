#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "ui_internal.h"
#include "svc_net.h"
#include "svc_weather.h"

static lv_obj_t *s_time, *s_sec, *s_temp, *s_desc, *s_detail;
static bool s_have_weather = false;
#if UI_WEATHER_ICON
static lv_obj_t *s_icon;
static int s_icon_code = -1;
#endif

static void refresh_weather(void)
{
    svc_weather_t w;
    if (!svc_weather_get(&w)) {
        return;
    }
    char buf[64];

    snprintf(buf, sizeof(buf), "%d" UI_DEG "C", (int)lroundf(w.temp));
    lv_label_set_text(s_temp, buf);

    lv_label_set_text(s_desc, svc_weather_text(w.code));

    snprintf(buf, sizeof(buf), "Feels %d" UI_DEG "C\nHumidity %d%%\nWind %.1f m/s",
             (int)lroundf(w.feels), w.humi, w.wind);
    lv_label_set_text(s_detail, buf);

#if UI_WEATHER_ICON
    int key = w.code * 2 + (w.is_day ? 1 : 0);          /* 天气或昼夜变了才重画 */
    if (key != s_icon_code) {
        ui_weather_icon_set(s_icon, w.code, w.is_day != 0);
        s_icon_code = key;
    }
#endif
}

static lv_obj_t *page_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_COL_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(scr, false);

    /* 时间和秒 */
    s_time = ui_make_label(scr, &lv_font_montserrat_48, UI_COL_TEXT, "--:--");
    lv_obj_align(s_time, LV_ALIGN_TOP_LEFT, 16, 28);
    s_sec = ui_make_label(scr, &lv_font_montserrat_20, UI_COL_ACCENT, ":--");
    lv_obj_align(s_sec, LV_ALIGN_TOP_LEFT, 150, 52);

    /* 天气卡片 */
    lv_obj_t *card = lv_obj_create(scr);
    lv_obj_set_size(card, 296, 124);
    lv_obj_align(card, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(card, lv_color_hex(UI_COL_CARD), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_scrollable(card, false);

#if UI_WEATHER_ICON
    s_icon = ui_weather_icon_create(card);
    lv_obj_align(s_icon, LV_ALIGN_TOP_LEFT, 8, 4);
#else
    lv_obj_t *box = lv_obj_create(card);                /* 占位框 */
    lv_obj_set_size(box, 64, 64);
    lv_obj_align(box, LV_ALIGN_TOP_LEFT, 14, 10);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(UI_COL_ACCENT), 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_radius(box, 12, 0);
#endif

    s_desc = ui_make_label(card, &lv_font_montserrat_14, UI_COL_TEXT, "Loading...");
    lv_obj_align(s_desc, LV_ALIGN_TOP_LEFT, 8, 90);

    s_temp = ui_make_label(card, &lv_font_montserrat_48, UI_COL_TEXT, "--" UI_DEG "C");
    lv_obj_align(s_temp, LV_ALIGN_TOP_LEFT, 112, 6);

    s_detail = ui_make_label(card, &lv_font_montserrat_14, UI_COL_DIM, "");
    lv_obj_set_style_text_line_space(s_detail, 4, 0);
    lv_obj_align(s_detail, LV_ALIGN_TOP_LEFT, 116, 58);

    return scr;
}

static void page_tick(void)
{
    if (svc_net_time_valid()) {
        time_t now = time(NULL);
        struct tm t;
        localtime_r(&now, &t);
        lv_label_set_text_fmt(s_time, "%02d:%02d", t.tm_hour, t.tm_min);
        lv_label_set_text_fmt(s_sec, ":%02d", t.tm_sec);
    } else {
        lv_label_set_text(s_time, "--:--");
        lv_label_set_text(s_sec, ":--");
    }
    if (!s_have_weather) {
        refresh_weather();                  /* 数据到了就显示，不用等事件 */
    }
}

static void page_event(const app_event_t *evt)
{
    if (evt->id == EVT_WEATHER_UPDATED) {
        refresh_weather();
    }
}

const ui_page_t ui_page_home = {
    .create   = page_create,
    .on_event = page_event,
    .on_tick  = page_tick,
};