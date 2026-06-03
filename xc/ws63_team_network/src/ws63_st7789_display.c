#include "ws63_st7789_display.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "common_def.h"
#include "errcode.h"
#include "gpio.h"
#include "pinctrl.h"
#include "securec.h"
#include "soc_osal.h"
#include "spi.h"
#include "tcxo.h"

#if defined(__GNUC__)
#define ST7789_UNUSED_FUNC __attribute__((unused))
#else
#define ST7789_UNUSED_FUNC
#endif

#ifndef CONFIG_SLE_TEAM_DISPLAY_USE_LVGL
#define CONFIG_SLE_TEAM_DISPLAY_USE_LVGL 1
#endif

#ifndef CONFIG_SLE_TEAM_LVGL_DRAW_BUF_LINES
#define CONFIG_SLE_TEAM_LVGL_DRAW_BUF_LINES 8
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_WIDTH
#define CONFIG_SLE_TEAM_ST7789_WIDTH 135
#endif

#if CONFIG_SLE_TEAM_DISPLAY_USE_LVGL
#if defined(__has_include)
#if __has_include("lvgl.h")
#include "lvgl.h"
#define SLE_TEAM_LVGL_HEADER_FOUND 1
#elif __has_include("lvgl/lvgl.h")
#include "lvgl/lvgl.h"
#define SLE_TEAM_LVGL_HEADER_FOUND 1
#else
#define SLE_TEAM_LVGL_HEADER_FOUND 0
#endif
#else
#include "lvgl.h"
#define SLE_TEAM_LVGL_HEADER_FOUND 1
#endif
#else
#define SLE_TEAM_LVGL_HEADER_FOUND 0
#endif

#if CONFIG_SLE_TEAM_DISPLAY_USE_LVGL && SLE_TEAM_LVGL_HEADER_FOUND
#define SLE_TEAM_USE_LVGL_BACKEND 1
#else
#define SLE_TEAM_USE_LVGL_BACKEND 0
#endif

#define ST7789_SPI_SLAVE_NUM 1
#define ST7789_SPI_FREQ_MHZ 8
#define ST7789_SPI_WAIT_CYCLES 0x10
#define ST7789_SPI_TIMEOUT 0xFFFFFFFFU
#define ST7789_SOFT_SPI_ENABLE 1
#define ST7789_SOFT_SPI_MODE3 0
#define ST7789_SOFT_SPI_DELAY_CYCLES 12U
#define ST7789_FULL_INIT_SEQ_ENABLE 1
#define ST7789_MADCTL_DEFAULT 0x60U
#define ST7789_COLOR_BLACK 0x0000U
#define ST7789_COLOR_WHITE 0xFFFFU
#define ST7789_COLOR_RED 0xF800U
#define ST7789_COLOR_GREEN 0x07E0U
#define ST7789_COLOR_YELLOW 0xFFE0U
#define ST7789_MAX_TEXT_LEN 32U
#define ST7789_FONT6X8_FIRST 32U
#define ST7789_FONT6X8_COUNT ((uint8_t)(sizeof(g_st7789_font6x8) / sizeof(g_st7789_font6x8[0])))
#define ST7789_FONT6X8_WIDTH 6U
#define ST7789_FONT6X8_HEIGHT 8U
#define ST7789_TEXT_ADVANCE_X 7U
#define ST7789_LVGL_TICK_FALLBACK_MS 5U

static ws63_st7789_config_t g_st7789_cfg;
static uint8_t g_st7789_ready;
static uint32_t g_st7789_last_tick_ms;
static uint8_t g_st7789_status_cache_valid;
static uint8_t g_st7789_last_online_count;
static uint8_t g_st7789_last_offline_count;
static uint8_t g_st7789_last_alert_count;
static char g_st7789_last_role[16];
static char g_st7789_last_self[16];
static char g_st7789_last_fw[24];
static void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

#if SLE_TEAM_USE_LVGL_BACKEND
#if LVGL_VERSION_MAJOR >= 9
static lv_display_t *g_st7789_lv_display;
#else
static lv_disp_t *g_st7789_lv_display;
static lv_disp_draw_buf_t g_st7789_lv_draw_buf;
static lv_disp_drv_t g_st7789_lv_disp_drv;
#endif
static lv_obj_t *g_st7789_lv_label_title;
static lv_obj_t *g_st7789_lv_label_status;
static lv_obj_t *g_st7789_lv_label_alert;
static lv_color_t *g_st7789_lv_buf1;
static uint32_t g_st7789_lv_buf_px_count;
static uint8_t g_st7789_lv_ready;
#endif

/*
 * 6x8 ASCII font from the BearPi SDK SSD1306 sample, kept local so this
 * ST7789 module can be copied as a self-contained v4 board adapter.
 */
static const uint8_t g_st7789_font6x8[][6] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x2f, 0x00, 0x00},
    {0x00, 0x00, 0x07, 0x00, 0x07, 0x00},
    {0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14},
    {0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12},
    {0x00, 0x62, 0x64, 0x08, 0x13, 0x23},
    {0x00, 0x36, 0x49, 0x55, 0x22, 0x50},
    {0x00, 0x00, 0x05, 0x03, 0x00, 0x00},
    {0x00, 0x00, 0x1c, 0x22, 0x41, 0x00},
    {0x00, 0x00, 0x41, 0x22, 0x1c, 0x00},
    {0x00, 0x14, 0x08, 0x3e, 0x08, 0x14},
    {0x00, 0x08, 0x08, 0x3e, 0x08, 0x08},
    {0x00, 0x00, 0x00, 0xa0, 0x60, 0x00},
    {0x00, 0x08, 0x08, 0x08, 0x08, 0x08},
    {0x00, 0x00, 0x60, 0x60, 0x00, 0x00},
    {0x00, 0x20, 0x10, 0x08, 0x04, 0x02},
    {0x00, 0x3e, 0x51, 0x49, 0x45, 0x3e},
    {0x00, 0x00, 0x42, 0x7f, 0x40, 0x00},
    {0x00, 0x42, 0x61, 0x51, 0x49, 0x46},
    {0x00, 0x21, 0x41, 0x45, 0x4b, 0x31},
    {0x00, 0x18, 0x14, 0x12, 0x7f, 0x10},
    {0x00, 0x27, 0x45, 0x45, 0x45, 0x39},
    {0x00, 0x3c, 0x4a, 0x49, 0x49, 0x30},
    {0x00, 0x01, 0x71, 0x09, 0x05, 0x03},
    {0x00, 0x36, 0x49, 0x49, 0x49, 0x36},
    {0x00, 0x06, 0x49, 0x49, 0x29, 0x1e},
    {0x00, 0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x00, 0x56, 0x36, 0x00, 0x00},
    {0x00, 0x08, 0x14, 0x22, 0x41, 0x00},
    {0x00, 0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x00, 0x41, 0x22, 0x14, 0x08},
    {0x00, 0x02, 0x01, 0x51, 0x09, 0x06},
    {0x00, 0x32, 0x49, 0x59, 0x51, 0x3e},
    {0x00, 0x7c, 0x12, 0x11, 0x12, 0x7c},
    {0x00, 0x7f, 0x49, 0x49, 0x49, 0x36},
    {0x00, 0x3e, 0x41, 0x41, 0x41, 0x22},
    {0x00, 0x7f, 0x41, 0x41, 0x22, 0x1c},
    {0x00, 0x7f, 0x49, 0x49, 0x49, 0x41},
    {0x00, 0x7f, 0x09, 0x09, 0x09, 0x01},
    {0x00, 0x3e, 0x41, 0x49, 0x49, 0x7a},
    {0x00, 0x7f, 0x08, 0x08, 0x08, 0x7f},
    {0x00, 0x00, 0x41, 0x7f, 0x41, 0x00},
    {0x00, 0x20, 0x40, 0x41, 0x3f, 0x01},
    {0x00, 0x7f, 0x08, 0x14, 0x22, 0x41},
    {0x00, 0x7f, 0x40, 0x40, 0x40, 0x40},
    {0x00, 0x7f, 0x02, 0x0c, 0x02, 0x7f},
    {0x00, 0x7f, 0x04, 0x08, 0x10, 0x7f},
    {0x00, 0x3e, 0x41, 0x41, 0x41, 0x3e},
    {0x00, 0x7f, 0x09, 0x09, 0x09, 0x06},
    {0x00, 0x3e, 0x41, 0x51, 0x21, 0x5e},
    {0x00, 0x7f, 0x09, 0x19, 0x29, 0x46},
    {0x00, 0x46, 0x49, 0x49, 0x49, 0x31},
    {0x00, 0x01, 0x01, 0x7f, 0x01, 0x01},
    {0x00, 0x3f, 0x40, 0x40, 0x40, 0x3f},
    {0x00, 0x1f, 0x20, 0x40, 0x20, 0x1f},
    {0x00, 0x3f, 0x40, 0x38, 0x40, 0x3f},
    {0x00, 0x63, 0x14, 0x08, 0x14, 0x63},
    {0x00, 0x07, 0x08, 0x70, 0x08, 0x07},
    {0x00, 0x61, 0x51, 0x49, 0x45, 0x43},
    {0x00, 0x00, 0x7f, 0x41, 0x41, 0x00},
    {0x00, 0x55, 0x2a, 0x55, 0x2a, 0x55},
    {0x00, 0x00, 0x41, 0x41, 0x7f, 0x00},
    {0x00, 0x04, 0x02, 0x01, 0x02, 0x04},
    {0x00, 0x40, 0x40, 0x40, 0x40, 0x40},
    {0x00, 0x00, 0x01, 0x02, 0x04, 0x00},
    {0x00, 0x20, 0x54, 0x54, 0x54, 0x78},
    {0x00, 0x7f, 0x48, 0x44, 0x44, 0x38},
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x20},
    {0x00, 0x38, 0x44, 0x44, 0x48, 0x7f},
    {0x00, 0x38, 0x54, 0x54, 0x54, 0x18},
    {0x00, 0x08, 0x7e, 0x09, 0x01, 0x02},
    {0x00, 0x18, 0xa4, 0xa4, 0xa4, 0x7c},
    {0x00, 0x7f, 0x08, 0x04, 0x04, 0x78},
    {0x00, 0x00, 0x44, 0x7d, 0x40, 0x00},
    {0x00, 0x40, 0x80, 0x84, 0x7d, 0x00},
    {0x00, 0x7f, 0x10, 0x28, 0x44, 0x00},
    {0x00, 0x00, 0x41, 0x7f, 0x40, 0x00},
    {0x00, 0x7c, 0x04, 0x18, 0x04, 0x78},
    {0x00, 0x7c, 0x08, 0x04, 0x04, 0x78},
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x38},
    {0x00, 0xfc, 0x24, 0x24, 0x24, 0x18},
    {0x00, 0x18, 0x24, 0x24, 0x18, 0xfc},
    {0x00, 0x7c, 0x08, 0x04, 0x04, 0x08},
    {0x00, 0x48, 0x54, 0x54, 0x54, 0x20},
    {0x00, 0x04, 0x3f, 0x44, 0x40, 0x20},
    {0x00, 0x3c, 0x40, 0x40, 0x20, 0x7c},
    {0x00, 0x1c, 0x20, 0x40, 0x20, 0x1c},
    {0x00, 0x3c, 0x40, 0x30, 0x40, 0x3c},
    {0x00, 0x44, 0x28, 0x10, 0x28, 0x44},
    {0x00, 0x1c, 0xa0, 0xa0, 0xa0, 0x7c},
    {0x00, 0x44, 0x64, 0x54, 0x4c, 0x44},
    {0x14, 0x14, 0x14, 0x14, 0x14, 0x14},
};

static void st7789_cs(uint8_t selected)
{
    (void)uapi_gpio_set_val(g_st7789_cfg.cs_pin, selected != 0U ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH);
}

static void st7789_dc(uint8_t data)
{
    (void)uapi_gpio_set_val(g_st7789_cfg.dc_pin, data != 0U ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
}

#if ST7789_SOFT_SPI_ENABLE
static void st7789_sclk(uint8_t high)
{
    (void)uapi_gpio_set_val(g_st7789_cfg.sclk_pin, high != 0U ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
}

static void st7789_mosi(uint8_t high)
{
    (void)uapi_gpio_set_val(g_st7789_cfg.mosi_pin, high != 0U ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
}

static void st7789_spi_idle_level(void)
{
#if ST7789_SOFT_SPI_MODE3
    st7789_sclk(1U);
#else
    st7789_sclk(0U);
#endif
}

static void st7789_soft_spi_delay(void)
{
    volatile uint32_t n;

    for (n = 0U; n < ST7789_SOFT_SPI_DELAY_CYCLES; n++) {
    }
}

static void st7789_soft_spi_write_u8(uint8_t byte)
{
    uint8_t bit;

    for (bit = 0U; bit < 8U; bit++) {
#if ST7789_SOFT_SPI_MODE3
        st7789_sclk(0U);
        st7789_soft_spi_delay();
        st7789_mosi((byte & 0x80U) != 0U ? 1U : 0U);
        st7789_soft_spi_delay();
        st7789_sclk(1U);
        st7789_soft_spi_delay();
#else
        st7789_sclk(0U);
        st7789_soft_spi_delay();
        st7789_mosi((byte & 0x80U) != 0U ? 1U : 0U);
        st7789_soft_spi_delay();
        st7789_sclk(1U);
        st7789_soft_spi_delay();
        st7789_sclk(0U);
        st7789_soft_spi_delay();
#endif
        byte <<= 1U;
    }
}
#endif

static int st7789_write(const uint8_t *buf, uint32_t len)
{
#if ST7789_SOFT_SPI_ENABLE
    uint32_t i;
#else
    spi_xfer_data_t data = {0};
    errcode_t ret;
#endif

    if (buf == NULL || len == 0U) {
        return -1;
    }
#if ST7789_SOFT_SPI_ENABLE
    st7789_cs(1U);
    for (i = 0U; i < len; i++) {
        st7789_soft_spi_write_u8(buf[i]);
    }
    st7789_spi_idle_level();
    st7789_cs(0U);
    return 0;
#else
    data.tx_buff = (uint8_t *)buf;
    data.tx_bytes = len;
    st7789_cs(1U);
    ret = uapi_spi_master_write(g_st7789_cfg.spi_bus, &data, ST7789_SPI_TIMEOUT);
    st7789_cs(0U);
    return ret == ERRCODE_SUCC ? 0 : -1;
#endif
}

static int st7789_cmd(uint8_t cmd)
{
    st7789_dc(0U);
    return st7789_write(&cmd, 1U);
}

static int st7789_data(const uint8_t *buf, uint32_t len)
{
    st7789_dc(1U);
    return st7789_write(buf, len);
}

static int st7789_data_u8(uint8_t value) ST7789_UNUSED_FUNC;

static int st7789_data_u8(uint8_t value)
{
    return st7789_data(&value, 1U);
}

static int st7789_cmd_data(uint8_t cmd, const uint8_t *data, uint32_t len)
{
    if (st7789_cmd(cmd) != 0) {
        return -1;
    }
    if (data == NULL || len == 0U) {
        return 0;
    }
    return st7789_data(data, len);
}

static void st7789_set_view_config(uint8_t madctl, uint16_t x_off, uint16_t y_off) ST7789_UNUSED_FUNC;

static void st7789_set_view_config(uint8_t madctl, uint16_t x_off, uint16_t y_off)
{
    g_st7789_cfg.x_offset = x_off;
    g_st7789_cfg.y_offset = y_off;
    (void)st7789_cmd_data(0x36U, &madctl, 1U);
    st7789_fill_rect(0U, 0U, g_st7789_cfg.width, g_st7789_cfg.height, ST7789_COLOR_BLACK);
    osal_printk("[display] view madctl=0x%02X off=%u,%u\r\n",
        madctl, (uint32_t)g_st7789_cfg.x_offset, (uint32_t)g_st7789_cfg.y_offset);
}

static int st7789_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];
    uint16_t panel_x0 = (uint16_t)(x0 + g_st7789_cfg.x_offset);
    uint16_t panel_x1 = (uint16_t)(x1 + g_st7789_cfg.x_offset);
    uint16_t panel_y0 = (uint16_t)(y0 + g_st7789_cfg.y_offset);
    uint16_t panel_y1 = (uint16_t)(y1 + g_st7789_cfg.y_offset);

    if (st7789_cmd(0x2AU) != 0) {
        return -1;
    }
    data[0] = (uint8_t)(panel_x0 >> 8U);
    data[1] = (uint8_t)panel_x0;
    data[2] = (uint8_t)(panel_x1 >> 8U);
    data[3] = (uint8_t)panel_x1;
    if (st7789_data(data, sizeof(data)) != 0) {
        return -1;
    }
    if (st7789_cmd(0x2BU) != 0) {
        return -1;
    }
    data[0] = (uint8_t)(panel_y0 >> 8U);
    data[1] = (uint8_t)panel_y0;
    data[2] = (uint8_t)(panel_y1 >> 8U);
    data[3] = (uint8_t)panel_y1;
    if (st7789_data(data, sizeof(data)) != 0) {
        return -1;
    }
    return st7789_cmd(0x2CU);
}

static void st7789_set_pixel(uint16_t x, uint16_t y, uint16_t color) ST7789_UNUSED_FUNC;

static void st7789_set_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    uint8_t data[2];

    if (g_st7789_ready == 0U || x >= g_st7789_cfg.width || y >= g_st7789_cfg.height) {
        return;
    }
    if (st7789_addr_window(x, y, x, y) != 0) {
        return;
    }
    data[0] = (uint8_t)(color >> 8U);
    data[1] = (uint8_t)color;
    (void)st7789_data(data, sizeof(data));
}

static void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t row;
    uint16_t col;

    if (g_st7789_ready == 0U || w == 0U || h == 0U || x >= g_st7789_cfg.width || y >= g_st7789_cfg.height) {
        return;
    }
    if ((uint32_t)x + w > g_st7789_cfg.width) {
        w = (uint16_t)(g_st7789_cfg.width - x);
    }
    if ((uint32_t)y + h > g_st7789_cfg.height) {
        h = (uint16_t)(g_st7789_cfg.height - y);
    }
#if !ST7789_SOFT_SPI_ENABLE
    uint8_t line[240 * 2];

    if (w > (uint16_t)(sizeof(line) / 2U)) {
        w = (uint16_t)(sizeof(line) / 2U);
    }
    for (col = 0U; col < w; col++) {
        line[col * 2U] = (uint8_t)(color >> 8U);
        line[col * 2U + 1U] = (uint8_t)color;
    }
#endif
    if (st7789_addr_window(x, y, (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U)) != 0) {
        return;
    }
    st7789_dc(1U);
    st7789_cs(1U);
    for (row = 0U; row < h; row++) {
#if ST7789_SOFT_SPI_ENABLE
        for (col = 0U; col < w; col++) {
            st7789_soft_spi_write_u8((uint8_t)(color >> 8U));
            st7789_soft_spi_write_u8((uint8_t)color);
        }
#else
        spi_xfer_data_t data = {0};

        data.tx_buff = line;
        data.tx_bytes = (uint32_t)w * 2U;
        if (uapi_spi_master_write(g_st7789_cfg.spi_bus, &data, ST7789_SPI_TIMEOUT) != ERRCODE_SUCC) {
            break;
        }
#endif
    }
    st7789_spi_idle_level();
    st7789_cs(0U);
}

static void st7789_draw_char_fast(uint16_t x, uint16_t y, const uint8_t *glyph, uint16_t fg, uint16_t bg)
{
    uint8_t pixel_bytes[ST7789_FONT6X8_WIDTH * ST7789_FONT6X8_HEIGHT * 2U];
    uint32_t idx = 0U;
    uint8_t row;
    uint8_t col;

    if (glyph == NULL) {
        return;
    }
    if (x >= g_st7789_cfg.width || y >= g_st7789_cfg.height) {
        return;
    }
    if ((uint32_t)x + ST7789_FONT6X8_WIDTH > g_st7789_cfg.width ||
        (uint32_t)y + ST7789_FONT6X8_HEIGHT > g_st7789_cfg.height) {
        return;
    }
    for (row = 0U; row < ST7789_FONT6X8_HEIGHT; row++) {
        for (col = 0U; col < ST7789_FONT6X8_WIDTH; col++) {
            uint16_t color = ((glyph[col] >> row) & 0x01U) != 0U ? fg : bg;

            pixel_bytes[idx++] = (uint8_t)(color >> 8U);
            pixel_bytes[idx++] = (uint8_t)color;
        }
    }
    if (st7789_addr_window(x, y, (uint16_t)(x + ST7789_FONT6X8_WIDTH - 1U),
        (uint16_t)(y + ST7789_FONT6X8_HEIGHT - 1U)) != 0) {
        return;
    }
    (void)st7789_data(pixel_bytes, sizeof(pixel_bytes));
}

static void st7789_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg)
{
    uint8_t code = (uint8_t)c;
    const uint8_t *glyph;

    if (code < ST7789_FONT6X8_FIRST || code >= (ST7789_FONT6X8_FIRST + ST7789_FONT6X8_COUNT)) {
        code = (uint8_t)'?';
    }
    glyph = g_st7789_font6x8[code - ST7789_FONT6X8_FIRST];
    st7789_draw_char_fast(x, y, glyph, fg, bg);
}

static void st7789_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t fg, uint16_t bg)
{
    uint16_t cursor = x;
    size_t i;

    if (text == NULL) {
        return;
    }
    for (i = 0U; text[i] != '\0' && i < ST7789_MAX_TEXT_LEN; i++) {
        st7789_draw_char(cursor, y, text[i], fg, bg);
        cursor = (uint16_t)(cursor + ST7789_TEXT_ADVANCE_X);
        if (cursor + ST7789_FONT6X8_WIDTH >= g_st7789_cfg.width) {
            break;
        }
    }
}

static void st7789_init_pins(void)
{
    (void)uapi_pin_set_mode(g_st7789_cfg.sclk_pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_mode(g_st7789_cfg.mosi_pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_mode(g_st7789_cfg.cs_pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_mode(g_st7789_cfg.dc_pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_mode(g_st7789_cfg.reset_pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_gpio_set_dir(g_st7789_cfg.sclk_pin, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_dir(g_st7789_cfg.mosi_pin, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_dir(g_st7789_cfg.cs_pin, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_dir(g_st7789_cfg.dc_pin, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_dir(g_st7789_cfg.reset_pin, GPIO_DIRECTION_OUTPUT);
    st7789_spi_idle_level();
    st7789_mosi(0U);
    st7789_cs(0U);
}

static void st7789_hw_reset(void)
{
    (void)uapi_gpio_set_val(g_st7789_cfg.reset_pin, GPIO_LEVEL_LOW);
    osal_msleep(20);
    (void)uapi_gpio_set_val(g_st7789_cfg.reset_pin, GPIO_LEVEL_HIGH);
    osal_msleep(120);
}

static int st7789_spi_init(void)
{
#if ST7789_SOFT_SPI_ENABLE
    return 0;
#else
    spi_attr_t attr = {0};
    spi_extra_attr_t extra = {0};

    attr.is_slave = false;
    attr.slave_num = ST7789_SPI_SLAVE_NUM;
    attr.bus_clk = 32000000;
    attr.freq_mhz = ST7789_SPI_FREQ_MHZ;
    attr.clk_polarity = 1;
    attr.clk_phase = 1;
    attr.frame_format = 0;
    attr.spi_frame_format = HAL_SPI_FRAME_FORMAT_STANDARD;
    attr.frame_size = HAL_SPI_FRAME_SIZE_8;
    attr.tmod = 0;
    attr.sste = 1;
    extra.qspi_param.wait_cycles = ST7789_SPI_WAIT_CYCLES;
    return uapi_spi_init(g_st7789_cfg.spi_bus, &attr, &extra) == ERRCODE_SUCC ? 0 : -1;
#endif
}

static int st7789_init_sequence(void)
{
#if ST7789_FULL_INIT_SEQ_ENABLE
    static const uint8_t cmd_b2[] = {0x0CU, 0x0CU, 0x00U, 0x33U, 0x33U};
    static const uint8_t cmd_b7[] = {0x35U};
    static const uint8_t cmd_bb[] = {0x1FU};
    static const uint8_t cmd_c0[] = {0x2CU};
    static const uint8_t cmd_c2[] = {0x01U};
    static const uint8_t cmd_c3[] = {0x0BU};
    static const uint8_t cmd_c4[] = {0x20U};
    static const uint8_t cmd_c6[] = {0x0FU};
    static const uint8_t cmd_d0[] = {0xA4U, 0xA1U};
    static const uint8_t cmd_e0[] = {
        0xD0U, 0x08U, 0x11U, 0x08U, 0x0CU, 0x15U, 0x39U, 0x33U,
        0x50U, 0x36U, 0x13U, 0x14U, 0x29U, 0x2DU
    };
    static const uint8_t cmd_e1[] = {
        0xD0U, 0x08U, 0x10U, 0x08U, 0x06U, 0x06U, 0x39U, 0x44U,
        0x51U, 0x0BU, 0x16U, 0x14U, 0x2FU, 0x31U
    };
    static const uint8_t cmd_3a[] = {0x55U};
    static const uint8_t cmd_36[] = {ST7789_MADCTL_DEFAULT};

    if (st7789_cmd(0x01U) != 0) {
        return -1;
    }
    osal_msleep(150);
    if (st7789_cmd(0x11U) != 0) {
        return -1;
    }
    osal_msleep(120);
    if (st7789_cmd_data(0x3AU, cmd_3a, sizeof(cmd_3a)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0x36U, cmd_36, sizeof(cmd_36)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xB2U, cmd_b2, sizeof(cmd_b2)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xB7U, cmd_b7, sizeof(cmd_b7)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xBBU, cmd_bb, sizeof(cmd_bb)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xC0U, cmd_c0, sizeof(cmd_c0)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xC2U, cmd_c2, sizeof(cmd_c2)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xC3U, cmd_c3, sizeof(cmd_c3)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xC4U, cmd_c4, sizeof(cmd_c4)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xC6U, cmd_c6, sizeof(cmd_c6)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xD0U, cmd_d0, sizeof(cmd_d0)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xE0U, cmd_e0, sizeof(cmd_e0)) != 0) {
        return -1;
    }
    if (st7789_cmd_data(0xE1U, cmd_e1, sizeof(cmd_e1)) != 0) {
        return -1;
    }
    if (st7789_cmd(0x21U) != 0) {
        return -1;
    }
    if (st7789_cmd(0x13U) != 0) {
        return -1;
    }
    if (st7789_cmd(0x29U) != 0) {
        return -1;
    }
    osal_msleep(20);
    return 0;
#else
    if (st7789_cmd(0x01U) != 0) {
        return -1;
    }
    osal_msleep(150);
    if (st7789_cmd(0x11U) != 0) {
        return -1;
    }
    osal_msleep(120);
    if (st7789_cmd(0x3AU) != 0 || st7789_data_u8(0x55U) != 0) {
        return -1;
    }
    /* 1.14'' 135x240 ST7789 modules commonly need BGR + row/column swap. */
    if (st7789_cmd(0x36U) != 0 || st7789_data_u8(0x70U) != 0) {
        return -1;
    }
    /* Keep default non-inversion to avoid panel-dependent full-black behavior. */
    if (st7789_cmd(0x20U) != 0) {
        return -1;
    }
    if (st7789_cmd(0x13U) != 0) {
        return -1;
    }
    if (st7789_cmd(0x29U) != 0) {
        return -1;
    }
    osal_msleep(20);
    return 0;
#endif
}

#if SLE_TEAM_USE_LVGL_BACKEND
static int st7789_push_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *pixels)
{
    uint16_t row;
    uint16_t col;

    if (pixels == NULL || w == 0U || h == 0U) {
        return -1;
    }
    if ((uint32_t)x + w > g_st7789_cfg.width || (uint32_t)y + h > g_st7789_cfg.height) {
        return -1;
    }
    if (st7789_addr_window(x, y, (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U)) != 0) {
        return -1;
    }
    st7789_dc(1U);
    st7789_cs(1U);
    for (row = 0U; row < h; row++) {
        const uint16_t *row_pixels = pixels + (uint32_t)row * w;
#if ST7789_SOFT_SPI_ENABLE
        for (col = 0U; col < w; col++) {
            uint16_t px = row_pixels[col];
            st7789_soft_spi_write_u8((uint8_t)(px >> 8U));
            st7789_soft_spi_write_u8((uint8_t)px);
        }
#else
        spi_xfer_data_t data = {0};

        data.tx_buff = (uint8_t *)row_pixels;
        data.tx_bytes = (uint32_t)w * 2U;
        if (uapi_spi_master_write(g_st7789_cfg.spi_bus, &data, ST7789_SPI_TIMEOUT) != ERRCODE_SUCC) {
            st7789_cs(0U);
            return -1;
        }
#endif
    }
    st7789_spi_idle_level();
    st7789_cs(0U);
    return 0;
}

#if LVGL_VERSION_MAJOR >= 9
static void st7789_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint16_t x = (uint16_t)area->x1;
    uint16_t y = (uint16_t)area->y1;
    uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
    uint16_t h = (uint16_t)(area->y2 - area->y1 + 1);
    (void)st7789_push_rect(x, y, w, h, (const uint16_t *)px_map);
    lv_display_flush_ready(disp);
}
#else
static void st7789_lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint16_t x = (uint16_t)area->x1;
    uint16_t y = (uint16_t)area->y1;
    uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
    uint16_t h = (uint16_t)(area->y2 - area->y1 + 1);
    (void)st7789_push_rect(x, y, w, h, (const uint16_t *)color_p);
    lv_disp_flush_ready(disp_drv);
}
#endif

static void st7789_lvgl_create_ui(void)
{
    lv_obj_t *root = lv_scr_act();

    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_color(root, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_pad_all(root, 4, LV_PART_MAIN);

    g_st7789_lv_label_title = lv_label_create(root);
    lv_obj_set_style_text_color(g_st7789_lv_label_title, lv_color_hex(0x34D399), LV_PART_MAIN);
    lv_obj_align(g_st7789_lv_label_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_text(g_st7789_lv_label_title, "SLE boot");

    g_st7789_lv_label_status = lv_label_create(root);
    lv_obj_set_style_text_color(g_st7789_lv_label_status, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(g_st7789_lv_label_status, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_label_set_text(g_st7789_lv_label_status, "ID - ON 0 LOST 0");

    g_st7789_lv_label_alert = lv_label_create(root);
    lv_obj_set_style_text_color(g_st7789_lv_label_alert, lv_color_hex(0xFACC15), LV_PART_MAIN);
    lv_obj_align(g_st7789_lv_label_alert, LV_ALIGN_TOP_LEFT, 0, 52);
    lv_label_set_text(g_st7789_lv_label_alert, "ALERT -");
}

static void st7789_lvgl_flush_now(void)
{
    if (g_st7789_lv_display == NULL) {
        return;
    }
    lv_refr_now(g_st7789_lv_display);
    (void)lv_timer_handler();
}

static int st7789_lvgl_init(void)
{
    uint32_t buf_lines = CONFIG_SLE_TEAM_LVGL_DRAW_BUF_LINES;
    uint32_t px_count;

    if (buf_lines == 0U) {
        buf_lines = 16U;
    }
    lv_init();
    if (buf_lines > g_st7789_cfg.height) {
        buf_lines = g_st7789_cfg.height;
    }
    px_count = (uint32_t)g_st7789_cfg.width * buf_lines;
    g_st7789_lv_buf1 = (lv_color_t *)osal_vmalloc((uint32_t)sizeof(lv_color_t) * px_count);
    if (g_st7789_lv_buf1 == NULL) {
        osal_printk("[display] lvgl draw buffer alloc failed px=%lu\r\n", (unsigned long)px_count);
        return -1;
    }
    g_st7789_lv_buf_px_count = px_count;
#if LVGL_VERSION_MAJOR >= 9
    g_st7789_lv_display = lv_display_create(g_st7789_cfg.width, g_st7789_cfg.height);
    if (g_st7789_lv_display == NULL) {
        osal_vfree(g_st7789_lv_buf1);
        g_st7789_lv_buf1 = NULL;
        return -1;
    }
    lv_display_set_flush_cb(g_st7789_lv_display, st7789_lvgl_flush_cb);
    lv_display_set_buffers(g_st7789_lv_display, g_st7789_lv_buf1, NULL, px_count * sizeof(lv_color_t),
        LV_DISPLAY_RENDER_MODE_PARTIAL);
#else
    lv_disp_draw_buf_init(&g_st7789_lv_draw_buf, g_st7789_lv_buf1, NULL, px_count);
    lv_disp_drv_init(&g_st7789_lv_disp_drv);
    g_st7789_lv_disp_drv.hor_res = g_st7789_cfg.width;
    g_st7789_lv_disp_drv.ver_res = g_st7789_cfg.height;
    g_st7789_lv_disp_drv.flush_cb = st7789_lvgl_flush_cb;
    g_st7789_lv_disp_drv.draw_buf = &g_st7789_lv_draw_buf;
    g_st7789_lv_display = lv_disp_drv_register(&g_st7789_lv_disp_drv);
    if (g_st7789_lv_display == NULL) {
        osal_vfree(g_st7789_lv_buf1);
        g_st7789_lv_buf1 = NULL;
        return -1;
    }
#endif
    st7789_lvgl_create_ui();
    g_st7789_lv_ready = 1U;
    osal_printk("[display] lvgl backend enabled, draw_buf=%lu px (~%lu bytes)\r\n",
        (unsigned long)g_st7789_lv_buf_px_count,
        (unsigned long)(g_st7789_lv_buf_px_count * sizeof(lv_color_t)));
    return 0;
}
#endif

int ws63_st7789_init(const ws63_st7789_config_t *cfg)
{
    if (cfg == NULL || cfg->width == 0U || cfg->height == 0U) {
        return -1;
    }
    (void)memcpy_s(&g_st7789_cfg, sizeof(g_st7789_cfg), cfg, sizeof(*cfg));
    g_st7789_ready = 0U;
    st7789_init_pins();
    if (st7789_spi_init() != 0) {
        osal_printk("[display] st7789 spi init failed\r\n");
        return -1;
    }
    st7789_hw_reset();
    if (st7789_init_sequence() != 0) {
        osal_printk("[display] st7789 init sequence failed\r\n");
        return -1;
    }
    g_st7789_ready = 1U;
    st7789_fill_rect(0U, 0U, g_st7789_cfg.width, g_st7789_cfg.height, ST7789_COLOR_BLACK);
#if SLE_TEAM_USE_LVGL_BACKEND
    if (st7789_lvgl_init() != 0) {
        osal_printk("[display] lvgl init failed, fallback to built-in text renderer\r\n");
    }
#else
    if (CONFIG_SLE_TEAM_DISPLAY_USE_LVGL != 0) {
        osal_printk("[display] lvgl requested but headers are unavailable; fallback enabled\r\n");
    }
#endif
#if SLE_TEAM_USE_LVGL_BACKEND
    if (g_st7789_lv_ready != 0U) {
        st7789_lvgl_flush_now();
    } else {
        st7789_draw_text(4U, 8U, "SLE boot", ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    }
#else
    st7789_draw_text(4U, 8U, "SLE boot", ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
#endif
    g_st7789_status_cache_valid = 0U;
    (void)memset_s(g_st7789_last_role, sizeof(g_st7789_last_role), 0, sizeof(g_st7789_last_role));
    (void)memset_s(g_st7789_last_self, sizeof(g_st7789_last_self), 0, sizeof(g_st7789_last_self));
    (void)memset_s(g_st7789_last_fw, sizeof(g_st7789_last_fw), 0, sizeof(g_st7789_last_fw));
    g_st7789_last_tick_ms = uapi_tcxo_get_ms();
    osal_printk("[display] st7789 ready %ux%u off=%u,%u sclk=%u mosi=%u cs=%u dc=%u rst=%u\r\n",
        g_st7789_cfg.width, g_st7789_cfg.height, g_st7789_cfg.x_offset, g_st7789_cfg.y_offset,
        g_st7789_cfg.sclk_pin, g_st7789_cfg.mosi_pin, g_st7789_cfg.cs_pin, g_st7789_cfg.dc_pin,
        g_st7789_cfg.reset_pin);
#if ST7789_SOFT_SPI_ENABLE
    osal_printk("[display] soft-spi enabled mode=%u (cpol=%u cpha=%u)\r\n",
        (uint32_t)(ST7789_SOFT_SPI_MODE3 ? 3U : 0U),
        (uint32_t)(ST7789_SOFT_SPI_MODE3 ? 1U : 0U),
        (uint32_t)(ST7789_SOFT_SPI_MODE3 ? 1U : 0U));
#endif
    return 0;
}

int ws63_st7789_show_status(const char *role, const char *self, uint8_t online_count,
    uint8_t offline_count, uint8_t alert_count, const char *fw_version)
{
    char line[ST7789_MAX_TEXT_LEN];
    const char *role_text = role != NULL ? role : "-";
    const char *self_text = self != NULL ? self : "-";
    const char *fw_text = fw_version != NULL ? fw_version : "-";

    if (g_st7789_ready == 0U) {
        return -1;
    }
    if (g_st7789_status_cache_valid != 0U &&
        g_st7789_last_online_count == online_count &&
        g_st7789_last_offline_count == offline_count &&
        g_st7789_last_alert_count == alert_count &&
        strcmp(g_st7789_last_role, role_text) == 0 &&
        strcmp(g_st7789_last_self, self_text) == 0 &&
        strcmp(g_st7789_last_fw, fw_text) == 0) {
        return 0;
    }
#if SLE_TEAM_USE_LVGL_BACKEND
    if (g_st7789_lv_ready != 0U) {
        if (g_st7789_lv_label_title != NULL) {
            (void)snprintf(line, sizeof(line), "SLE %s %s", fw_text, role_text);
            lv_label_set_text(g_st7789_lv_label_title, line);
        }
        if (g_st7789_lv_label_status != NULL) {
            (void)snprintf(line, sizeof(line), "ID %s ON %u OFF %u A%u",
                self_text, online_count, offline_count, alert_count);
            lv_label_set_text(g_st7789_lv_label_status, line);
        }
    } else {
        st7789_fill_rect(0U, 0U, g_st7789_cfg.width, 48U, ST7789_COLOR_BLACK);
        (void)snprintf(line, sizeof(line), "SLE %s %s", fw_text, role_text);
        st7789_draw_text(4U, 8U, line, ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
        (void)snprintf(line, sizeof(line), "ID %s ON %u OFF %u A%u",
            self_text, online_count, offline_count, alert_count);
        st7789_draw_text(4U, 24U, line, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    }
#else
    st7789_fill_rect(0U, 0U, g_st7789_cfg.width, 48U, ST7789_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "SLE %s %s", fw_text, role_text);
    st7789_draw_text(4U, 8U, line, ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "ID %s ON %u OFF %u A%u",
        self_text, online_count, offline_count, alert_count);
    st7789_draw_text(4U, 24U, line, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
#endif
    g_st7789_last_online_count = online_count;
    g_st7789_last_offline_count = offline_count;
    g_st7789_last_alert_count = alert_count;
    (void)snprintf(g_st7789_last_role, sizeof(g_st7789_last_role), "%s", role_text);
    (void)snprintf(g_st7789_last_self, sizeof(g_st7789_last_self), "%s", self_text);
    (void)snprintf(g_st7789_last_fw, sizeof(g_st7789_last_fw), "%s", fw_text);
    g_st7789_status_cache_valid = 1U;
    return 0;
}

int ws63_st7789_show_alert(uint8_t member_id, int32_t latitude_e6, int32_t longitude_e6, uint32_t last_seen_s)
{
    char line[ST7789_MAX_TEXT_LEN];

    if (g_st7789_ready == 0U) {
        return -1;
    }
#if SLE_TEAM_USE_LVGL_BACKEND
    if (g_st7789_lv_ready != 0U && g_st7789_lv_label_alert != NULL) {
        char alert_line[ST7789_MAX_TEXT_LEN];
        int32_t lat_int = latitude_e6 / 1000000L;
        int32_t lat_frac = latitude_e6 >= 0 ? (latitude_e6 % 1000000L) : -(latitude_e6 % 1000000L);
        int32_t lon_int = longitude_e6 / 1000000L;
        int32_t lon_frac = longitude_e6 >= 0 ? (longitude_e6 % 1000000L) : -(longitude_e6 % 1000000L);
        (void)snprintf(alert_line, sizeof(alert_line), "LOST M%u @%lu", member_id, (unsigned long)last_seen_s);
        (void)snprintf(line, sizeof(line), "LAT %ld.%06ld LON %ld.%06ld",
            (long)lat_int, (long)lat_frac, (long)lon_int, (long)lon_frac);
        lv_label_set_text_fmt(g_st7789_lv_label_alert, "%s\n%s", alert_line, line);
        return 0;
    }
#endif
    st7789_fill_rect(0U, 56U, g_st7789_cfg.width, 64U, ST7789_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "LOST M%u @%lu", member_id, (unsigned long)last_seen_s);
    st7789_draw_text(4U, 60U, line, ST7789_COLOR_RED, ST7789_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "LAT %ld", (long)latitude_e6);
    st7789_draw_text(4U, 76U, line, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "LON %ld", (long)longitude_e6);
    st7789_draw_text(4U, 92U, line, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
    return 0;
}

void ws63_st7789_tick(void)
{
    uint32_t now_ms;
    uint32_t delta_ms;

    if (g_st7789_ready == 0U) {
        return;
    }
    now_ms = uapi_tcxo_get_ms();
    delta_ms = now_ms - g_st7789_last_tick_ms;
    if (delta_ms == 0U) {
        return;
    }
#if SLE_TEAM_USE_LVGL_BACKEND
    if (g_st7789_lv_ready != 0U) {
        lv_tick_inc(delta_ms);
        (void)lv_timer_handler();
        g_st7789_last_tick_ms = now_ms;
        return;
    }
#endif
    if (delta_ms >= ST7789_LVGL_TICK_FALLBACK_MS) {
        g_st7789_last_tick_ms = now_ms;
    }
}
