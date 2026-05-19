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

#define ST7789_SPI_SLAVE_NUM 1
#define ST7789_SPI_FREQ_MHZ 24
#define ST7789_SPI_WAIT_CYCLES 0x10
#define ST7789_SPI_TIMEOUT 0xFFFFFFFFU
#define ST7789_COLOR_BLACK 0x0000U
#define ST7789_COLOR_WHITE 0xFFFFU
#define ST7789_COLOR_RED 0xF800U
#define ST7789_COLOR_GREEN 0x07E0U
#define ST7789_COLOR_BLUE 0x001FU
#define ST7789_COLOR_YELLOW 0xFFE0U
#define ST7789_MAX_TEXT_LEN 32U
#define ST7789_FONT6X8_FIRST 32U
#define ST7789_FONT6X8_COUNT ((uint8_t)(sizeof(g_st7789_font6x8) / sizeof(g_st7789_font6x8[0])))

static ws63_st7789_config_t g_st7789_cfg;
static uint8_t g_st7789_ready;

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

static int st7789_write(const uint8_t *buf, uint32_t len)
{
    spi_xfer_data_t data = {0};
    errcode_t ret;

    if (buf == NULL || len == 0U) {
        return -1;
    }
    data.tx_buff = (uint8_t *)buf;
    data.tx_bytes = len;
    st7789_cs(1U);
    ret = uapi_spi_master_write(g_st7789_cfg.spi_bus, &data, ST7789_SPI_TIMEOUT);
    st7789_cs(0U);
    return ret == ERRCODE_SUCC ? 0 : -1;
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

static int st7789_data_u8(uint8_t value)
{
    return st7789_data(&value, 1U);
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
    uint8_t line[240 * 2];
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
    if (w > (uint16_t)(sizeof(line) / 2U)) {
        w = (uint16_t)(sizeof(line) / 2U);
    }
    for (col = 0U; col < w; col++) {
        line[col * 2U] = (uint8_t)(color >> 8U);
        line[col * 2U + 1U] = (uint8_t)color;
    }
    if (st7789_addr_window(x, y, (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U)) != 0) {
        return;
    }
    st7789_dc(1U);
    st7789_cs(1U);
    for (row = 0U; row < h; row++) {
        spi_xfer_data_t data = {0};

        data.tx_buff = line;
        data.tx_bytes = (uint32_t)w * 2U;
        if (uapi_spi_master_write(g_st7789_cfg.spi_bus, &data, ST7789_SPI_TIMEOUT) != ERRCODE_SUCC) {
            break;
        }
    }
    st7789_cs(0U);
}

static void st7789_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg)
{
    uint8_t row;
    uint8_t col;
    uint8_t code = (uint8_t)c;
    const uint8_t *glyph;

    if (code < ST7789_FONT6X8_FIRST || code >= (ST7789_FONT6X8_FIRST + ST7789_FONT6X8_COUNT)) {
        code = (uint8_t)'?';
    }
    glyph = g_st7789_font6x8[code - ST7789_FONT6X8_FIRST];
    for (col = 0U; col < 6U; col++) {
        uint8_t bits = glyph[col];

        for (row = 0U; row < 8U; row++) {
            st7789_set_pixel((uint16_t)(x + col), (uint16_t)(y + row),
                ((bits >> row) & 0x01U) != 0U ? fg : bg);
        }
    }
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
        cursor = (uint16_t)(cursor + 7U);
        if (cursor + 6U >= g_st7789_cfg.width) {
            break;
        }
    }
}

static void st7789_init_pins(void)
{
    (void)uapi_pin_set_mode(g_st7789_cfg.sclk_pin, PIN_MODE_3);
    (void)uapi_pin_set_mode(g_st7789_cfg.mosi_pin, PIN_MODE_3);
    (void)uapi_pin_set_mode(g_st7789_cfg.cs_pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_mode(g_st7789_cfg.dc_pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_mode(g_st7789_cfg.reset_pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_gpio_set_dir(g_st7789_cfg.cs_pin, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_dir(g_st7789_cfg.dc_pin, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_dir(g_st7789_cfg.reset_pin, GPIO_DIRECTION_OUTPUT);
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
}

static int st7789_init_sequence(void)
{
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
    if (st7789_cmd(0x36U) != 0 || st7789_data_u8(0x00U) != 0) {
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
}

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
    st7789_draw_text(4U, 8U, "SLE V4", ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    osal_printk("[display] st7789 ready %ux%u off=%u,%u sclk=%u mosi=%u cs=%u dc=%u rst=%u\r\n",
        g_st7789_cfg.width, g_st7789_cfg.height, g_st7789_cfg.x_offset, g_st7789_cfg.y_offset,
        g_st7789_cfg.sclk_pin, g_st7789_cfg.mosi_pin, g_st7789_cfg.cs_pin, g_st7789_cfg.dc_pin,
        g_st7789_cfg.reset_pin);
    return 0;
}

int ws63_st7789_show_status(const char *role, const char *self, uint8_t online_count, uint8_t lost_count)
{
    char line[ST7789_MAX_TEXT_LEN];

    if (g_st7789_ready == 0U) {
        return -1;
    }
    st7789_fill_rect(0U, 0U, g_st7789_cfg.width, 48U, ST7789_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "SLE V4 %s", role != NULL ? role : "-");
    st7789_draw_text(4U, 8U, line, ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "ID %s ON %u LOST %u", self != NULL ? self : "-", online_count, lost_count);
    st7789_draw_text(4U, 24U, line, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    return 0;
}

int ws63_st7789_show_alert(uint8_t member_id, int32_t latitude_e6, int32_t longitude_e6, uint32_t last_seen_s)
{
    char line[ST7789_MAX_TEXT_LEN];

    if (g_st7789_ready == 0U) {
        return -1;
    }
    st7789_fill_rect(0U, 56U, g_st7789_cfg.width, 64U, ST7789_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "LOST M%u @%lu", member_id, (unsigned long)last_seen_s);
    st7789_draw_text(4U, 60U, line, ST7789_COLOR_RED, ST7789_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "LAT %ld", (long)latitude_e6);
    st7789_draw_text(4U, 76U, line, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "LON %ld", (long)longitude_e6);
    st7789_draw_text(4U, 92U, line, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
    return 0;
}
