#include "drv_display.h"
#include "board.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_lvgl_port.h"

#define LCD_SPI_HOST   SPI2_HOST
#define STRIP_LINES    20                       /* 每次传 20 行 */

static const char *TAG = "drv_display";

static esp_lcd_panel_io_handle_t s_io    = NULL;
static esp_lcd_panel_handle_t    s_panel = NULL;
static SemaphoreHandle_t         s_done  = NULL;
static uint16_t                 *s_buf   = NULL;

/* 一段数据传完时由驱动调用，用来通知"可以复用缓冲区了" */
static bool on_trans_done(esp_lcd_panel_io_handle_t io,
                          esp_lcd_panel_io_event_data_t *edata, void *ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_done, &woken);
    return woken == pdTRUE;
}

/* 画一块区域，并等它传完 */
static void draw_block(int x0, int y0, int x1, int y1, const void *data)
{
    esp_lcd_panel_draw_bitmap(s_panel, x0, y0, x1, y1, data);   /* x1、y1 不包含 */
    xSemaphoreTake(s_done, pdMS_TO_TICKS(1000));
}

esp_err_t drv_display_init(void)
{
    s_done = xSemaphoreCreateBinary();
    s_buf  = heap_caps_malloc(LCD_W * STRIP_LINES * sizeof(uint16_t),
                              MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (s_done == NULL || s_buf == NULL) {
        return ESP_ERR_NO_MEM;
    }

    /* 1. SPI 总线 */
    spi_bus_config_t bus = {
        .sclk_io_num     = PIN_LCD_SCLK,
        .mosi_io_num     = PIN_LCD_MOSI,
        .miso_io_num     = -1,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = LCD_W * STRIP_LINES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO));

    /* 2. 面板 IO：CS、DC、速度、命令位数 */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num       = PIN_LCD_DC,
        .cs_gpio_num       = PIN_LCD_CS,
        .pclk_hz           = LCD_SPI_HZ,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST,
                                             &io_cfg, &s_io));

    esp_lcd_panel_io_callbacks_t cbs = { .on_color_trans_done = on_trans_done };
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(s_io, &cbs, NULL));

    /* 3. ST7789 面板 */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order  = LCD_BGR ? LCD_RGB_ELEMENT_ORDER_BGR : LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(s_io, &panel_cfg, &s_panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, LCD_INVERT));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, LCD_SWAP_XY));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, LCD_MIRROR_X, LCD_MIRROR_Y));

    drv_display_fill(0x0000);                     /* 先清成黑色，再打开显示 */
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    ESP_LOGI(TAG, "ST7789 ready: %dx%d, SPI %d MHz", LCD_W, LCD_H, LCD_SPI_HZ / 1000000);
    return ESP_OK;
}

uint16_t drv_display_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return (uint16_t)((c >> 8) | (c << 8));       /* SPI 先发高字节，所以要交换 */
}

void drv_display_fill(uint16_t color)
{
    for (int i = 0; i < LCD_W * STRIP_LINES; i++) {
        s_buf[i] = color;
    }
    for (int y = 0; y < LCD_H; y += STRIP_LINES) {
        draw_block(0, y, LCD_W, y + STRIP_LINES, s_buf);
    }
}

void drv_display_test_pattern(void)
{
    const uint16_t bands[4] = {
        drv_display_color(255, 0, 0),
        drv_display_color(0, 255, 0),
        drv_display_color(0, 0, 255),
        drv_display_color(255, 255, 255),
    };
    for (int y = 0; y < LCD_H; y += STRIP_LINES) {
        for (int r = 0; r < STRIP_LINES; r++) {
            for (int x = 0; x < LCD_W; x++) {
                uint16_t c = bands[x / (LCD_W / 4)];
                if (y + r < 24 && x < 24) {
                    c = 0x0000;                   /* 左上角的黑色方块 */
                }
                s_buf[r * LCD_W + x] = c;
            }
        }
        draw_block(0, y, LCD_W, y + STRIP_LINES, s_buf);
    }
}

lv_display_t *drv_display_lvgl_start(void)
{
    /* 1. 启动 LVGL 和它的任务 */
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    /* 2. 把我们已经初始化好的屏幕交给 LVGL */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = s_io,
        .panel_handle  = s_panel,
        .buffer_size   = LCD_W * 40,        /* 每个缓冲区 40 行，单位是像素 */
        .double_buffer = true,
        .hres          = LCD_W,
        .vres          = LCD_H,
        .monochrome    = false,
        .rotation = {                        /* 要和上面面板的设置一致 */
            .swap_xy  = LCD_SWAP_XY,
            .mirror_x = LCD_MIRROR_X,
            .mirror_y = LCD_MIRROR_Y,
        },
        .flags = {
            .buff_dma   = true,
            .swap_bytes = true,              /* SPI 先发高字节，LVGL 要交换字节序 */
        },
    };
    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (disp == NULL) {
        ESP_LOGE(TAG, "lvgl_port_add_disp failed");
    }
    return disp;
}