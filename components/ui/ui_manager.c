#include "ui.h"
#include "ui_internal.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

static const char *TAG = "ui";

static const ui_page_t *const PAGES[UI_PAGE_COUNT] = { &ui_page_home };
static lv_obj_t *s_screens[UI_PAGE_COUNT];
static int s_cur = -1;
static app_events_sub_t s_sub;

lv_obj_t *ui_make_label(lv_obj_t *parent, const lv_font_t *font,
                        uint32_t color, const char *text)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_label_set_text(l, text);
    return l;
}

void ui_goto(ui_page_id_t id)
{
    if (id >= UI_PAGE_COUNT) {
        return;
    }
    if (s_screens[id] == NULL) {
        s_screens[id] = PAGES[id]->create();
    }
    if (s_cur < 0) {
        lv_screen_load(s_screens[id]);                  /* 第一页不要动画 */
    } else {
        lv_screen_load_anim(s_screens[id], LV_SCREEN_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
    }
    s_cur = id;
    ui_status_set_page(id, UI_PAGE_COUNT);
}

/* LVGL 定时器，每 100ms 一次。它在 LVGL 任务里运行，已经在锁里，不用再加锁 */
static void ui_timer_cb(lv_timer_t *t)
{
    static int ticks = 0;
    app_event_t evt;

    while (app_events_wait(s_sub, &evt, 0)) {           /* 等待时间 0：没事件就立刻返回 */
        ui_status_on_event(&evt);
        ui_popup_on_event(&evt);
        if (s_cur >= 0 && PAGES[s_cur]->on_event) {
            PAGES[s_cur]->on_event(&evt);
        }
    }
    if (++ticks >= 10) {                                /* 10 次就是 1 秒 */
        ticks = 0;
        ui_status_tick();
        if (s_cur >= 0 && PAGES[s_cur]->on_tick) {
            PAGES[s_cur]->on_tick();
        }
    }
}

esp_err_t ui_init(void)
{
    s_sub = app_events_subscribe(16);       /* 要在 app_events_init() 之后调用 */
    if (s_sub == NULL) {
        return ESP_FAIL;
    }

    lvgl_port_lock(0);
    ui_status_init();
    ui_goto(UI_PAGE_HOME);
    lv_timer_create(ui_timer_cb, 100, NULL);
    lvgl_port_unlock();

    ESP_LOGI(TAG, "ui ready");
    return ESP_OK;
}