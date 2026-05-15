#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_init.h"
#include "cmsis_os2.h"
#include "errcode.h"
#include "gpio.h"
#include "osal_debug.h"
#include "pinctrl.h"
#include "soc_osal.h"
#include "spi.h"

#ifndef CONFIG_ST7789_SPI_BUS_ID
#define CONFIG_ST7789_SPI_BUS_ID 0
#endif

#ifndef CONFIG_ST7789_SPI_FREQ_MHZ
#define CONFIG_ST7789_SPI_FREQ_MHZ 16
#endif

#ifndef CONFIG_ST7789_TFT_SCL_PIN
#define CONFIG_ST7789_TFT_SCL_PIN 7
#endif

#ifndef CONFIG_ST7789_TFT_SDA_PIN
#define CONFIG_ST7789_TFT_SDA_PIN 9
#endif

#ifndef CONFIG_ST7789_TFT_MISO_PIN
#define CONFIG_ST7789_TFT_MISO_PIN (-1)
#endif

#ifndef CONFIG_ST7789_TFT_CS_PIN
#define CONFIG_ST7789_TFT_CS_PIN 8
#endif

#ifndef CONFIG_ST7789_TFT_DC_PIN
#define CONFIG_ST7789_TFT_DC_PIN 10
#endif

#ifndef CONFIG_ST7789_TFT_RESET_PIN
#define CONFIG_ST7789_TFT_RESET_PIN 6
#endif

#ifndef CONFIG_ST7789_TFT_X_OFFSET
#define CONFIG_ST7789_TFT_X_OFFSET 52
#endif

#ifndef CONFIG_ST7789_TFT_Y_OFFSET
#define CONFIG_ST7789_TFT_Y_OFFSET 40
#endif

#ifndef CONFIG_ST7789_FRAME_DELAY_MS
#define CONFIG_ST7789_FRAME_DELAY_MS 16
#endif

#ifndef CONFIG_ST7789_TASK_STACK_SIZE
#define CONFIG_ST7789_TASK_STACK_SIZE 6144
#endif

#ifndef CONFIG_ST7789_TASK_PRIO
#define CONFIG_ST7789_TASK_PRIO 17
#endif

#define ST7789_WIDTH 135U
#define ST7789_HEIGHT 240U
#define ST7789_SPI_BUS_CLK_HZ 32000000U
#define ST7789_SPI_TIMEOUT 0xFFFFFFFFU
#define ST7789_SPI_WAIT_CYCLES 0x10U

#define ST7789_CMD_SWRESET 0x01
#define ST7789_CMD_SLPOUT 0x11
#define ST7789_CMD_NORON 0x13
#define ST7789_CMD_INVOFF 0x20
#define ST7789_CMD_INVON 0x21
#define ST7789_CMD_DISPON 0x29
#define ST7789_CMD_CASET 0x2A
#define ST7789_CMD_RASET 0x2B
#define ST7789_CMD_RAMWR 0x2C
#define ST7789_CMD_MADCTL 0x36
#define ST7789_CMD_COLMOD 0x3A

#define ST7789_MADCTL_BGR 0x08
#define ST7789_BLOCK_SIZE 28U

static uint8_t g_line_buffer[ST7789_WIDTH * 2U];

static void st7789_select(void)
{
    (void)uapi_gpio_set_val(CONFIG_ST7789_TFT_CS_PIN, GPIO_LEVEL_LOW);
}

static void st7789_unselect(void)
{
    (void)uapi_gpio_set_val(CONFIG_ST7789_TFT_CS_PIN, GPIO_LEVEL_HIGH);
}

static void st7789_dc_command(void)
{
    (void)uapi_gpio_set_val(CONFIG_ST7789_TFT_DC_PIN, GPIO_LEVEL_LOW);
}

static void st7789_dc_data(void)
{
    (void)uapi_gpio_set_val(CONFIG_ST7789_TFT_DC_PIN, GPIO_LEVEL_HIGH);
}

static errcode_t st7789_spi_write(const uint8_t *buffer, uint32_t size)
{
    spi_xfer_data_t data = { 0 };

    if (buffer == NULL || size == 0U) {
        return ERRCODE_SUCC;
    }

    data.tx_buff = (uint8_t *)buffer;
    data.tx_bytes = size;
    return uapi_spi_master_write(CONFIG_ST7789_SPI_BUS_ID, &data, ST7789_SPI_TIMEOUT);
}

static errcode_t st7789_write_command(uint8_t command)
{
    errcode_t ret;

    st7789_dc_command();
    st7789_select();
    ret = st7789_spi_write(&command, 1U);
    st7789_unselect();

    return ret;
}

static errcode_t st7789_write_data(const uint8_t *buffer, uint32_t size)
{
    errcode_t ret;

    st7789_dc_data();
    st7789_select();
    ret = st7789_spi_write(buffer, size);
    st7789_unselect();

    return ret;
}

static errcode_t st7789_send_command(uint8_t command, const uint8_t *params, uint32_t param_len)
{
    errcode_t ret = st7789_write_command(command);

    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    return st7789_write_data(params, param_len);
}

static void st7789_gpio_init(void)
{
    (void)uapi_pin_set_mode(CONFIG_ST7789_TFT_SCL_PIN, PIN_MODE_3);
    (void)uapi_pin_set_mode(CONFIG_ST7789_TFT_SDA_PIN, PIN_MODE_3);
#if CONFIG_ST7789_TFT_MISO_PIN >= 0
    (void)uapi_pin_set_mode(CONFIG_ST7789_TFT_MISO_PIN, PIN_MODE_3);
#endif

    (void)uapi_pin_set_mode(CONFIG_ST7789_TFT_CS_PIN, PIN_MODE_0);
    (void)uapi_pin_set_mode(CONFIG_ST7789_TFT_DC_PIN, PIN_MODE_0);
    (void)uapi_pin_set_mode(CONFIG_ST7789_TFT_RESET_PIN, PIN_MODE_0);

    (void)uapi_gpio_set_dir(CONFIG_ST7789_TFT_CS_PIN, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_dir(CONFIG_ST7789_TFT_DC_PIN, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_dir(CONFIG_ST7789_TFT_RESET_PIN, GPIO_DIRECTION_OUTPUT);

    st7789_unselect();
    st7789_dc_data();
    (void)uapi_gpio_set_val(CONFIG_ST7789_TFT_RESET_PIN, GPIO_LEVEL_HIGH);
}

static errcode_t st7789_spi_init(void)
{
    spi_attr_t config = { 0 };
    spi_extra_attr_t extra_config = { 0 };

    config.is_slave = false;
    config.slave_num = 1;
    config.bus_clk = ST7789_SPI_BUS_CLK_HZ;
    config.freq_mhz = CONFIG_ST7789_SPI_FREQ_MHZ;
    config.clk_polarity = SPI_CFG_CLK_CPOL_0;
    config.clk_phase = SPI_CFG_CLK_CPHA_0;
    config.frame_format = SPI_CFG_FRAME_FORMAT_MOTOROLA_SPI;
    config.spi_frame_format = HAL_SPI_FRAME_FORMAT_STANDARD;
    config.frame_size = HAL_SPI_FRAME_SIZE_8;
    config.tmod = HAL_SPI_TRANS_MODE_TX;
    config.sste = SPI_CFG_SSTE_ENABLE;
    extra_config.sspi_param.wait_cycles = ST7789_SPI_WAIT_CYCLES;

    return uapi_spi_init(CONFIG_ST7789_SPI_BUS_ID, &config, &extra_config);
}

static void st7789_hw_reset(void)
{
    (void)uapi_gpio_set_val(CONFIG_ST7789_TFT_RESET_PIN, GPIO_LEVEL_HIGH);
    osal_msleep(10);
    (void)uapi_gpio_set_val(CONFIG_ST7789_TFT_RESET_PIN, GPIO_LEVEL_LOW);
    osal_msleep(20);
    (void)uapi_gpio_set_val(CONFIG_ST7789_TFT_RESET_PIN, GPIO_LEVEL_HIGH);
    osal_msleep(120);
}

static errcode_t st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint16_t col_start = (uint16_t)(x0 + CONFIG_ST7789_TFT_X_OFFSET);
    uint16_t col_end = (uint16_t)(x1 + CONFIG_ST7789_TFT_X_OFFSET);
    uint16_t row_start = (uint16_t)(y0 + CONFIG_ST7789_TFT_Y_OFFSET);
    uint16_t row_end = (uint16_t)(y1 + CONFIG_ST7789_TFT_Y_OFFSET);
    uint8_t data[4];
    errcode_t ret;

    data[0] = (uint8_t)(col_start >> 8);
    data[1] = (uint8_t)(col_start & 0xFFU);
    data[2] = (uint8_t)(col_end >> 8);
    data[3] = (uint8_t)(col_end & 0xFFU);
    ret = st7789_send_command(ST7789_CMD_CASET, data, sizeof(data));
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    data[0] = (uint8_t)(row_start >> 8);
    data[1] = (uint8_t)(row_start & 0xFFU);
    data[2] = (uint8_t)(row_end >> 8);
    data[3] = (uint8_t)(row_end & 0xFFU);
    ret = st7789_send_command(ST7789_CMD_RASET, data, sizeof(data));
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    return st7789_write_command(ST7789_CMD_RAMWR);
}

static errcode_t st7789_lcd_init(void)
{
    uint8_t madctl = 0x00U;
    uint8_t colmod = 0x55U;
    errcode_t ret;

#if defined(CONFIG_ST7789_COLOR_ORDER_BGR)
    madctl |= ST7789_MADCTL_BGR;
#endif

    st7789_hw_reset();

    ret = st7789_write_command(ST7789_CMD_SWRESET);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    osal_msleep(150);

    ret = st7789_write_command(ST7789_CMD_SLPOUT);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    osal_msleep(120);

    ret = st7789_send_command(ST7789_CMD_MADCTL, &madctl, 1U);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    ret = st7789_send_command(ST7789_CMD_COLMOD, &colmod, 1U);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    osal_msleep(10);

#if defined(CONFIG_ST7789_INVERSION_ON)
    ret = st7789_write_command(ST7789_CMD_INVON);
#else
    ret = st7789_write_command(ST7789_CMD_INVOFF);
#endif
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    ret = st7789_write_command(ST7789_CMD_NORON);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    osal_msleep(10);

    ret = st7789_write_command(ST7789_CMD_DISPON);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    osal_msleep(120);

    return ERRCODE_SUCC;
}

static uint16_t st7789_rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return (uint16_t)(((uint16_t)(red & 0xF8U) << 8) |
        ((uint16_t)(green & 0xFCU) << 3) |
        ((uint16_t)blue >> 3));
}

static void st7789_line_pixel(uint32_t x, uint16_t color)
{
    uint32_t index = x * 2U;

    g_line_buffer[index] = (uint8_t)(color >> 8);
    g_line_buffer[index + 1U] = (uint8_t)(color & 0xFFU);
}

static errcode_t st7789_stream_line(void)
{
    return st7789_spi_write(g_line_buffer, sizeof(g_line_buffer));
}

static errcode_t st7789_show_color_bars(void)
{
    static const uint16_t colors[] = {
        0xF800U, 0xFFE0U, 0x07E0U, 0x07FFU, 0x001FU, 0xF81FU
    };
    uint32_t x;
    uint32_t y;
    errcode_t ret;

    ret = st7789_set_window(0U, 0U, ST7789_WIDTH - 1U, ST7789_HEIGHT - 1U);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    st7789_dc_data();
    st7789_select();
    for (y = 0U; y < ST7789_HEIGHT; y++) {
        for (x = 0U; x < ST7789_WIDTH; x++) {
            uint32_t index = (x * (uint32_t)(sizeof(colors) / sizeof(colors[0]))) / ST7789_WIDTH;
            st7789_line_pixel(x, colors[index]);
        }
        ret = st7789_stream_line();
        if (ret != ERRCODE_SUCC) {
            st7789_unselect();
            return ret;
        }
    }
    st7789_unselect();

    return ERRCODE_SUCC;
}

static uint32_t st7789_pingpong(uint32_t value, uint32_t limit)
{
    uint32_t period;
    uint32_t phase;

    if (limit == 0U) {
        return 0U;
    }

    period = limit * 2U;
    phase = value % period;
    if (phase <= limit) {
        return phase;
    }

    return period - phase;
}

static uint16_t st7789_animated_color(uint32_t x, uint32_t y, uint32_t frame)
{
    uint8_t red = (uint8_t)(32U + ((x * 3U + frame * 5U) & 0x7FU));
    uint8_t green = (uint8_t)(48U + ((y + frame * 3U) & 0x7FU));
    uint8_t blue = (uint8_t)(96U + ((x + y + frame * 4U) & 0x7FU));

    return st7789_rgb565(red, green, blue);
}

static errcode_t st7789_render_frame(uint32_t frame)
{
    uint32_t block_x = st7789_pingpong(frame * 3U, ST7789_WIDTH - ST7789_BLOCK_SIZE);
    uint32_t block_y = st7789_pingpong(frame * 2U, ST7789_HEIGHT - ST7789_BLOCK_SIZE);
    uint32_t scan_y = (frame * 5U) % ST7789_HEIGHT;
    uint32_t x;
    uint32_t y;
    errcode_t ret;

    ret = st7789_set_window(0U, 0U, ST7789_WIDTH - 1U, ST7789_HEIGHT - 1U);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    st7789_dc_data();
    st7789_select();
    for (y = 0U; y < ST7789_HEIGHT; y++) {
        for (x = 0U; x < ST7789_WIDTH; x++) {
            uint16_t color = st7789_animated_color(x, y, frame);
            bool in_block = (x >= block_x && x < block_x + ST7789_BLOCK_SIZE &&
                y >= block_y && y < block_y + ST7789_BLOCK_SIZE);
            bool block_edge = in_block &&
                (x == block_x || x == block_x + ST7789_BLOCK_SIZE - 1U ||
                y == block_y || y == block_y + ST7789_BLOCK_SIZE - 1U);

            if (y == scan_y || y == ((scan_y + 1U) % ST7789_HEIGHT)) {
                color = st7789_rgb565(255U, 255U, 255U);
            }
            if (((x + y + frame) & 0x1FU) == 0U) {
                color = st7789_rgb565(255U, (uint8_t)(80U + (frame & 0x7FU)), 32U);
            }
            if (in_block) {
                color = block_edge ? st7789_rgb565(255U, 255U, 255U) :
                    st7789_rgb565((uint8_t)(220U - (frame & 0x3FU)), 32U,
                        (uint8_t)(120U + (frame & 0x7FU)));
            }

            st7789_line_pixel(x, color);
        }

        ret = st7789_stream_line();
        if (ret != ERRCODE_SUCC) {
            st7789_unselect();
            return ret;
        }
    }
    st7789_unselect();

    return ERRCODE_SUCC;
}

static void *st7789_demo_task(const char *arg)
{
    uint32_t frame = 0U;
    errcode_t ret;

    (void)arg;

    st7789_gpio_init();
    ret = st7789_spi_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[st7789] spi init failed ret=0x%x\r\n", ret);
        return NULL;
    }

    ret = st7789_lcd_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[st7789] lcd init failed ret=0x%x\r\n", ret);
        return NULL;
    }

    osal_printk("[st7789] init ok bus=%d sck=%d mosi=%d cs=%d dc=%d rst=%d size=%dx%d\r\n",
        CONFIG_ST7789_SPI_BUS_ID,
        CONFIG_ST7789_TFT_SCL_PIN,
        CONFIG_ST7789_TFT_SDA_PIN,
        CONFIG_ST7789_TFT_CS_PIN,
        CONFIG_ST7789_TFT_DC_PIN,
        CONFIG_ST7789_TFT_RESET_PIN,
        ST7789_WIDTH,
        ST7789_HEIGHT);

    ret = st7789_show_color_bars();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[st7789] color bars failed ret=0x%x\r\n", ret);
    }
    osal_msleep(700);

    while (1) {
        ret = st7789_render_frame(frame++);
        if (ret != ERRCODE_SUCC) {
            osal_printk("[st7789] frame failed ret=0x%x\r\n", ret);
            osal_msleep(1000);
            continue;
        }
        osal_msleep(CONFIG_ST7789_FRAME_DELAY_MS);
    }

    return NULL;
}

static void st7789_demo_entry(void)
{
    osThreadAttr_t attr;

    attr.name = "St7789DemoTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = CONFIG_ST7789_TASK_STACK_SIZE;
    attr.priority = (osPriority_t)CONFIG_ST7789_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)st7789_demo_task, NULL, &attr) == NULL) {
        osal_printk("[st7789] failed to create task\r\n");
    }
}

app_run(st7789_demo_entry);
