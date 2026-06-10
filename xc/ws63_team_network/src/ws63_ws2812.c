#include "ws63_ws2812.h"

#ifndef WS63_WS2812_HOST_TEST
#include "common_def.h"
#include "errcode.h"
#include "gpio.h"
#include "platform_core.h"
#include "pinctrl.h"
#include "soc_osal.h"
#include "tcxo.h"

#define WS63_WS2812_MAX_PIN 18U
#define WS63_WS2812_GPIO_GROUP_WIDTH 8U
#define WS63_WS2812_GPIO_GROUP_STRIDE 0x40U
#define WS63_WS2812_GPIO_DATA_SET_OFFSET 0x30U
#define WS63_WS2812_GPIO_DATA_CLR_OFFSET 0x34U
#define WS63_WS2812_RESET_US 320U
#define WS63_WS2812_CPU_MHZ 160U
#define WS63_WS2812_T0H_NS 350U
#define WS63_WS2812_T1H_NS 700U
#define WS63_WS2812_SLOT_NS 1250U
#define WS63_WS2812_NS_TO_CYCLES(ns) \
    ((uint32_t)((((uint64_t)WS63_WS2812_CPU_MHZ * (uint64_t)(ns)) + 999ULL) / 1000ULL))
#define WS63_WS2812_T0H_CYCLES WS63_WS2812_NS_TO_CYCLES(WS63_WS2812_T0H_NS)
#define WS63_WS2812_T1H_CYCLES WS63_WS2812_NS_TO_CYCLES(WS63_WS2812_T1H_NS)
#define WS63_WS2812_SLOT_CYCLES WS63_WS2812_NS_TO_CYCLES(WS63_WS2812_SLOT_NS)

typedef struct {
    volatile uint32_t *set_reg;
    volatile uint32_t *clr_reg;
    uint32_t mask;
    uint8_t pin;
    uint8_t ready;
} ws63_ws2812_ctx_t;

static ws63_ws2812_ctx_t g_ws2812;

static uint32_t ws63_ws2812_cycle32(void)
{
    uint32_t cycle;

    __asm__ volatile("rdcycle %0" : "=r"(cycle));
    return cycle;
}

static void ws63_ws2812_wait_until_cycle(uint32_t target)
{
    while ((int32_t)(ws63_ws2812_cycle32() - target) < 0) {
    }
}

static uintptr_t ws63_ws2812_gpio_base(uint8_t pin)
{
    if (pin <= 7U) {
        return (uintptr_t)0x44028000U;
    }
    if (pin <= 15U) {
        return (uintptr_t)0x44029000U;
    }
    return (uintptr_t)0x4402A000U;
}

static void ws63_ws2812_pin_high(void)
{
    *g_ws2812.set_reg = g_ws2812.mask;
}

static void ws63_ws2812_pin_low(void)
{
    *g_ws2812.clr_reg = g_ws2812.mask;
}

static void ws63_ws2812_write_bit(uint8_t bit)
{
    uint32_t start;
    uint32_t high_cycles = bit != 0U ? WS63_WS2812_T1H_CYCLES : WS63_WS2812_T0H_CYCLES;

    start = ws63_ws2812_cycle32();
    ws63_ws2812_pin_high();
    ws63_ws2812_wait_until_cycle(start + high_cycles);
    ws63_ws2812_pin_low();
    ws63_ws2812_wait_until_cycle(start + WS63_WS2812_SLOT_CYCLES);
}

static void ws63_ws2812_write_byte(uint8_t value)
{
    uint8_t mask;

    for (mask = 0x80U; mask != 0U; mask >>= 1U) {
        ws63_ws2812_write_bit((value & mask) != 0U ? 1U : 0U);
    }
}
#endif

void ws63_ws2812_encode_grb(uint8_t red, uint8_t green, uint8_t blue, uint8_t out[3])
{
    if (out == 0) {
        return;
    }
    out[0] = green;
    out[1] = red;
    out[2] = blue;
}

#ifndef WS63_WS2812_HOST_TEST
int ws63_ws2812_init(uint8_t pin)
{
    uintptr_t base;
    uintptr_t group_offset;
    uint8_t group_pin;

    if (pin > WS63_WS2812_MAX_PIN) {
        return -1;
    }

    (void)uapi_pin_set_mode(pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_ds(pin, PIN_DS_7);
    (void)uapi_pin_set_pull(pin, PIN_PULL_TYPE_DISABLE);
    if (uapi_gpio_set_dir(pin, GPIO_DIRECTION_OUTPUT) != ERRCODE_SUCC) {
        return -1;
    }

    base = ws63_ws2812_gpio_base(pin);
    group_pin = (uint8_t)(pin % WS63_WS2812_GPIO_GROUP_WIDTH);
    group_offset = (uintptr_t)((pin / WS63_WS2812_GPIO_GROUP_WIDTH) * WS63_WS2812_GPIO_GROUP_STRIDE);
    g_ws2812.set_reg = (volatile uint32_t *)(base + group_offset + WS63_WS2812_GPIO_DATA_SET_OFFSET);
    g_ws2812.clr_reg = (volatile uint32_t *)(base + group_offset + WS63_WS2812_GPIO_DATA_CLR_OFFSET);
    g_ws2812.mask = (uint32_t)1U << group_pin;
    g_ws2812.pin = pin;
    g_ws2812.ready = 1U;
    ws63_ws2812_pin_low();
    (void)uapi_tcxo_delay_us(WS63_WS2812_RESET_US);
    return 0;
}

int ws63_ws2812_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t encoded[3];
    uint32_t irq_sts;

    if (g_ws2812.ready == 0U) {
        return -1;
    }

    ws63_ws2812_encode_grb(red, green, blue, encoded);
    irq_sts = osal_irq_lock();
    ws63_ws2812_write_byte(encoded[0]);
    ws63_ws2812_write_byte(encoded[1]);
    ws63_ws2812_write_byte(encoded[2]);
    ws63_ws2812_pin_low();
    osal_irq_restore(irq_sts);
    (void)uapi_tcxo_delay_us(WS63_WS2812_RESET_US);
    return 0;
}

int ws63_ws2812_clear(void)
{
    return ws63_ws2812_set_rgb(0U, 0U, 0U);
}

uint8_t ws63_ws2812_is_ready(void)
{
    return g_ws2812.ready;
}

uint8_t ws63_ws2812_pin(void)
{
    return g_ws2812.pin;
}
#endif
