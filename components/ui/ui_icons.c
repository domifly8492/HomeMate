#include <stdbool.h>
#include "ui_internal.h"

#if UI_WEATHER_ICON
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "ui_icons";

#define ICON_N      80                          /* 每个图标 80x80 */
#define ICON_BYTES  (ICON_N * ICON_N * 4)       /* RGBA，每个像素 4 字节 */

extern const uint8_t icons_bin_start[] asm("_binary_weather_icons_bin_start");
extern const uint8_t icons_bin_end[]   asm("_binary_weather_icons_bin_end");

/* 图标在文件里的顺序，要和 weather_icons.bin 一致 */
typedef enum {
    ICON_SUN = 0,
    ICON_PARTLY,
    ICON_CLOUDY,
    ICON_FOG,
    ICON_LIGHT_RAIN,
    ICON_RAIN,
    ICON_SNOW,
    ICON_THUNDER,
    ICON_NIGHT,
    ICON_PARTLY_NIGHT,
    ICON_COUNT,
} icon_idx_t;

static int pick_icon(int code, bool is_day)
{
    switch (code) {
    case 0: case 1:
        return is_day ? ICON_SUN : ICON_NIGHT;
    case 2:
        return is_day ? ICON_PARTLY : ICON_PARTLY_NIGHT;
    case 3:
        return ICON_CLOUDY;
    case 45: case 48:
        return ICON_FOG;
    case 51: case 53: case 56: case 57:
    case 61: case 80:
        return ICON_LIGHT_RAIN;
    case 55: case 63: case 65: case 66: case 67:
    case 81: case 82:
        return ICON_RAIN;
    case 71: case 73: case 75: case 77:
    case 85: case 86:
        return ICON_SNOW;
    case 95: case 96: case 99:
        return ICON_THUNDER;
    default:
        return ICON_CLOUDY;
    }
}

lv_obj_t *ui_weather_icon_create(lv_obj_t *parent)
{
    size_t size = ICON_BYTES;                           /* ARGB8888 画布，宽 80 像素，每行 320 字节 */
    uint8_t *buf = heap_caps_aligned_calloc(64, 1, size, MALLOC_CAP_SPIRAM);
    if (buf == NULL) {
        buf = heap_caps_aligned_calloc(64, 1, size, MALLOC_CAP_8BIT);
    }
    if (buf == NULL) {
        return NULL;
    }
    lv_obj_t *cv = lv_canvas_create(parent);
    lv_canvas_set_buffer(cv, buf, ICON_N, ICON_N, LV_COLOR_FORMAT_ARGB8888);
    lv_canvas_fill_bg(cv, lv_color_black(), LV_OPA_TRANSP);
    return cv;
}

void ui_weather_icon_set(lv_obj_t *cv, int code, bool is_day)
{
    if (cv == NULL) {
        return;
    }
    int idx = pick_icon(code, is_day);
    if ((size_t)(idx + 1) * ICON_BYTES > (size_t)(icons_bin_end - icons_bin_start)) {
        ESP_LOGE(TAG, "icon file too small for icon %d", idx);
        return;
    }
    const uint8_t *p = icons_bin_start + (size_t)idx * ICON_BYTES;

    lv_canvas_fill_bg(cv, lv_color_black(), LV_OPA_TRANSP);
    for (int y = 0; y < ICON_N; y++) {
        for (int x = 0; x < ICON_N; x++) {
            const uint8_t *px = p + (y * ICON_N + x) * 4;      /* R G B A */
            if (px[3] != 0) {                                  /* 全透明的像素不用画 */
                lv_canvas_set_px(cv, x, y, lv_color_make(px[0], px[1], px[2]), px[3]);
            }
        }
    }
}
#endif