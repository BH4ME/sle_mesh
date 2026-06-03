#ifndef WS63_ST7789_DISPLAY_H
#define WS63_ST7789_DISPLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t spi_bus;
    uint8_t sclk_pin;
    uint8_t mosi_pin;
    uint8_t cs_pin;
    uint8_t dc_pin;
    uint8_t reset_pin;
    uint16_t x_offset;
    uint16_t y_offset;
    uint16_t width;
    uint16_t height;
} ws63_st7789_config_t;

int ws63_st7789_init(const ws63_st7789_config_t *cfg);
int ws63_st7789_show_status(const char *role, const char *self, uint8_t online_count,
    uint8_t offline_count, uint8_t alert_count, const char *fw_version);
int ws63_st7789_show_alert(uint8_t member_id, int32_t latitude_e6, int32_t longitude_e6, uint32_t last_seen_s);
void ws63_st7789_tick(void);

#ifdef __cplusplus
}
#endif

#endif
