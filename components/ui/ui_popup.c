#include "ui_internal.h"
#include "svc_rules.h"

#define POPUP_MS  8000                  /* 弹窗停留 8 秒 */

static lv_obj_t   *s_mask;              /* 把背景压暗的蒙层 */
static lv_obj_t   *s_card;              /* 提醒卡片 */
static lv_obj_t   *s_title;
static lv_obj_t   *s_detail;
static lv_obj_t   *s_bar;               /* 倒计时条 */
static lv_timer_t *s_hide_timer;        /* 时间到了自动关闭 */

/* LVGL 的动画就是：在一段时间里让一个数值从 A 变到 B，每一帧调用一次下面的函数 */
static void anim_set_y(void *obj, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)obj, v);
}
static void anim_set_bar(void *obj, int32_t v)
{
    lv_bar_set_value((lv_obj_t *)obj, v, LV_ANIM_OFF);
}

static void hide_cb(lv_timer_t *t)
{
    s_hide_timer = NULL;                /* 一次性定时器执行完会被 LVGL 自动删除，指针要清空 */
    lv_obj_add_flag(s_mask, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_card, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *centered_label(lv_obj_t *parent, const lv_font_t *font,
                                uint32_t color, int y)
{
    lv_obj_t *l = ui_make_label(parent, font, color, "");
    lv_obj_set_width(l, 240);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, y);
    return l;
}

/* 第一次要弹窗时才创建控件，之后重复使用 */
static void popup_create(void)
{
    lv_obj_t *top = lv_layer_top();     /* 顶层：盖在所有页面上面，换页也不会消失 */

    /* 蒙层：全屏的黑色，60% 不透明 */
    s_mask = lv_obj_create(top);
    lv_obj_set_size(s_mask, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(s_mask, 0, 0);
    lv_obj_set_style_bg_color(s_mask, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_mask, LV_OPA_60, 0);
    lv_obj_set_style_border_width(s_mask, 0, 0);
    lv_obj_set_style_radius(s_mask, 0, 0);
    lv_obj_set_scrollable(s_mask, false);

    /* 卡片：琥珀色边框的圆角矩形 */
    s_card = lv_obj_create(top);
    lv_obj_set_size(s_card, 260, 150);
    lv_obj_align(s_card, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_style_bg_color(s_card, lv_color_hex(UI_COL_CARD), 0);
    lv_obj_set_style_border_color(s_card, lv_color_hex(UI_COL_AMBER), 0);
    lv_obj_set_style_border_width(s_card, 3, 0);
    lv_obj_set_style_radius(s_card, 16, 0);
    lv_obj_set_style_pad_all(s_card, 0, 0);
    lv_obj_set_scrollable(s_card, false);

    lv_obj_t *bell = ui_make_label(s_card, &lv_font_montserrat_48,
                                   UI_COL_AMBER, LV_SYMBOL_BELL);
    lv_obj_align(bell, LV_ALIGN_TOP_MID, 0, 6);

    s_title  = centered_label(s_card, &lv_font_montserrat_20, UI_COL_TEXT, 68);
    s_detail = centered_label(s_card, &lv_font_montserrat_14, UI_COL_DIM, 98);

    /* 倒计时条：从满慢慢减到空 */
    s_bar = lv_bar_create(s_card);
    lv_obj_set_size(s_bar, 210, 6);
    lv_obj_align(s_bar, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_bar_set_range(s_bar, 0, 100);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(UI_COL_DOT_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(UI_COL_AMBER), LV_PART_INDICATOR);

    lv_obj_add_flag(s_mask, LV_OBJ_FLAG_HIDDEN);        /* 先藏起来 */
    lv_obj_add_flag(s_card, LV_OBJ_FLAG_HIDDEN);
}

static void popup_show(const char *title, const char *detail)
{
    if (s_mask == NULL) {
        popup_create();
    }
    lv_label_set_text(s_title, title);
    lv_label_set_text(s_detail, detail);
    lv_obj_remove_flag(s_mask, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_card, LV_OBJ_FLAG_HIDDEN);

    /* 动画 1：卡片从屏幕上方滑进来，最后带一点"弹一下"的效果 */
    lv_anim_delete(s_card, NULL);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_card);
    lv_anim_set_exec_cb(&a, anim_set_y);
    lv_anim_set_values(&a, -170, 45);
    lv_anim_set_duration(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_start(&a);

    /* 动画 2：倒计时条在 POPUP_MS 内匀速从 100 减到 0 */
    lv_anim_delete(s_bar, NULL);
    lv_bar_set_value(s_bar, 100, LV_ANIM_OFF);
    lv_anim_t b;
    lv_anim_init(&b);
    lv_anim_set_var(&b, s_bar);
    lv_anim_set_exec_cb(&b, anim_set_bar);
    lv_anim_set_values(&b, 100, 0);
    lv_anim_set_duration(&b, POPUP_MS);
    lv_anim_set_path_cb(&b, lv_anim_path_linear);
    lv_anim_start(&b);

    /* 到时间自动关闭；新的提醒来了，重新计时 */
    if (s_hide_timer != NULL) {
        lv_timer_delete(s_hide_timer);
    }
    s_hide_timer = lv_timer_create(hide_cb, POPUP_MS, NULL);
    lv_timer_set_repeat_count(s_hide_timer, 1);
}

/* 收到事件时由 ui_manager 调用。界面自己不决定提醒什么，
 * 只拿到编号，再向规则模块要标题和说明文字 */
void ui_popup_on_event(const app_event_t *evt)
{
    if (evt->id != EVT_REMINDER) {
        return;
    }
    reminder_id_t id = (reminder_id_t)evt->arg;
    char detail[48];
    svc_rules_detail(id, detail, sizeof(detail));
    popup_show(svc_rules_title(id), detail);
}