#include <stdio.h>
#include <time.h>
#include "ui_internal.h"
#include "svc_net.h"

#define MAX_DOTS 8

static lv_obj_t *s_date;
static lv_obj_t *s_pill;
static lv_obj_t *s_pill_lbl;
static lv_obj_t *s_wifi;
static lv_obj_t *s_dots[MAX_DOTS];
static int s_dot_count = 0;
static int s_unread = 0;

void ui_status_init(void)
{
    lv_obj_t *top = lv_layer_top();

    /* 日期 */
    s_date = ui_make_label(top, &lv_font_montserrat_14, UI_COL_DIM, "--");
    lv_obj_align(s_date, LV_ALIGN_TOP_LEFT, 12, 6);

    /* 未读留言角标，没有未读就隐藏 */
    s_pill = lv_obj_create(top);
    lv_obj_set_size(s_pill, 50, 22);
    lv_obj_align(s_pill, LV_ALIGN_TOP_LEFT, 122, 4);
    lv_obj_set_style_bg_color(s_pill, lv_color_hex(UI_COL_PILL), 0);
    lv_obj_set_style_bg_opa(s_pill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_pill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_pill, 0, 0);
    lv_obj_set_style_pad_all(s_pill, 0, 0);
    lv_obj_set_scrollable(s_pill, false);
    s_pill_lbl = ui_make_label(s_pill, &lv_font_montserrat_14, UI_COL_TEXT,
                               LV_SYMBOL_ENVELOPE " 0");
    lv_obj_center(s_pill_lbl);
    lv_obj_add_flag(s_pill, LV_OBJ_FLAG_HIDDEN);

    /* Wi-Fi 图标：连上是青色，没连上是暗色 */
    s_wifi = ui_make_label(top, &lv_font_montserrat_14, UI_COL_DOT_OFF, LV_SYMBOL_WIFI);
    lv_obj_align(s_wifi, LV_ALIGN_TOP_RIGHT, -12, 6);
}

void ui_status_set_page(int page, int count)
{
    lv_obj_t *top = lv_layer_top();
    if (count > MAX_DOTS) {
        count = MAX_DOTS;
    }
    while (s_dot_count < count) {
        lv_obj_t *d = lv_obj_create(top);
        lv_obj_set_size(d, 5, 5);
        lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(d, 0, 0);
        lv_obj_set_style_pad_all(d, 0, 0);
        lv_obj_set_scrollable(d, false);
        s_dots[s_dot_count++] = d;
    }
    for (int i = 0; i < s_dot_count; i++) {
        lv_obj_align(s_dots[i], LV_ALIGN_BOTTOM_MID, (2 * i - (count - 1)) * 5, -3);
        lv_obj_set_style_bg_color(s_dots[i],
            lv_color_hex(i == page ? UI_COL_ACCENT : UI_COL_DOT_OFF), 0);
    }
}

void ui_status_set_unread(int n)
{
    s_unread = n;
    if (n > 0) {
        lv_label_set_text_fmt(s_pill_lbl, LV_SYMBOL_ENVELOPE " %d", n);
        lv_obj_center(s_pill_lbl);
        lv_obj_remove_flag(s_pill, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_pill, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_status_tick(void)
{
    char buf[24];
    if (svc_net_time_valid()) {
        time_t now = time(NULL);
        struct tm t;
        localtime_r(&now, &t);
        strftime(buf, sizeof(buf), "%m-%d %a", &t);
    } else {
        snprintf(buf, sizeof(buf), "--");
    }
    lv_label_set_text(s_date, buf);

    lv_obj_set_style_text_color(s_wifi,
        lv_color_hex(svc_net_is_connected() ? UI_COL_ACCENT : UI_COL_DOT_OFF), 0);
}

void ui_status_on_event(const app_event_t *evt)
{
    switch (evt->id) {
    case EVT_NEW_MESSAGE:
        ui_status_set_unread(s_unread + 1);
        break;
    case EVT_WIFI_CONNECTED:
    case EVT_WIFI_LOST:
    case EVT_TIME_SYNCED:
        ui_status_tick();                       /* 有变化就马上刷新，不用等下一秒 */
        break;
    default:
        break;
    }
}