#include "sle_team_cli.h"
#include "sle_team_web_api.h"
#include "ws63_st7789_display.h"
#include "ws63_ws2812.h"
#include "ws63_console_pages.h"

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "app_init.h"
#include "common_def.h"
#include "adc.h"
#include "adc_porting.h"
#include "errcode.h"
#include "gpio.h"
#include "pinctrl.h"
#include "securec.h"
#include "soc_osal.h"
#include "tcxo.h"
#include "uart.h"
#include "watchdog.h"
#include "nv.h"

#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
#include "lwip/netifapi.h"
#include "lwip/sockets.h"
#include "lwip/tcpip.h"
#include "wifi_device.h"
#include "wifi_device_config.h"
#include "wifi_hotspot.h"
#include "wifi_hotspot_config.h"
#endif

#include "sle_errcode.h"
#include "sle_device_discovery.h"
#include "sle_team_packet.h"
#include "sle_ssap_server.h"
#include "sle_uart_server.h"
#include "sle_uart_server_adv.h"
#include "sle_ssap_client.h"
#include "sle_uart_client.h"

#ifndef CONFIG_SLE_TEAM_SELF_ID
#define CONFIG_SLE_TEAM_SELF_ID 1
#endif

#ifndef CONFIG_SLE_TEAM_LEADER_ID
#define CONFIG_SLE_TEAM_LEADER_ID 1
#endif

#ifndef CONFIG_SLE_TEAM_TEAM_ID
#define CONFIG_SLE_TEAM_TEAM_ID 1
#endif

#ifndef CONFIG_SLE_TEAM_CHANNEL_HASH
#define CONFIG_SLE_TEAM_CHANNEL_HASH 0x11
#endif

#ifndef CONFIG_SLE_TEAM_UART_BUS
#define CONFIG_SLE_TEAM_UART_BUS 0
#endif

#ifndef CONFIG_SLE_TEAM_UART_TXD_PIN
#define CONFIG_SLE_TEAM_UART_TXD_PIN 21
#endif

#ifndef CONFIG_SLE_TEAM_UART_RXD_PIN
#define CONFIG_SLE_TEAM_UART_RXD_PIN 22
#endif

#ifndef CONFIG_SLE_TEAM_LED_PIN
#define CONFIG_SLE_TEAM_LED_PIN 255
#endif

#ifndef CONFIG_SLE_TEAM_LED_ACTIVE_LOW
#define CONFIG_SLE_TEAM_LED_ACTIVE_LOW 0
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_ENABLE
#define CONFIG_SLE_TEAM_ST7789_ENABLE 1
#endif

#ifndef CONFIG_SLE_TEAM_DISPLAY_USE_LVGL
#define CONFIG_SLE_TEAM_DISPLAY_USE_LVGL 1
#endif

#ifndef CONFIG_SLE_TEAM_LVGL_DRAW_BUF_LINES
#define CONFIG_SLE_TEAM_LVGL_DRAW_BUF_LINES 8
#endif

#ifndef CONFIG_SLE_TEAM_WS2812_ENABLE
#define CONFIG_SLE_TEAM_WS2812_ENABLE 1
#endif

#ifndef CONFIG_SLE_TEAM_WS2812_PIN
#define CONFIG_SLE_TEAM_WS2812_PIN 0
#endif

#ifndef CONFIG_SLE_TEAM_BUZZER_ENABLE
#define CONFIG_SLE_TEAM_BUZZER_ENABLE 1
#endif

#ifndef CONFIG_SLE_TEAM_BUZZER_PIN
#define CONFIG_SLE_TEAM_BUZZER_PIN 14
#endif

#ifndef CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH
#define CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH 1
#endif

#ifndef CONFIG_SLE_TEAM_BUZZER_MUTED
#define CONFIG_SLE_TEAM_BUZZER_MUTED 1
#endif

#ifndef CONFIG_SLE_TEAM_BUZZER_AUTO_TOGGLE
#define CONFIG_SLE_TEAM_BUZZER_AUTO_TOGGLE 0
#endif

#ifndef CONFIG_SLE_TEAM_GPS_ENABLE
#define CONFIG_SLE_TEAM_GPS_ENABLE 0
#endif

#ifndef CONFIG_SLE_TEAM_GPS_UART_BUS
#define CONFIG_SLE_TEAM_GPS_UART_BUS 1
#endif

#ifndef CONFIG_SLE_TEAM_GPS_UART_TXD_PIN
#define CONFIG_SLE_TEAM_GPS_UART_TXD_PIN 17
#endif

#ifndef CONFIG_SLE_TEAM_GPS_UART_RXD_PIN
#define CONFIG_SLE_TEAM_GPS_UART_RXD_PIN 18
#endif

#ifndef CONFIG_SLE_TEAM_ADC_ENABLE
#define CONFIG_SLE_TEAM_ADC_ENABLE 1
#endif

#ifndef CONFIG_SLE_TEAM_ADC_CTRL_PIN
#define CONFIG_SLE_TEAM_ADC_CTRL_PIN 5
#endif

#ifndef CONFIG_SLE_TEAM_ADC_VBAT_PIN
#define CONFIG_SLE_TEAM_ADC_VBAT_PIN 12
#endif

#ifndef CONFIG_SLE_TEAM_ADC_CTRL_ACTIVE_HIGH
#define CONFIG_SLE_TEAM_ADC_CTRL_ACTIVE_HIGH 1
#endif

#ifndef CONFIG_SLE_TEAM_ADC_VBAT_CHANNEL
#define CONFIG_SLE_TEAM_ADC_VBAT_CHANNEL 5
#endif

#ifndef CONFIG_SLE_TEAM_ADC_SAMPLE_SETTLE_MS
#define CONFIG_SLE_TEAM_ADC_SAMPLE_SETTLE_MS 50
#endif

#ifndef CONFIG_SLE_TEAM_ADC_SAMPLE_INTERVAL_S
#define CONFIG_SLE_TEAM_ADC_SAMPLE_INTERVAL_S 30
#endif

#ifndef CONFIG_SLE_TEAM_CHRG_ENABLE
#define CONFIG_SLE_TEAM_CHRG_ENABLE 1
#endif

#ifndef CONFIG_SLE_TEAM_CHRG_PIN
#define CONFIG_SLE_TEAM_CHRG_PIN 2
#endif

#ifndef CONFIG_SLE_TEAM_CHRG_ACTIVE_LOW
#define CONFIG_SLE_TEAM_CHRG_ACTIVE_LOW 1
#endif

#ifndef CONFIG_SLE_TEAM_CHRG_EXTERNAL_PULLUP
#define CONFIG_SLE_TEAM_CHRG_EXTERNAL_PULLUP 1
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_SPI_BUS
#define CONFIG_SLE_TEAM_ST7789_SPI_BUS 0
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_SCLK_PIN
#define CONFIG_SLE_TEAM_ST7789_SCLK_PIN 7
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_MOSI_PIN
#define CONFIG_SLE_TEAM_ST7789_MOSI_PIN 9
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_CS_PIN
#define CONFIG_SLE_TEAM_ST7789_CS_PIN 8
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW
#define CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW 1
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_DC_PIN
#define CONFIG_SLE_TEAM_ST7789_DC_PIN 13
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_RESET_PIN
#define CONFIG_SLE_TEAM_ST7789_RESET_PIN 10
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_X_OFFSET
#define CONFIG_SLE_TEAM_ST7789_X_OFFSET 40
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_Y_OFFSET
#define CONFIG_SLE_TEAM_ST7789_Y_OFFSET 53
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_WIDTH
#define CONFIG_SLE_TEAM_ST7789_WIDTH 240
#endif

#ifndef CONFIG_SLE_TEAM_ST7789_HEIGHT
#define CONFIG_SLE_TEAM_ST7789_HEIGHT 135
#endif

#ifndef CONFIG_SLE_TEAM_HEARTBEAT_INTERVAL_S
#define CONFIG_SLE_TEAM_HEARTBEAT_INTERVAL_S 1
#endif

#ifndef CONFIG_SLE_TEAM_REPORT_INTERVAL_S
#define CONFIG_SLE_TEAM_REPORT_INTERVAL_S 5
#endif

#ifndef CONFIG_SLE_TEAM_WARN_DISTANCE_M
#define CONFIG_SLE_TEAM_WARN_DISTANCE_M 50
#endif

#ifndef CONFIG_SLE_TEAM_LOST_DISTANCE_M
#define CONFIG_SLE_TEAM_LOST_DISTANCE_M 80
#endif

#ifndef CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S
#define CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S 3
#endif

#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
#ifndef CONFIG_SLE_TEAM_WIFI_AP_AUTO_START
#define CONFIG_SLE_TEAM_WIFI_AP_AUTO_START 1
#endif

#ifndef CONFIG_SLE_TEAM_WIFI_AP_SSID
#define CONFIG_SLE_TEAM_WIFI_AP_SSID "SLE-TEAM-V4"
#endif

#ifndef CONFIG_SLE_TEAM_WIFI_AP_PSK
#define CONFIG_SLE_TEAM_WIFI_AP_PSK "123456789"
#endif

#ifndef CONFIG_SLE_TEAM_WIFI_AP_CHANNEL
#define CONFIG_SLE_TEAM_WIFI_AP_CHANNEL 6
#endif

#ifndef CONFIG_SLE_TEAM_WIFI_AP_IP_LAST
#define CONFIG_SLE_TEAM_WIFI_AP_IP_LAST 1
#endif
#endif

#define SLE_TEAM_APP_TASK_STACK_SIZE 0x1800
#define SLE_TEAM_APP_TASK_PRIO 28
#define SLE_TEAM_UART_BAUDRATE 115200
#define SLE_TEAM_UART_RX_BUF_SIZE 512
#define SLE_TEAM_CLI_LINE_SIZE 192
#define SLE_TEAM_CLI_QUEUE_LEN 4
#define SLE_TEAM_CLI_QUEUE_TIMEOUT_MS 20
#define SLE_TEAM_MAIN_LOOP_SLEEP_MS 5U
#define SLE_TEAM_DISPLAY_TASK_STACK_SIZE 0x1800
#define SLE_TEAM_DISPLAY_TASK_PRIO 31
#define SLE_TEAM_DISPLAY_TASK_INTERVAL_MS 20U
#define SLE_TEAM_LED_QUEUE_LEN 6
#define SLE_TEAM_LED_QUEUE_TIMEOUT_MS 1000
#define SLE_TEAM_LED_TASK_STACK_SIZE 0x800
#define SLE_TEAM_LED_TASK_PRIO 29
#define SLE_TEAM_LED_TX_ON_MS 40
#define SLE_TEAM_LED_TX_OFF_MS 40
#define SLE_TEAM_LED_TX_PULSES 2
#define SLE_TEAM_LED_RX_ON_MS 220
#define SLE_TEAM_LED_RX_OFF_MS 120
#define SLE_TEAM_LED_RX_PULSES 1
#define SLE_TEAM_LED_SEEK_ON_MS 18
#define SLE_TEAM_LED_SEEK_OFF_MS 80
#define SLE_TEAM_LED_SEEK_PULSES 1
#define SLE_TEAM_LED_SEEK_INTERVAL_MS 1000U
#define SLE_TEAM_WS2812_FORCE_CLEAR_COUNT 8U
#define SLE_TEAM_WS2812_FORCE_CLEAR_DELAY_MS 10U
#define SLE_TEAM_WS2812_TEST_R 64U
#define SLE_TEAM_WS2812_TEST_G 64U
#define SLE_TEAM_WS2812_TEST_B 64U
#define SLE_TEAM_WS2812_TEST_STEP_MS 120U
#define SLE_TEAM_WS2812_BOOT_R 0U
#define SLE_TEAM_WS2812_BOOT_G 6U
#define SLE_TEAM_WS2812_BOOT_B 16U
#define SLE_TEAM_WS2812_IDLE_R 16U
#define SLE_TEAM_WS2812_IDLE_G 16U
#define SLE_TEAM_WS2812_IDLE_B 16U
#define SLE_TEAM_WS2812_LEADER_R 0U
#define SLE_TEAM_WS2812_LEADER_G 16U
#define SLE_TEAM_WS2812_LEADER_B 0U
#define SLE_TEAM_WS2812_MEMBER_R 0U
#define SLE_TEAM_WS2812_MEMBER_G 0U
#define SLE_TEAM_WS2812_MEMBER_B 16U
#define SLE_TEAM_WS2812_SEEK_R 16U
#define SLE_TEAM_WS2812_SEEK_G 12U
#define SLE_TEAM_WS2812_SEEK_B 0U
#define SLE_TEAM_WS2812_TX_R 16U
#define SLE_TEAM_WS2812_TX_G 0U
#define SLE_TEAM_WS2812_TX_B 12U
#define SLE_TEAM_WS2812_RX_R 0U
#define SLE_TEAM_WS2812_RX_G 0U
#define SLE_TEAM_WS2812_RX_B 16U
#define SLE_TEAM_WS2812_WARN_R 16U
#define SLE_TEAM_WS2812_WARN_G 6U
#define SLE_TEAM_WS2812_WARN_B 0U
#define SLE_TEAM_WS2812_ERROR_R 16U
#define SLE_TEAM_WS2812_ERROR_G 0U
#define SLE_TEAM_WS2812_ERROR_B 0U
#define SLE_TEAM_WS2812_IDLE_BLINK_ON_MS 500U
#define SLE_TEAM_WS2812_IDLE_BLINK_PERIOD_MS 1600U
#define SLE_TEAM_WS2812_LEADER_BLINK_ON_MS 500U
#define SLE_TEAM_WS2812_LEADER_BLINK_PERIOD_MS 1000U
#define SLE_TEAM_WS2812_MEMBER_BLINK_ON_MS 500U
#define SLE_TEAM_WS2812_MEMBER_BLINK_PERIOD_MS 1000U
#define SLE_TEAM_WS2812_ERROR_BLINK_ON_MS 220U
#define SLE_TEAM_WS2812_ERROR_BLINK_PERIOD_MS 360U
#define SLE_TEAM_WS2812_FLASH_ON_MS 140U
#define SLE_TEAM_WS2812_FLASH_OFF_MS 80U
#define SLE_TEAM_WS2812_FLASH_PULSES 4U
#define SLE_TEAM_BUZZER_FORCE_OFF_LEVEL GPIO_LEVEL_LOW
#define SLE_TEAM_BUZZER_FORCE_ON_LEVEL GPIO_LEVEL_HIGH
#define SLE_TEAM_BUZZER_TOGGLE_INTERVAL_MS 3000U
#define SLE_TEAM_BUZZ_ON_MS 120U
#define SLE_TEAM_BUZZ_OFF_MS 120U

#define SLE_TEAM_WIFI_AP_TASK_STACK_SIZE 0x1000
#define SLE_TEAM_WIFI_AP_TASK_PRIO 13
#define SLE_TEAM_WIFI_IFNAME_MAX_SIZE 16
#define SLE_TEAM_WIFI_INIT_WAIT_MAX_MS 10000
#define SLE_TEAM_IDENTITY_WAIT_MAX_MS 4000
#define SLE_TEAM_TCPIP_INIT_WAIT_MAX_MS 10000
#define SLE_TEAM_WIFI_AP_RETRY_MS 5000
#define SLE_TEAM_HTTP_START_DELAY_MS 5000
#define SLE_TEAM_HTTP_RETRY_MS 5000
#define SLE_TEAM_HTTP_PORT 80
#define SLE_TEAM_HTTP_BACKLOG 4
#define SLE_TEAM_HTTP_RECV_TIMEOUT_MS 800
#define SLE_TEAM_HTTP_SEND_TIMEOUT_MS 3000
#define SLE_TEAM_FACTORY_RESET_TASK_STACK_SIZE 0x800
#define SLE_TEAM_FACTORY_RESET_TASK_PRIO 12
#define SLE_TEAM_HTTP_REQ_BUF_SIZE 768
#define SLE_TEAM_HTTP_JSON_BUF_SIZE 2048
#define SLE_TEAM_HTTP_HTML_BUF_SIZE 8192
#define SLE_TEAM_HTTP_PATH_BUF_SIZE 160
#define SLE_TEAM_ROUTE_ID_FALLBACK 1U
#define TEAM_CONN_TRACK_MAX 16U
#define TEAM_PENDING_CONN_MAX SLE_TEAM_MAX_DIRECT_CONNECTIONS
#define TEAM_ROUTE_ENTRY_MAX SLE_TEAM_MAX_MEMBERS
#define SLE_TEAM_PAIRING_ROTATE_INTERVAL_S 2U
#define SLE_TEAM_PAIRING_KEEP_CONNECTED 4U
#define SLE_TEAM_RELAY_MGMT_RAM_BUDGET_BYTES 512U
#define SLE_TEAM_RELAY_MGMT_EST_BYTES_PER_RELAY 64U
#define SLE_TEAM_RELAY_MGMT_HARD_MAX SLE_TEAM_MAX_DIRECT_CONNECTIONS
#define SLE_TEAM_RELAY_DENSE_MIN 2U
#define SLE_TEAM_PARENT_SWITCH_COOLDOWN_S 2U
#define SLE_TEAM_PARENT_SWITCH_RSSI_HYST_DBM 8
#define SLE_TEAM_PARENT_CANDIDATE_MIN_RSSI (-95)
#define SLE_TEAM_RELAY_REBALANCE_INTERVAL_S 1U
#define SLE_TEAM_RELAY_REVOKE_STALE_FACTOR 2U
#define SLE_TEAM_RELAY_CANDIDATE_MIN_RSSI (-92)
#define SLE_TEAM_RELAY_SWAP_RSSI_HYST_DBM 8
#define SLE_TEAM_RELAY_SWAP_STABLE_S 30U
#define SLE_TEAM_ROUTE_METRICS_INTERVAL_S 1U
#define SLE_TEAM_ROUTE_STALE_FACTOR 2U
#define SLE_TEAM_ROUTE_HINT_COOLDOWN_S 2U
#define SLE_TEAM_RELAY_FAILOVER_GRACE_S 6U
#define SLE_TEAM_RELAY_CONFIG_RETRY_INTERVAL_S 1U
#define SLE_TEAM_DIRECT_ENFORCE_INTERVAL_S 1U
#define SLE_TEAM_RELAY_CHILD_RESCAN_INTERVAL_S 2U
#define SLE_TEAM_PARENT_RESELECT_DISCONNECT_DELAY_S 1U
#define SLE_TEAM_MEMBER_UPSTREAM_RECOVER_INTERVAL_S 2U
#define SLE_TEAM_MEMBER_UPSTREAM_STUCK_S 8U
#define SLE_TEAM_NV_KEY_WEB_CONFIG 0x5001
#define SLE_TEAM_NV_KEY_ALLOWED_MEMBERS 0x5002
#define SLE_TEAM_NV_CONFIG_MAGIC 0x534C4554U
#define SLE_TEAM_NV_CONFIG_VERSION 1U
#define SLE_TEAM_NV_ALLOWED_MAGIC 0x534C414DU
#define SLE_TEAM_NV_ALLOWED_VERSION 1U
#define SLE_TEAM_MEMBER_RESCAN_INTERVAL_S 1U
#define SLE_TEAM_WIFI_SECURITY_COMPAT_MIX ((wifi_security_enum)WIFI_SEC_TYPE_WPA2_WPA_PSK_MIX)
#define SLE_TEAM_WIFI_PROTOCOL_COMPAT_AX WIFI_MODE_11B_G_N_AX
#if (SLE_TEAM_RELAY_MGMT_EST_BYTES_PER_RELAY == 0U)
#error "SLE_TEAM_RELAY_MGMT_EST_BYTES_PER_RELAY must be non-zero"
#endif

#define SLE_TEAM_FW_VERSION "v4.4.137"
#define SLE_TEAM_HW_CONSTRAINTS "v3.2 schematic pinmap, muted buzzer"
#define SLE_TEAM_DISPLAY_STATUS_MIN_INTERVAL_MS 500U
#define SLE_TEAM_ADC_DIVIDER_TOP_KOHM 390U
#define SLE_TEAM_ADC_DIVIDER_BOTTOM_KOHM 100U
#define SLE_TEAM_BATTERY_EMPTY_MV 3300U
#define SLE_TEAM_BATTERY_FULL_MV 4200U

#if defined(__GNUC__)
#define SLE_TEAM_UNUSED_FUNC __attribute__((unused))
#else
#define SLE_TEAM_UNUSED_FUNC
#endif

typedef struct {
    char line[SLE_TEAM_CLI_LINE_SIZE];
} sle_team_cli_msg_t;

typedef enum {
    SLE_TEAM_LED_EVENT_TX = 1,
    SLE_TEAM_LED_EVENT_RX = 2,
    SLE_TEAM_LED_EVENT_SEEK = 3,
} sle_team_led_event_t;

typedef enum {
    TEAM_RGB_STATE_OFF = 0,
    TEAM_RGB_STATE_BOOT,
    TEAM_RGB_STATE_IDLE,
    TEAM_RGB_STATE_LEADER,
    TEAM_RGB_STATE_MEMBER,
    TEAM_RGB_STATE_SEEK,
    TEAM_RGB_STATE_TX,
    TEAM_RGB_STATE_RX,
    TEAM_RGB_STATE_WARN,
    TEAM_RGB_STATE_ERROR,
} team_rgb_state_t;

typedef enum {
    TEAM_CONN_DIR_UNKNOWN = 0U,
    TEAM_CONN_DIR_UPSTREAM = 1U,
    TEAM_CONN_DIR_DOWNSTREAM = 2U,
} team_conn_dir_t;

typedef struct {
    uint8_t active;
    uint16_t conn_id;
    team_conn_dir_t dir;
    uint8_t route_id;
    uint8_t route_id_provisional;
} team_conn_track_t;

typedef struct {
    uint8_t active;
    sle_addr_t addr;
    uint8_t route_id;
    uint8_t route_id_provisional;
    uint32_t last_seen_s;
} team_pending_conn_t;

typedef struct {
    uint8_t active;
    uint8_t member_id;
    uint16_t conn_id;
    team_conn_dir_t dir;
    uint8_t next_hop_id;
    uint32_t last_seen_s;
} team_route_entry_t;

typedef enum {
    TEAM_DISPLAY_EVENT_NONE = 0,
    TEAM_DISPLAY_EVENT_JOIN,
    TEAM_DISPLAY_EVENT_LEFT,
    TEAM_DISPLAY_EVENT_TIMEOUT,
    TEAM_DISPLAY_EVENT_LOST,
    TEAM_DISPLAY_EVENT_REJOIN,
} team_display_event_t;

typedef enum {
    TEAM_DISPLAY_MEMBER_UNKNOWN = 0,
    TEAM_DISPLAY_MEMBER_ONLINE,
    TEAM_DISPLAY_MEMBER_OFFLINE,
} team_display_member_state_t;

static const char *team_display_event_name(team_display_event_t event)
{
    switch (event) {
        case TEAM_DISPLAY_EVENT_JOIN:
            return "JOIN";
        case TEAM_DISPLAY_EVENT_LEFT:
            return "LEFT";
        case TEAM_DISPLAY_EVENT_TIMEOUT:
            return "TIMEOUT";
        case TEAM_DISPLAY_EVENT_LOST:
            return "LOST";
        case TEAM_DISPLAY_EVENT_REJOIN:
            return "REJOIN";
        default:
            return "EVENT";
    }
}

typedef struct {
    uint8_t uart_rx_buf[SLE_TEAM_UART_RX_BUF_SIZE];
    char line_buf[SLE_TEAM_CLI_LINE_SIZE];
    char self_label[8];
    char leader_label[8];
#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
    char softap_ssid[32];
    uint8_t self_mac[6];
    uint8_t self_mac_ready;
#endif
    uint16_t line_len;
    unsigned long cli_queue_id;
    unsigned long led_queue_id;
    uint8_t cli_queue_ready;
    uint8_t led_queue_ready;
    uint8_t led_pin;
    uint8_t led_active_low;
    uint8_t ws2812_ready;
    uint8_t ws2812_pin;
    uint8_t ws2812_r;
    uint8_t ws2812_g;
    uint8_t ws2812_b;
    uint8_t ws2812_state;
    uint32_t ws2812_state_last_ms;
    uint8_t ws2812_base_state;
    uint32_t ws2812_base_enter_ms;
    uint8_t ws2812_flash_state;
    uint8_t ws2812_flash_pulses_left;
    uint8_t ws2812_flash_on;
    uint32_t ws2812_flash_step_ms;
    uint8_t buzzer_ready;
    uint8_t buzzer_pin;
    uint8_t buzzer_active_high;
    uint8_t buzzer_level_on;
    uint32_t buzzer_toggle_last_ms;
    uint8_t gps_ready;
    uint8_t gps_uart_bus;
    uint8_t gps_txd_pin;
    uint8_t gps_rxd_pin;
    uint8_t adc_ready;
    uint8_t adc_ctrl_pin;
    uint8_t adc_vbat_pin;
    uint8_t adc_ctrl_active_high;
    uint8_t adc_vbat_channel;
    uint8_t chrg_ready;
    uint8_t chrg_pin;
    uint8_t chrg_active_low;
    uint8_t chrg_raw;
    uint8_t charging;
    uint8_t battery_valid;
    uint8_t battery_percent;
    uint16_t adc_sample_mv;
    uint16_t battery_mv;
    uint32_t battery_sample_last_ms;
    int32_t battery_sample_last_ret;
    uint8_t display_ready;
    uint8_t display_task_started;
    volatile uint8_t display_status_dirty;
    volatile uint8_t display_event_dirty;
    uint8_t display_event_count;
    uint32_t display_status_last_flush_ms;
    char display_status_role[16];
    char display_status_self[8];
    uint8_t display_status_online_count;
    uint8_t display_status_offline_count;
    uint8_t display_status_event_count;
    uint8_t display_event_member_id;
    uint8_t display_event_type;
    char display_event_label[8];
    int32_t display_event_latitude_e6;
    int32_t display_event_longitude_e6;
    uint32_t display_event_last_seen_s;
    uint8_t display_member_ids[SLE_TEAM_MAX_MEMBERS];
    uint8_t display_member_states[SLE_TEAM_MAX_MEMBERS];
    uint8_t display_member_last_events[SLE_TEAM_MAX_MEMBERS];
    char display_member_labels[SLE_TEAM_MAX_MEMBERS][8];
    uint32_t last_seek_led_ms;
    uint8_t route_id;
    uint8_t role_configured;
    uint8_t sle_started;
    uint8_t relay_client_started;
    volatile uint8_t role_request_pending;
    uint8_t role_request_role;
    uint8_t role_request_leader;
    uint8_t role_request_team;
    uint8_t role_request_channel;
    uint8_t role_request_save_nv;
    uint16_t role_request_leader_suffix;
    int role_request_last_ret;
    uint32_t member_rescan_last_s;
    uint32_t pairing_rotate_last_s;
    uint8_t pairing_rotate_index;
    uint32_t parent_switch_last_s;
    int8_t parent_selected_rssi;
    uint32_t relay_rebalance_last_s;
    uint32_t relay_child_rescan_last_s;
    uint32_t direct_enforce_last_s;
    uint8_t relay_online_count;
    uint8_t relay_target_count;
    uint8_t relay_budget_count;
    uint8_t relay_swap_candidate_id;
    uint8_t relay_swap_victim_id;
    uint32_t relay_swap_since_s;
    uint32_t route_metrics_last_s;
    uint8_t route_metrics_active;
    uint8_t route_metrics_direct;
    uint8_t route_metrics_relayed;
    uint8_t route_metrics_unreachable;
    uint8_t route_metrics_stale;
    uint8_t route_metrics_converged;
    uint8_t last_online_member_count;
    uint32_t route_metrics_epoch;
    uint32_t route_metrics_last_change_s;
    uint32_t route_metrics_last_converged_s;
    uint32_t route_hint_sent_total;
    uint32_t route_hint_failed_total;
    uint32_t route_hint_cooldown_skipped_total;
    uint32_t route_update_rx_total;
    uint32_t route_reparent_total;
    uint32_t route_reparent_last_s;
    uint8_t relay_failover_active;
    uint8_t relay_failover_lost_id;
    uint32_t relay_failover_start_s;
    uint32_t relay_config_retry_last_s;
    uint32_t parent_reselect_disconnect_last_s;
    uint32_t member_upstream_recover_last_s;
    uint32_t member_upstream_not_ready_since_s;
    uint16_t member_upstream_not_ready_count;
    uint8_t relay_failover_member_ids[SLE_TEAM_MAX_MEMBERS];
    uint8_t relay_config_pending_member_ids[SLE_TEAM_MAX_MEMBERS];
    uint16_t direct_prune_conn_ids[SLE_TEAM_MAX_DIRECT_CONNECTIONS];
    uint8_t direct_prune_member_ids[SLE_TEAM_MAX_DIRECT_CONNECTIONS];
    uint8_t route_hint_member_ids[SLE_TEAM_MAX_MEMBERS];
    uint8_t route_hint_parent_ids[SLE_TEAM_MAX_MEMBERS];
    uint32_t route_hint_last_sent_s[SLE_TEAM_MAX_MEMBERS];
} sle_team_ws63_runtime_t;

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t role;
    uint8_t team_id;
    uint8_t channel_hash;
    uint16_t leader_suffix;
    uint16_t checksum;
} sle_team_web_config_nv_t;

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t member_filter_enabled;
    uint8_t allowed_member_count;
    uint8_t reserved;
    uint8_t allowed_member_ids[SLE_TEAM_MAX_MEMBERS];
    uint16_t checksum;
} sle_team_allowed_members_nv_t;

static sle_team_ws63_runtime_t g_team_rt;
static sle_team_node_t g_team_node;
static sle_team_cli_t g_team_cli;
static sle_team_web_event_log_t g_team_events;
static sle_connection_callbacks_t g_team_conn_cbks = { 0 };
static team_conn_track_t g_team_conn_tracks[TEAM_CONN_TRACK_MAX];
static team_pending_conn_t g_team_pending_conns[TEAM_PENDING_CONN_MAX];
static team_route_entry_t g_team_routes[TEAM_ROUTE_ENTRY_MAX];

#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
static char g_team_http_req_buf[SLE_TEAM_HTTP_REQ_BUF_SIZE];
static char g_team_http_json_buf[SLE_TEAM_HTTP_JSON_BUF_SIZE];
static char g_team_http_html_buf[SLE_TEAM_HTTP_HTML_BUF_SIZE];
static volatile int g_team_http_listen_fd = -1;
static volatile int g_team_http_last_errno = 0;
static volatile uint32_t g_team_http_accept_count = 0;
static volatile uint8_t g_team_http_ready = 0;
#endif

static uart_buffer_config_t g_uart_buffer_config = {
    .rx_buffer = g_team_rt.uart_rx_buf,
    .rx_buffer_size = SLE_TEAM_UART_RX_BUF_SIZE,
};

static int team_configure_role(sle_team_node_role_t role, uint8_t leader_id);
static int team_request_role_config(sle_team_node_role_t role, uint8_t leader_id, uint8_t team_id,
    uint8_t channel_hash, uint16_t leader_suffix, uint8_t save_nv);
static uint16_t team_self_mac_suffix(void);
static void team_reboot_schedule(const char *reason);
static uint8_t team_route_id_from_suffix(uint16_t suffix);
static void team_chrg_init(void);
static void team_chrg_sample(void);
static const char *team_power_source_name(void);
static uint8_t team_power_source_certain(void);
static void team_handle_role_request_once(void);
static void team_register_connection_callbacks(void);
static int team_nv_allowed_apply_to_node(void);
static int team_nv_allowed_save_from_node(void);
static void team_upstream_parent_note(uint8_t parent_id, sle_team_parent_state_t state, const char *reason);
static void team_upstream_parent_reset(const char *reason);
static uint8_t team_display_member_index_locked(uint8_t member_id, uint8_t create, uint8_t *out_index);
static uint8_t team_leader_route_physical_parent_matches(const team_route_entry_t *route, uint8_t parent_id);
static uint8_t team_leader_failover_window_active(uint32_t now_s);
static uint8_t team_leader_failover_should_preserve_route(uint8_t member_id, uint8_t next_hop_id);
static void team_leader_failover_note_route_recovered(uint8_t member_id, uint8_t next_hop_id);
static uint8_t team_leader_should_seek_member(uint8_t candidate_id, uint8_t candidate_bucket);
static uint8_t team_should_defer_member_timeout(void *user_ctx, uint8_t member_id,
    uint32_t now_s, uint32_t last_seen_s);
static uint8_t team_interval_not_reached(uint32_t now_s, uint32_t last_s, uint32_t interval_s);
static const char *team_rgb_state_name(team_rgb_state_t state);
static void team_ws2812_set_state(team_rgb_state_t state);
static void team_ws2812_refresh_network_state(void);
static void team_ws2812_show_display_event(team_display_event_t event);
static void team_leader_relay_config_retry_tick(void);
static void team_leader_pairing_restart_scan(const char *reason);
static void team_leader_pairing_rotate_connections(void);
static void team_leader_enforce_direct_capacity(uint8_t force_now);
static void team_leader_auto_approve_pending(void);
static void team_member_upstream_recover_tick(void);
static void team_member_autoselect_parent(void);
static void team_member_drop_relay_children(const char *reason);
static uint8_t team_leader_direct_capacity(void);
static uint8_t team_leader_relay_budget(void);
static void team_leader_rebalance_relays(uint8_t force_now);
static void team_leader_route_metrics_update(void);
static void team_leader_route_convergence_hint(uint32_t now_s, uint8_t trigger_state_change,
    uint8_t stale_count, uint8_t unreachable_count);
static void team_identity_format_route_label(uint8_t node_id, uint8_t role, const uint8_t mac[6], uint8_t mac_ready,
    char *out, size_t out_size);
static uint32_t team_now_s(void *user_ctx);
static void *team_display_task(const char *arg);
static void team_display_start(void);
static void *team_network_task(const char *arg);

static uint16_t team_nv_checksum(const sle_team_web_config_nv_t *cfg)
{
    const uint8_t *bytes = (const uint8_t *)cfg;
    uint16_t sum = 0x5A5AU;
    size_t i;

    if (cfg == NULL) {
        return 0U;
    }
    for (i = 0U; i < offsetof(sle_team_web_config_nv_t, checksum); i++) {
        sum = (uint16_t)((sum << 5) | (sum >> 11));
        sum ^= bytes[i];
    }
    return sum;
}

static uint8_t team_nv_config_valid(const sle_team_web_config_nv_t *cfg)
{
    if (cfg == NULL) {
        return 0U;
    }
    if (cfg->magic != SLE_TEAM_NV_CONFIG_MAGIC || cfg->version != SLE_TEAM_NV_CONFIG_VERSION) {
        return 0U;
    }
    if (cfg->role != (uint8_t)SLE_TEAM_ROLE_LEADER && cfg->role != (uint8_t)SLE_TEAM_ROLE_MEMBER) {
        return 0U;
    }
    if (cfg->team_id == 0U || cfg->team_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    if (cfg->role == (uint8_t)SLE_TEAM_ROLE_MEMBER && cfg->leader_suffix == 0U) {
        return 0U;
    }
    return cfg->checksum == team_nv_checksum(cfg) ? 1U : 0U;
}

static int team_nv_config_save(sle_team_node_role_t role, uint8_t team_id, uint16_t leader_suffix,
    uint8_t channel_hash)
{
    sle_team_web_config_nv_t cfg;
    errcode_t ret;
    errcode_t flush_ret = ERRCODE_SUCC;

    (void)memset_s(&cfg, sizeof(cfg), 0, sizeof(cfg));
    cfg.magic = SLE_TEAM_NV_CONFIG_MAGIC;
    cfg.version = SLE_TEAM_NV_CONFIG_VERSION;
    cfg.role = (uint8_t)role;
    cfg.team_id = team_id;
    cfg.channel_hash = channel_hash;
    cfg.leader_suffix = leader_suffix;
    cfg.checksum = team_nv_checksum(&cfg);

    ret = uapi_nv_write(SLE_TEAM_NV_KEY_WEB_CONFIG, (const uint8_t *)&cfg, (uint16_t)sizeof(cfg));
    if (ret == ERRCODE_SUCC) {
        flush_ret = uapi_nv_flush();
    }
    osal_printk("[team-nv] save role=%u team=%u leader_suffix=%04X channel=%u ret=0x%x flush=0x%x\r\n",
        cfg.role, cfg.team_id, cfg.leader_suffix, cfg.channel_hash, ret, flush_ret);
    /* Some WS63 NV backends return a flush warning after a successful write. */
    return ret == ERRCODE_SUCC ? SLE_TEAM_OK : SLE_TEAM_ERR_UNSUPPORTED;
}

static int team_nv_config_load(sle_team_web_config_nv_t *cfg)
{
    uint16_t len = 0U;
    errcode_t ret;

    if (cfg == NULL) {
        return SLE_TEAM_ERR_FORMAT;
    }
    (void)memset_s(cfg, sizeof(*cfg), 0, sizeof(*cfg));
    ret = uapi_nv_read(SLE_TEAM_NV_KEY_WEB_CONFIG, (uint16_t)sizeof(*cfg), &len, (uint8_t *)cfg);
    if (ret != ERRCODE_SUCC || len != sizeof(*cfg) || team_nv_config_valid(cfg) == 0U) {
        osal_printk("[team-nv] no valid web config ret=0x%x len=%u\r\n", ret, len);
        return SLE_TEAM_ERR_FORMAT;
    }
    osal_printk("[team-nv] load role=%u team=%u leader_suffix=%04X channel=%u\r\n",
        cfg->role, cfg->team_id, cfg->leader_suffix, cfg->channel_hash);
    return SLE_TEAM_OK;
}

static int team_nv_config_clear(void)
{
    sle_team_web_config_nv_t blank;
    errcode_t ret;
    errcode_t flush_ret = ERRCODE_SUCC;

    (void)memset_s(&blank, sizeof(blank), 0, sizeof(blank));
    ret = uapi_nv_write(SLE_TEAM_NV_KEY_WEB_CONFIG, (const uint8_t *)&blank, (uint16_t)sizeof(blank));
    if (ret == ERRCODE_SUCC) {
        flush_ret = uapi_nv_flush();
    }
    osal_printk("[team-nv] clear web config ret=0x%x flush=0x%x\r\n", ret, flush_ret);
    return ret == ERRCODE_SUCC ? SLE_TEAM_OK : SLE_TEAM_ERR_UNSUPPORTED;
}

static uint16_t team_nv_allowed_checksum(const sle_team_allowed_members_nv_t *cfg)
{
    const uint8_t *bytes = (const uint8_t *)cfg;
    uint16_t sum = 0x4D41U;
    size_t i;

    if (cfg == NULL) {
        return 0U;
    }
    for (i = 0U; i < offsetof(sle_team_allowed_members_nv_t, checksum); i++) {
        sum = (uint16_t)((sum << 5) | (sum >> 11));
        sum ^= bytes[i];
    }
    return sum;
}

static uint8_t team_nv_allowed_valid(const sle_team_allowed_members_nv_t *cfg)
{
    uint8_t i;

    if (cfg == NULL) {
        return 0U;
    }
    if (cfg->magic != SLE_TEAM_NV_ALLOWED_MAGIC || cfg->version != SLE_TEAM_NV_ALLOWED_VERSION) {
        return 0U;
    }
    if (cfg->member_filter_enabled > 1U || cfg->allowed_member_count > SLE_TEAM_MAX_MEMBERS) {
        return 0U;
    }
    for (i = 0U; i < cfg->allowed_member_count; i++) {
        if (cfg->allowed_member_ids[i] == 0U || cfg->allowed_member_ids[i] == SLE_TEAM_BROADCAST_ID) {
            return 0U;
        }
    }
    return cfg->checksum == team_nv_allowed_checksum(cfg) ? 1U : 0U;
}

static int team_nv_allowed_load(sle_team_allowed_members_nv_t *cfg)
{
    uint16_t len = 0U;
    errcode_t ret;

    if (cfg == NULL) {
        return SLE_TEAM_ERR_FORMAT;
    }
    (void)memset_s(cfg, sizeof(*cfg), 0, sizeof(*cfg));
    ret = uapi_nv_read(SLE_TEAM_NV_KEY_ALLOWED_MEMBERS, (uint16_t)sizeof(*cfg), &len, (uint8_t *)cfg);
    if (ret != ERRCODE_SUCC || len != sizeof(*cfg) || team_nv_allowed_valid(cfg) == 0U) {
        osal_printk("[team-nv] no valid allowlist ret=0x%x len=%u\r\n", ret, len);
        return SLE_TEAM_ERR_FORMAT;
    }
    osal_printk("[team-nv] load allow filter=%u count=%u\r\n",
        cfg->member_filter_enabled, cfg->allowed_member_count);
    return SLE_TEAM_OK;
}

static int team_nv_allowed_save_from_node(void)
{
    sle_team_allowed_members_nv_t cfg;
    errcode_t ret;
    errcode_t flush_ret = ERRCODE_SUCC;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    (void)memset_s(&cfg, sizeof(cfg), 0, sizeof(cfg));
    cfg.magic = SLE_TEAM_NV_ALLOWED_MAGIC;
    cfg.version = SLE_TEAM_NV_ALLOWED_VERSION;
    cfg.member_filter_enabled = g_team_node.cfg.member_filter_enabled != 0U ? 1U : 0U;
    cfg.allowed_member_count = g_team_node.cfg.allowed_member_count;
    (void)memcpy_s(cfg.allowed_member_ids, sizeof(cfg.allowed_member_ids),
        g_team_node.cfg.allowed_member_ids, g_team_node.cfg.allowed_member_count);
    cfg.checksum = team_nv_allowed_checksum(&cfg);

    ret = uapi_nv_write(SLE_TEAM_NV_KEY_ALLOWED_MEMBERS, (const uint8_t *)&cfg, (uint16_t)sizeof(cfg));
    if (ret == ERRCODE_SUCC) {
        flush_ret = uapi_nv_flush();
    }
    osal_printk("[team-nv] save allow filter=%u count=%u ret=0x%x flush=0x%x\r\n",
        cfg.member_filter_enabled, cfg.allowed_member_count, ret, flush_ret);
    return ret == ERRCODE_SUCC ? SLE_TEAM_OK : SLE_TEAM_ERR_UNSUPPORTED;
}

static int team_nv_allowed_clear(void)
{
    sle_team_allowed_members_nv_t blank;
    errcode_t ret;
    errcode_t flush_ret = ERRCODE_SUCC;

    (void)memset_s(&blank, sizeof(blank), 0, sizeof(blank));
    ret = uapi_nv_write(SLE_TEAM_NV_KEY_ALLOWED_MEMBERS, (const uint8_t *)&blank, (uint16_t)sizeof(blank));
    if (ret == ERRCODE_SUCC) {
        flush_ret = uapi_nv_flush();
    }
    osal_printk("[team-nv] clear allowlist ret=0x%x flush=0x%x\r\n", ret, flush_ret);
    return ret == ERRCODE_SUCC ? SLE_TEAM_OK : SLE_TEAM_ERR_UNSUPPORTED;
}

static int team_nv_allowed_apply_to_node(void)
{
    sle_team_allowed_members_nv_t cfg;
    int ret;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    ret = team_nv_allowed_load(&cfg);
    if (ret != SLE_TEAM_OK) {
        int fallback_ret = sle_team_node_allow_all_members(&g_team_node);
        osal_printk("[team-nv] allowlist invalid, fallback allow-all ret=%d\r\n", fallback_ret);
        return fallback_ret;
    }
    if (cfg.member_filter_enabled == 0U) {
        ret = sle_team_node_allow_all_members(&g_team_node);
    } else if (cfg.allowed_member_count == 0U) {
        /*
         * Empty allowlist is treated as "allow all" to avoid lockout after
         * reboot when no explicit member list has been provisioned yet.
         */
        ret = sle_team_node_allow_all_members(&g_team_node);
    } else {
        ret = sle_team_node_set_allowed_members(&g_team_node, cfg.allowed_member_ids, cfg.allowed_member_count);
    }
    osal_printk("[team-nv] apply allow filter=%u count=%u ret=%d\r\n",
        cfg.member_filter_enabled, cfg.allowed_member_count, ret);
    return ret;
}

static int team_cfg_clear_all_saved(void)
{
    int config_ret = team_nv_config_clear();
    int allow_ret = team_nv_allowed_clear();

    if (g_team_rt.role_configured != 0U && g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        (void)sle_team_node_allow_all_members(&g_team_node);
    }
    return (config_ret == SLE_TEAM_OK && allow_ret == SLE_TEAM_OK) ? SLE_TEAM_OK : SLE_TEAM_ERR_UNSUPPORTED;
}

static void team_print(const char *text)
{
    osal_printk("[state] %s\r\n", text);
}

static uint8_t team_online_member_count(void)
{
    uint8_t i;
    uint8_t count = 0U;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return g_team_node.joined != 0U ? 1U : 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (g_team_node.members[i].online != 0U) {
            count++;
        }
    }
    return count;
}

static uint8_t team_offline_member_count(void)
{
    uint8_t i;
    uint8_t count = 0U;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return g_team_node.joined != 0U ? 0U : 1U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];

        if (member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        if (member->online == 0U) {
            count++;
        }
    }
    return count;
}

static void team_display_refresh_status(void)
{
#if CONFIG_SLE_TEAM_ST7789_ENABLE
    uint32_t irq_sts;
    const char *role = "idle";
    char status_role[16];
    char status_self[8];
    uint8_t online_count;
    uint8_t offline_count;

    if (g_team_rt.role_configured != 0U) {
        role = g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER ? "leader" : "member";
    }
    online_count = team_online_member_count();
    offline_count = team_offline_member_count();
    (void)snprintf(status_role, sizeof(status_role), "%s", role);
    (void)snprintf(status_self, sizeof(status_self), "%s", g_team_rt.self_label);

    irq_sts = osal_irq_lock();
    (void)snprintf(g_team_rt.display_status_role, sizeof(g_team_rt.display_status_role), "%s", status_role);
    (void)snprintf(g_team_rt.display_status_self, sizeof(g_team_rt.display_status_self), "%s", status_self);
    g_team_rt.display_status_online_count = online_count;
    g_team_rt.display_status_offline_count = offline_count;
    g_team_rt.display_status_event_count = g_team_rt.display_event_count;
    g_team_rt.display_status_dirty = 1U;
    osal_irq_restore(irq_sts);
#endif
}

static const sle_team_member_record_t *team_display_find_member_record(uint8_t member_id)
{
    uint8_t i;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return NULL;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];

        if (member->member_id == member_id) {
            return member;
        }
    }
    return NULL;
}

static const sle_team_pending_member_t *team_display_find_pending_member_record(uint8_t member_id)
{
    uint8_t i;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return NULL;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_pending_member_t *member = &g_team_node.pending_members[i];

        if (member->active != 0U && member->member_id == member_id) {
            return member;
        }
    }
    return NULL;
}

static void team_display_cache_member_label(uint8_t member_id, const char *label)
{
#if CONFIG_SLE_TEAM_ST7789_ENABLE
    uint32_t irq_sts;
    uint8_t index = 0U;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID || label == NULL || label[0] == '\0') {
        return;
    }
    irq_sts = osal_irq_lock();
    if (team_display_member_index_locked(member_id, 1U, &index) != 0U) {
        (void)snprintf(g_team_rt.display_member_labels[index], sizeof(g_team_rt.display_member_labels[index]),
            "%s", label);
    }
    osal_irq_restore(irq_sts);
#else
    unused(member_id);
    unused(label);
#endif
}

static uint8_t team_display_get_cached_member_label(uint8_t member_id, char *out, size_t out_size)
{
#if CONFIG_SLE_TEAM_ST7789_ENABLE
    uint32_t irq_sts;
    uint8_t index = 0U;
    uint8_t found;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID || out == NULL || out_size == 0U) {
        return 0U;
    }
    irq_sts = osal_irq_lock();
    found = team_display_member_index_locked(member_id, 0U, &index);
    if (found != 0U && g_team_rt.display_member_labels[index][0] != '\0') {
        (void)snprintf(out, out_size, "%s", g_team_rt.display_member_labels[index]);
    } else {
        found = 0U;
    }
    osal_irq_restore(irq_sts);
    return found;
#else
    unused(member_id);
    unused(out);
    unused(out_size);
    return 0U;
#endif
}

static void team_display_format_member_label(uint8_t member_id, char *out, size_t out_size)
{
    const sle_team_member_record_t *member = team_display_find_member_record(member_id);
    const sle_team_pending_member_t *pending;

    if (member != NULL) {
        team_identity_format_route_label(member->member_id, member->role, member->mac, member->mac_ready,
            out, out_size);
        if (member->mac_ready != 0U) {
            team_display_cache_member_label(member_id, out);
        } else if (team_display_get_cached_member_label(member_id, out, out_size) != 0U) {
            return;
        }
        return;
    }
    pending = team_display_find_pending_member_record(member_id);
    if (pending != NULL && pending->mac_ready != 0U) {
        team_identity_format_route_label(pending->member_id, pending->role, pending->mac, pending->mac_ready,
            out, out_size);
        team_display_cache_member_label(member_id, out);
        return;
    }
    if (team_display_get_cached_member_label(member_id, out, out_size) != 0U) {
        return;
    }
    team_identity_format_route_label(member_id, (uint8_t)SLE_TEAM_ROLE_MEMBER, NULL, 0U, out, out_size);
}

static uint8_t team_display_member_index_locked(uint8_t member_id, uint8_t create, uint8_t *out_index)
{
    uint8_t i;
    uint8_t free_index = SLE_TEAM_MAX_MEMBERS;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID || out_index == NULL) {
        return 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (g_team_rt.display_member_ids[i] == member_id) {
            *out_index = i;
            return 1U;
        }
        if (g_team_rt.display_member_ids[i] == 0U && free_index == SLE_TEAM_MAX_MEMBERS) {
            free_index = i;
        }
    }
    if (create == 0U || free_index == SLE_TEAM_MAX_MEMBERS) {
        return 0U;
    }
    g_team_rt.display_member_ids[free_index] = member_id;
    g_team_rt.display_member_states[free_index] = (uint8_t)TEAM_DISPLAY_MEMBER_UNKNOWN;
    g_team_rt.display_member_last_events[free_index] = (uint8_t)TEAM_DISPLAY_EVENT_NONE;
    g_team_rt.display_member_labels[free_index][0] = '\0';
    *out_index = free_index;
    return 1U;
}

static team_display_event_t team_display_note_member_joined(uint8_t member_id)
{
    uint32_t irq_sts;
    uint8_t index = 0U;
    uint8_t prev_state;
    team_display_event_t event;

    irq_sts = osal_irq_lock();
    if (team_display_member_index_locked(member_id, 1U, &index) == 0U) {
        osal_irq_restore(irq_sts);
        return TEAM_DISPLAY_EVENT_JOIN;
    }
    prev_state = g_team_rt.display_member_states[index];
    if (prev_state == (uint8_t)TEAM_DISPLAY_MEMBER_ONLINE) {
        osal_irq_restore(irq_sts);
        return TEAM_DISPLAY_EVENT_NONE;
    }
    event = prev_state == (uint8_t)TEAM_DISPLAY_MEMBER_OFFLINE ?
        TEAM_DISPLAY_EVENT_REJOIN : TEAM_DISPLAY_EVENT_JOIN;
    g_team_rt.display_member_states[index] = (uint8_t)TEAM_DISPLAY_MEMBER_ONLINE;
    g_team_rt.display_member_last_events[index] = (uint8_t)event;
    osal_irq_restore(irq_sts);
    return event;
}

static uint8_t team_display_member_last_event(uint8_t member_id, uint8_t *event)
{
    uint32_t irq_sts;
    uint8_t index = 0U;
    uint8_t found;

    if (event == NULL) {
        return 0U;
    }
    *event = (uint8_t)TEAM_DISPLAY_EVENT_NONE;
    irq_sts = osal_irq_lock();
    found = team_display_member_index_locked(member_id, 0U, &index);
    if (found != 0U) {
        *event = g_team_rt.display_member_last_events[index];
    }
    osal_irq_restore(irq_sts);
    return found;
}

static void team_display_note_member_offline(uint8_t member_id, team_display_event_t event)
{
    uint32_t irq_sts;
    uint8_t index = 0U;

    irq_sts = osal_irq_lock();
    if (team_display_member_index_locked(member_id, 1U, &index) != 0U) {
        g_team_rt.display_member_states[index] = (uint8_t)TEAM_DISPLAY_MEMBER_OFFLINE;
        g_team_rt.display_member_last_events[index] = (uint8_t)event;
    }
    osal_irq_restore(irq_sts);
}

static void team_display_show_event(team_display_event_t event, uint8_t member_id, int32_t latitude_e6,
    int32_t longitude_e6, uint32_t last_seen_s)
{
#if CONFIG_SLE_TEAM_ST7789_ENABLE
    uint32_t irq_sts;
    uint8_t same_event;
    char label[8];

    team_display_format_member_label(member_id, label, sizeof(label));
    irq_sts = osal_irq_lock();
    same_event = (g_team_rt.display_event_member_id == member_id &&
        g_team_rt.display_event_type == (uint8_t)event &&
        g_team_rt.display_event_last_seen_s == last_seen_s &&
        g_team_rt.display_event_latitude_e6 == latitude_e6 &&
        g_team_rt.display_event_longitude_e6 == longitude_e6 &&
        strcmp(g_team_rt.display_event_label, label) == 0) ? 1U : 0U;
    if (same_event == 0U) {
        g_team_rt.display_event_count++;
    }
    g_team_rt.display_status_event_count = g_team_rt.display_event_count;
    g_team_rt.display_event_member_id = member_id;
    g_team_rt.display_event_type = (uint8_t)event;
    (void)snprintf(g_team_rt.display_event_label, sizeof(g_team_rt.display_event_label), "%s", label);
    g_team_rt.display_event_latitude_e6 = latitude_e6;
    g_team_rt.display_event_longitude_e6 = longitude_e6;
    g_team_rt.display_event_last_seen_s = last_seen_s;
    g_team_rt.display_event_dirty = 1U;
    g_team_rt.display_status_dirty = 1U;
    osal_irq_restore(irq_sts);
#else
    unused(member_id);
    unused(latitude_e6);
    unused(longitude_e6);
    unused(last_seen_s);
#endif
    team_ws2812_show_display_event(event);
}

static void team_display_note_offline_delta(void)
{
    uint8_t online_now;
    uint8_t i;
    const sle_team_member_record_t *latest_lost = NULL;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        g_team_rt.last_online_member_count = team_online_member_count();
        return;
    }
    online_now = team_online_member_count();
    if (online_now >= g_team_rt.last_online_member_count) {
        g_team_rt.last_online_member_count = online_now;
        return;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];

        if (member->online != 0U || member->member_id == 0U) {
            continue;
        }
        if (latest_lost == NULL || member->last_seen_s > latest_lost->last_seen_s) {
            latest_lost = member;
        }
    }
    if (latest_lost != NULL) {
        team_display_note_member_offline(latest_lost->member_id, TEAM_DISPLAY_EVENT_TIMEOUT);
        team_display_show_event(TEAM_DISPLAY_EVENT_TIMEOUT, latest_lost->member_id, latest_lost->latitude_e6,
            latest_lost->longitude_e6, latest_lost->last_seen_s);
    }
    g_team_rt.last_online_member_count = online_now;
}

static void team_display_flush_pending_once(void)
{
#if CONFIG_SLE_TEAM_ST7789_ENABLE
    uint8_t do_status;
    uint8_t do_event;
    uint8_t event_type = (uint8_t)TEAM_DISPLAY_EVENT_NONE;
    uint8_t event_member_id = 0U;
    char event_label[8] = {0};
    char status_role[16] = {0};
    char status_self[8] = {0};
    int32_t event_lat = 0;
    int32_t event_lon = 0;
    uint32_t event_last_seen = 0U;
    uint8_t status_online = 0U;
    uint8_t status_offline = 0U;
    uint8_t status_event_count = 0U;
    uint32_t irq_sts;
    uint32_t now_ms;
    int display_ret;

    if (g_team_rt.display_ready == 0U) {
        return;
    }

    irq_sts = osal_irq_lock();
    do_status = g_team_rt.display_status_dirty;
    if (do_status != 0U) {
        (void)snprintf(status_role, sizeof(status_role), "%s", g_team_rt.display_status_role);
        (void)snprintf(status_self, sizeof(status_self), "%s", g_team_rt.display_status_self);
        status_online = g_team_rt.display_status_online_count;
        status_offline = g_team_rt.display_status_offline_count;
        status_event_count = g_team_rt.display_status_event_count;
        g_team_rt.display_status_dirty = 0U;
    }
    do_event = g_team_rt.display_event_dirty;
    if (do_event != 0U) {
        event_type = g_team_rt.display_event_type;
        event_member_id = g_team_rt.display_event_member_id;
        (void)snprintf(event_label, sizeof(event_label), "%s", g_team_rt.display_event_label);
        event_lat = g_team_rt.display_event_latitude_e6;
        event_lon = g_team_rt.display_event_longitude_e6;
        event_last_seen = g_team_rt.display_event_last_seen_s;
        g_team_rt.display_event_dirty = 0U;
    }
    osal_irq_restore(irq_sts);

    if (do_event != 0U) {
        display_ret = ws63_st7789_show_event((uint8_t)event_type, event_label, event_lat, event_lon,
            event_last_seen);
        osal_printk("[display-event] event=%s label=%s member=%u ret=%d last_seen=%lu\r\n",
            team_display_event_name((team_display_event_t)event_type),
            event_label[0] != '\0' ? event_label : "--",
            event_member_id,
            display_ret,
            (unsigned long)event_last_seen);
    }
    if (do_status != 0U) {
        now_ms = uapi_tcxo_get_ms();
        if (g_team_rt.display_status_last_flush_ms != 0U &&
            (now_ms - g_team_rt.display_status_last_flush_ms) < SLE_TEAM_DISPLAY_STATUS_MIN_INTERVAL_MS) {
            irq_sts = osal_irq_lock();
            g_team_rt.display_status_dirty = 1U;
            osal_irq_restore(irq_sts);
            return;
        }
        (void)ws63_st7789_show_status(status_role[0] != '\0' ? status_role : "idle",
            status_self[0] != '\0' ? status_self : "--", status_online, status_offline,
            status_event_count, SLE_TEAM_FW_VERSION);
        g_team_rt.display_status_last_flush_ms = now_ms;
    }
#endif
}

#if CONFIG_SLE_TEAM_ST7789_ENABLE
static void team_display_init_log(uint8_t ready, const char *phase)
{
    osal_printk("[hw] display present=1 ready=%u task=%u sclk=%u sda=%u cs=%u cs_low=%u rs=%u reset=%u size=%ux%u off=%u,%u phase=%s\r\n",
        ready, g_team_rt.display_task_started,
        (uint8_t)CONFIG_SLE_TEAM_ST7789_SCLK_PIN,
        (uint8_t)CONFIG_SLE_TEAM_ST7789_MOSI_PIN,
        (uint8_t)CONFIG_SLE_TEAM_ST7789_CS_PIN,
        (uint8_t)CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW,
        (uint8_t)CONFIG_SLE_TEAM_ST7789_DC_PIN,
        (uint8_t)CONFIG_SLE_TEAM_ST7789_RESET_PIN,
        (uint16_t)CONFIG_SLE_TEAM_ST7789_WIDTH,
        (uint16_t)CONFIG_SLE_TEAM_ST7789_HEIGHT,
        (uint16_t)CONFIG_SLE_TEAM_ST7789_X_OFFSET,
        (uint16_t)CONFIG_SLE_TEAM_ST7789_Y_OFFSET,
        phase);
    osal_printk("[hw] display fpc-note cs is firmware-held-low; panel gnd still needs real board ground\r\n");
}
#endif

static void team_display_init(void)
{
#if CONFIG_SLE_TEAM_ST7789_ENABLE
    ws63_st7789_config_t cfg;

    (void)memset_s(&cfg, sizeof(cfg), 0, sizeof(cfg));
    cfg.spi_bus = (uint8_t)CONFIG_SLE_TEAM_ST7789_SPI_BUS;
    cfg.sclk_pin = (uint8_t)CONFIG_SLE_TEAM_ST7789_SCLK_PIN;
    cfg.mosi_pin = (uint8_t)CONFIG_SLE_TEAM_ST7789_MOSI_PIN;
    cfg.cs_pin = (uint8_t)CONFIG_SLE_TEAM_ST7789_CS_PIN;
    cfg.dc_pin = (uint8_t)CONFIG_SLE_TEAM_ST7789_DC_PIN;
    cfg.reset_pin = (uint8_t)CONFIG_SLE_TEAM_ST7789_RESET_PIN;
    cfg.x_offset = (uint16_t)CONFIG_SLE_TEAM_ST7789_X_OFFSET;
    cfg.y_offset = (uint16_t)CONFIG_SLE_TEAM_ST7789_Y_OFFSET;
    cfg.width = (uint16_t)CONFIG_SLE_TEAM_ST7789_WIDTH;
    cfg.height = (uint16_t)CONFIG_SLE_TEAM_ST7789_HEIGHT;
    if (ws63_st7789_init(&cfg) == 0) {
        g_team_rt.display_ready = 1U;
        team_display_init_log(1U, "ready");
    } else {
        g_team_rt.display_ready = 0U;
        team_display_init_log(0U, "failed");
        osal_printk("[display] disabled after init failure\r\n");
    }
#endif
}

static void *team_display_task(const char *arg)
{
    unused(arg);
    team_display_init();

    while (1) {
        ws63_st7789_tick();
        team_display_flush_pending_once();
        osal_msleep(SLE_TEAM_DISPLAY_TASK_INTERVAL_MS);
    }

    return NULL;
}

static void team_display_start(void)
{
#if CONFIG_SLE_TEAM_ST7789_ENABLE
    osal_task *task = NULL;

    if (g_team_rt.display_task_started != 0U) {
        return;
    }
    osal_kthread_lock();
    task = osal_kthread_create((osal_kthread_handler)team_display_task, NULL, "TeamDisplayTask",
        SLE_TEAM_DISPLAY_TASK_STACK_SIZE);
    if (task != NULL) {
        osal_kthread_set_priority(task, SLE_TEAM_DISPLAY_TASK_PRIO);
        g_team_rt.display_task_started = 1U;
    }
    osal_kthread_unlock();
    if (task == NULL) {
        osal_printk("[display] task create failed\r\n");
    }
#endif
}

static void team_display_wait_ready(uint32_t timeout_ms)
{
#if CONFIG_SLE_TEAM_ST7789_ENABLE
    uint32_t waited_ms = 0U;

    while (g_team_rt.display_task_started != 0U && g_team_rt.display_ready == 0U && waited_ms < timeout_ms) {
        osal_msleep(20);
        waited_ms += 20U;
    }
#else
    unused(timeout_ms);
#endif
}

static void team_hardware_report_print(void)
{
    osal_printk("[hw] init summary fw=%s schematic=SCH_Schematic1_2_v3.2 constraints=%s\r\n",
        SLE_TEAM_FW_VERSION, SLE_TEAM_HW_CONSTRAINTS);
    osal_printk("[hw] ws2812 present=%u ready=%u pin=%u state=%s color=%u,%u,%u\r\n",
        (uint8_t)CONFIG_SLE_TEAM_WS2812_ENABLE, g_team_rt.ws2812_ready, g_team_rt.ws2812_pin,
        team_rgb_state_name((team_rgb_state_t)g_team_rt.ws2812_state), g_team_rt.ws2812_r,
        g_team_rt.ws2812_g, g_team_rt.ws2812_b);
    osal_printk("[hw] buzzer present=%u ready=%u pin=%u active_high=%u muted=%u level_on=%u\r\n",
        (uint8_t)CONFIG_SLE_TEAM_BUZZER_ENABLE, g_team_rt.buzzer_ready, g_team_rt.buzzer_pin,
        g_team_rt.buzzer_active_high, (uint8_t)CONFIG_SLE_TEAM_BUZZER_MUTED, g_team_rt.buzzer_level_on);
    osal_printk("[hw] gps configured=%u present=0 ready=%u uart=%u tx=%u rx=%u parser=0 module=none\r\n",
        (uint8_t)CONFIG_SLE_TEAM_GPS_ENABLE, g_team_rt.gps_ready, g_team_rt.gps_uart_bus,
        g_team_rt.gps_txd_pin, g_team_rt.gps_rxd_pin);
    osal_printk("[hw] adc present=%u ready=%u ctrl=%u vbat=%u channel=%u ctrl_active_high=%u "
        "valid=%u adc_mv=%u vbat_mv=%u battery=%u ret=%ld\r\n",
        (uint8_t)CONFIG_SLE_TEAM_ADC_ENABLE, g_team_rt.adc_ready, g_team_rt.adc_ctrl_pin,
        g_team_rt.adc_vbat_pin, g_team_rt.adc_vbat_channel, g_team_rt.adc_ctrl_active_high,
        g_team_rt.battery_valid, g_team_rt.adc_sample_mv, g_team_rt.battery_mv,
        g_team_rt.battery_percent, (long)g_team_rt.battery_sample_last_ret);
    team_chrg_sample();
    osal_printk("[hw] chrg present=%u ready=%u pin=%u active_low=%u external_pullup=%u raw=%u "
        "charging=%u source=%s source_certain=%u\r\n",
        (uint8_t)CONFIG_SLE_TEAM_CHRG_ENABLE, g_team_rt.chrg_ready, g_team_rt.chrg_pin,
        g_team_rt.chrg_active_low, (uint8_t)CONFIG_SLE_TEAM_CHRG_EXTERNAL_PULLUP,
        g_team_rt.chrg_raw, g_team_rt.charging, team_power_source_name(), team_power_source_certain());
    osal_printk("[hw] display present=%u ready=%u task=%u sclk=%u sda=%u cs=%u cs_low=%u rs=%u reset=%u size=%ux%u off=%u,%u\r\n",
        (uint8_t)CONFIG_SLE_TEAM_ST7789_ENABLE, g_team_rt.display_ready, g_team_rt.display_task_started,
        (uint8_t)CONFIG_SLE_TEAM_ST7789_SCLK_PIN, (uint8_t)CONFIG_SLE_TEAM_ST7789_MOSI_PIN,
        (uint8_t)CONFIG_SLE_TEAM_ST7789_CS_PIN, (uint8_t)CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW,
        (uint8_t)CONFIG_SLE_TEAM_ST7789_DC_PIN,
        (uint8_t)CONFIG_SLE_TEAM_ST7789_RESET_PIN, (uint16_t)CONFIG_SLE_TEAM_ST7789_WIDTH,
        (uint16_t)CONFIG_SLE_TEAM_ST7789_HEIGHT, (uint16_t)CONFIG_SLE_TEAM_ST7789_X_OFFSET,
        (uint16_t)CONFIG_SLE_TEAM_ST7789_Y_OFFSET);
}

static void team_ws2812_force_off(void)
{
    uint8_t i;

    if (g_team_rt.ws2812_ready == 0U) {
        return;
    }
    for (i = 0U; i < SLE_TEAM_WS2812_FORCE_CLEAR_COUNT; i++) {
        (void)ws63_ws2812_clear();
        osal_msleep(SLE_TEAM_WS2812_FORCE_CLEAR_DELAY_MS);
    }
    g_team_rt.ws2812_r = 0U;
    g_team_rt.ws2812_g = 0U;
    g_team_rt.ws2812_b = 0U;
    g_team_rt.ws2812_state = (uint8_t)TEAM_RGB_STATE_OFF;
    g_team_rt.ws2812_state_last_ms = uapi_tcxo_get_ms();
    g_team_rt.ws2812_base_state = (uint8_t)TEAM_RGB_STATE_OFF;
    g_team_rt.ws2812_base_enter_ms = 0U;
    g_team_rt.ws2812_flash_state = (uint8_t)TEAM_RGB_STATE_OFF;
    g_team_rt.ws2812_flash_pulses_left = 0U;
    g_team_rt.ws2812_flash_on = 0U;
    g_team_rt.ws2812_flash_step_ms = 0U;
    (void)uapi_gpio_set_val(g_team_rt.ws2812_pin, GPIO_LEVEL_LOW);
    osal_printk("[diag] ws2812 force-off pin=%u clear_count=%u\r\n",
        g_team_rt.ws2812_pin, SLE_TEAM_WS2812_FORCE_CLEAR_COUNT);
}

static int team_ws2812_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
#if CONFIG_SLE_TEAM_WS2812_ENABLE
    if (g_team_rt.ws2812_ready == 0U) {
        return -1;
    }
    if (ws63_ws2812_set_rgb(red, green, blue) != 0) {
        return -1;
    }
    g_team_rt.ws2812_r = red;
    g_team_rt.ws2812_g = green;
    g_team_rt.ws2812_b = blue;
    return 0;
#else
    unused(red);
    unused(green);
    unused(blue);
    team_ws2812_force_off();
    return -1;
#endif
}

static const char *team_rgb_state_name(team_rgb_state_t state)
{
    switch (state) {
        case TEAM_RGB_STATE_BOOT:
            return "boot";
        case TEAM_RGB_STATE_IDLE:
            return "idle";
        case TEAM_RGB_STATE_LEADER:
            return "leader";
        case TEAM_RGB_STATE_MEMBER:
            return "member";
        case TEAM_RGB_STATE_SEEK:
            return "seek";
        case TEAM_RGB_STATE_TX:
            return "tx";
        case TEAM_RGB_STATE_RX:
            return "rx";
        case TEAM_RGB_STATE_WARN:
            return "warn";
        case TEAM_RGB_STATE_ERROR:
            return "error";
        case TEAM_RGB_STATE_OFF:
        default:
            return "off";
    }
}

static void team_rgb_state_color(team_rgb_state_t state, uint8_t *red, uint8_t *green, uint8_t *blue)
{
    if (red == NULL || green == NULL || blue == NULL) {
        return;
    }
    switch (state) {
        case TEAM_RGB_STATE_BOOT:
            *red = SLE_TEAM_WS2812_BOOT_R;
            *green = SLE_TEAM_WS2812_BOOT_G;
            *blue = SLE_TEAM_WS2812_BOOT_B;
            return;
        case TEAM_RGB_STATE_IDLE:
            *red = SLE_TEAM_WS2812_IDLE_R;
            *green = SLE_TEAM_WS2812_IDLE_G;
            *blue = SLE_TEAM_WS2812_IDLE_B;
            return;
        case TEAM_RGB_STATE_LEADER:
            *red = SLE_TEAM_WS2812_LEADER_R;
            *green = SLE_TEAM_WS2812_LEADER_G;
            *blue = SLE_TEAM_WS2812_LEADER_B;
            return;
        case TEAM_RGB_STATE_MEMBER:
            *red = SLE_TEAM_WS2812_MEMBER_R;
            *green = SLE_TEAM_WS2812_MEMBER_G;
            *blue = SLE_TEAM_WS2812_MEMBER_B;
            return;
        case TEAM_RGB_STATE_SEEK:
            *red = SLE_TEAM_WS2812_SEEK_R;
            *green = SLE_TEAM_WS2812_SEEK_G;
            *blue = SLE_TEAM_WS2812_SEEK_B;
            return;
        case TEAM_RGB_STATE_TX:
            *red = SLE_TEAM_WS2812_TX_R;
            *green = SLE_TEAM_WS2812_TX_G;
            *blue = SLE_TEAM_WS2812_TX_B;
            return;
        case TEAM_RGB_STATE_RX:
            *red = SLE_TEAM_WS2812_RX_R;
            *green = SLE_TEAM_WS2812_RX_G;
            *blue = SLE_TEAM_WS2812_RX_B;
            return;
        case TEAM_RGB_STATE_WARN:
            *red = SLE_TEAM_WS2812_WARN_R;
            *green = SLE_TEAM_WS2812_WARN_G;
            *blue = SLE_TEAM_WS2812_WARN_B;
            return;
        case TEAM_RGB_STATE_ERROR:
            *red = SLE_TEAM_WS2812_ERROR_R;
            *green = SLE_TEAM_WS2812_ERROR_G;
            *blue = SLE_TEAM_WS2812_ERROR_B;
            return;
        case TEAM_RGB_STATE_OFF:
        default:
            *red = 0U;
            *green = 0U;
            *blue = 0U;
            return;
    }
}

static uint8_t team_rgb_state_is_blinking(team_rgb_state_t state)
{
    return state == TEAM_RGB_STATE_IDLE || state == TEAM_RGB_STATE_LEADER ||
        state == TEAM_RGB_STATE_MEMBER || state == TEAM_RGB_STATE_ERROR ? 1U : 0U;
}

static uint32_t team_rgb_state_blink_on_ms(team_rgb_state_t state)
{
    switch (state) {
        case TEAM_RGB_STATE_IDLE:
            return SLE_TEAM_WS2812_IDLE_BLINK_ON_MS;
        case TEAM_RGB_STATE_LEADER:
            return SLE_TEAM_WS2812_LEADER_BLINK_ON_MS;
        case TEAM_RGB_STATE_MEMBER:
            return SLE_TEAM_WS2812_MEMBER_BLINK_ON_MS;
        case TEAM_RGB_STATE_ERROR:
            return SLE_TEAM_WS2812_ERROR_BLINK_ON_MS;
        case TEAM_RGB_STATE_BOOT:
        case TEAM_RGB_STATE_SEEK:
        case TEAM_RGB_STATE_TX:
        case TEAM_RGB_STATE_RX:
        case TEAM_RGB_STATE_WARN:
        case TEAM_RGB_STATE_OFF:
        default:
            return 0U;
    }
}

static uint32_t team_rgb_state_blink_period_ms(team_rgb_state_t state)
{
    switch (state) {
        case TEAM_RGB_STATE_IDLE:
            return SLE_TEAM_WS2812_IDLE_BLINK_PERIOD_MS;
        case TEAM_RGB_STATE_LEADER:
            return SLE_TEAM_WS2812_LEADER_BLINK_PERIOD_MS;
        case TEAM_RGB_STATE_MEMBER:
            return SLE_TEAM_WS2812_MEMBER_BLINK_PERIOD_MS;
        case TEAM_RGB_STATE_ERROR:
            return SLE_TEAM_WS2812_ERROR_BLINK_PERIOD_MS;
        case TEAM_RGB_STATE_BOOT:
        case TEAM_RGB_STATE_SEEK:
        case TEAM_RGB_STATE_TX:
        case TEAM_RGB_STATE_RX:
        case TEAM_RGB_STATE_WARN:
        case TEAM_RGB_STATE_OFF:
        default:
            return 0U;
    }
}

static uint8_t team_rgb_state_blink_is_on(team_rgb_state_t state, uint32_t elapsed_ms)
{
    uint32_t period_ms = team_rgb_state_blink_period_ms(state);
    uint32_t on_ms = team_rgb_state_blink_on_ms(state);

    if (period_ms == 0U) {
        return 1U;
    }
    if (on_ms >= period_ms) {
        return 1U;
    }
    return (elapsed_ms % period_ms) < on_ms ? 1U : 0U;
}

static void team_ws2812_apply_color(team_rgb_state_t state, uint8_t red, uint8_t green, uint8_t blue,
    uint8_t log_state)
{
    if (g_team_rt.ws2812_ready == 0U) {
        return;
    }
    if (g_team_rt.ws2812_state == (uint8_t)state &&
        g_team_rt.ws2812_r == red && g_team_rt.ws2812_g == green && g_team_rt.ws2812_b == blue) {
        return;
    }
    if (team_ws2812_set_rgb(red, green, blue) == 0) {
        g_team_rt.ws2812_state = (uint8_t)state;
        g_team_rt.ws2812_state_last_ms = uapi_tcxo_get_ms();
        if (log_state != 0U) {
            osal_printk("[state] rgb state=%s pin=%u color=%u,%u,%u\r\n",
                team_rgb_state_name(state), g_team_rt.ws2812_pin, red, green, blue);
        }
    }
}

static void team_ws2812_set_state(team_rgb_state_t state)
{
    uint8_t red = 0U;
    uint8_t green = 0U;
    uint8_t blue = 0U;

    team_rgb_state_color(state, &red, &green, &blue);
    team_ws2812_apply_color(state, red, green, blue, 1U);
}

static team_rgb_state_t team_ws2812_base_state(void)
{
    if (g_team_rt.role_configured == 0U) {
        return TEAM_RGB_STATE_IDLE;
    }
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        return TEAM_RGB_STATE_LEADER;
    }
    if (g_team_node.joined != 0U) {
        return TEAM_RGB_STATE_MEMBER;
    }
    return TEAM_RGB_STATE_ERROR;
}

static void team_ws2812_render_base_state(uint32_t now_ms)
{
    team_rgb_state_t state = team_ws2812_base_state();
    uint8_t red = 0U;
    uint8_t green = 0U;
    uint8_t blue = 0U;
    uint8_t log_state;

    if (g_team_rt.ws2812_base_state != (uint8_t)state) {
        g_team_rt.ws2812_base_state = (uint8_t)state;
        g_team_rt.ws2812_base_enter_ms = now_ms;
    }
    if (team_rgb_state_is_blinking(state) == 0U ||
        team_rgb_state_blink_is_on(state, now_ms - g_team_rt.ws2812_base_enter_ms) != 0U) {
        team_rgb_state_color(state, &red, &green, &blue);
    }
    log_state = g_team_rt.ws2812_state == (uint8_t)state ? 0U : 1U;
    team_ws2812_apply_color(state, red, green, blue, log_state);
}

static void team_ws2812_restart_base_phase(uint32_t now_ms)
{
    g_team_rt.ws2812_base_state = (uint8_t)TEAM_RGB_STATE_OFF;
    g_team_rt.ws2812_base_enter_ms = now_ms;
}

static void team_ws2812_start_flash(team_rgb_state_t state)
{
    if (g_team_rt.ws2812_ready == 0U) {
        return;
    }
    g_team_rt.ws2812_flash_state = (uint8_t)state;
    g_team_rt.ws2812_flash_pulses_left = SLE_TEAM_WS2812_FLASH_PULSES;
    g_team_rt.ws2812_flash_on = 1U;
    g_team_rt.ws2812_flash_step_ms = uapi_tcxo_get_ms();
    team_ws2812_set_state(state);
}

static uint8_t team_ws2812_refresh_flash(uint32_t now_ms)
{
    uint32_t interval_ms;

    if (g_team_rt.ws2812_flash_state == (uint8_t)TEAM_RGB_STATE_OFF ||
        g_team_rt.ws2812_flash_pulses_left == 0U) {
        return 0U;
    }
    interval_ms = g_team_rt.ws2812_flash_on != 0U ?
        SLE_TEAM_WS2812_FLASH_ON_MS : SLE_TEAM_WS2812_FLASH_OFF_MS;
    if ((now_ms - g_team_rt.ws2812_flash_step_ms) < interval_ms) {
        return 1U;
    }

    g_team_rt.ws2812_flash_step_ms = now_ms;
    if (g_team_rt.ws2812_flash_on != 0U) {
        g_team_rt.ws2812_flash_pulses_left--;
        if (g_team_rt.ws2812_flash_pulses_left == 0U) {
            g_team_rt.ws2812_flash_state = (uint8_t)TEAM_RGB_STATE_OFF;
            g_team_rt.ws2812_flash_on = 0U;
            team_ws2812_restart_base_phase(now_ms);
            team_ws2812_render_base_state(now_ms);
            return 0U;
        }
        g_team_rt.ws2812_flash_on = 0U;
        team_ws2812_apply_color(TEAM_RGB_STATE_OFF, 0U, 0U, 0U, 0U);
        return 1U;
    }

    g_team_rt.ws2812_flash_on = 1U;
    team_ws2812_set_state((team_rgb_state_t)g_team_rt.ws2812_flash_state);
    return 1U;
}

static void team_ws2812_refresh_network_state(void)
{
    uint32_t now_ms;

    if (g_team_rt.ws2812_ready == 0U) {
        return;
    }
    now_ms = uapi_tcxo_get_ms();
    if (team_ws2812_refresh_flash(now_ms) != 0U) {
        return;
    }
    team_ws2812_render_base_state(now_ms);
}

static void team_ws2812_show_display_event(team_display_event_t event)
{
    switch (event) {
        case TEAM_DISPLAY_EVENT_TIMEOUT:
        case TEAM_DISPLAY_EVENT_LOST:
            if (g_team_rt.role_configured != 0U && g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
                team_ws2812_start_flash(TEAM_RGB_STATE_ERROR);
            }
            return;
        case TEAM_DISPLAY_EVENT_LEFT:
            if (g_team_rt.role_configured != 0U && g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
                team_ws2812_start_flash(TEAM_RGB_STATE_WARN);
            }
            return;
        case TEAM_DISPLAY_EVENT_JOIN:
        case TEAM_DISPLAY_EVENT_REJOIN:
            if (g_team_rt.role_configured != 0U && g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
                team_ws2812_start_flash(TEAM_RGB_STATE_LEADER);
            } else {
                team_ws2812_refresh_network_state();
            }
            return;
        case TEAM_DISPLAY_EVENT_NONE:
        default:
            return;
    }
}

static void team_ws2812_show_boot_marker(void)
{
    team_ws2812_set_state(TEAM_RGB_STATE_BOOT);
    osal_printk("[diag] ws2812 startup marker pin=%u color=%u,%u,%u timing=cycle-counter\r\n",
        g_team_rt.ws2812_pin,
        (uint8_t)SLE_TEAM_WS2812_BOOT_R,
        (uint8_t)SLE_TEAM_WS2812_BOOT_G,
        (uint8_t)SLE_TEAM_WS2812_BOOT_B);
}

static void team_ws2812_test_pattern(void)
{
#if CONFIG_SLE_TEAM_WS2812_ENABLE
    if (g_team_rt.ws2812_ready == 0U) {
        osal_printk("[diag] ws2812 not ready; test pattern ignored\r\n");
        return;
    }
    osal_printk("[diag] ws2812 boot rgb-test pin=%u level=%u step_ms=%u\r\n",
        g_team_rt.ws2812_pin,
        (uint8_t)SLE_TEAM_WS2812_TEST_R,
        (uint16_t)SLE_TEAM_WS2812_TEST_STEP_MS);
    (void)team_ws2812_set_rgb(SLE_TEAM_WS2812_TEST_R, 0U, 0U);
    osal_msleep(SLE_TEAM_WS2812_TEST_STEP_MS);
    (void)team_ws2812_set_rgb(0U, SLE_TEAM_WS2812_TEST_G, 0U);
    osal_msleep(SLE_TEAM_WS2812_TEST_STEP_MS);
    (void)team_ws2812_set_rgb(0U, 0U, SLE_TEAM_WS2812_TEST_B);
    osal_msleep(SLE_TEAM_WS2812_TEST_STEP_MS);
    team_ws2812_show_boot_marker();
#else
    team_ws2812_force_off();
    osal_printk("[diag] ws2812 disabled; test pattern ignored\r\n");
#endif
}

static void team_ws2812_init(void)
{
    g_team_rt.ws2812_pin = (uint8_t)CONFIG_SLE_TEAM_WS2812_PIN;
    if (ws63_ws2812_init(g_team_rt.ws2812_pin) == 0) {
        g_team_rt.ws2812_ready = 1U;
        g_team_rt.ws2812_r = 0U;
        g_team_rt.ws2812_g = 0U;
        g_team_rt.ws2812_b = 0U;
        g_team_rt.ws2812_state = (uint8_t)TEAM_RGB_STATE_OFF;
        g_team_rt.ws2812_state_last_ms = 0U;
        g_team_rt.ws2812_base_state = (uint8_t)TEAM_RGB_STATE_OFF;
        g_team_rt.ws2812_base_enter_ms = 0U;
        g_team_rt.ws2812_flash_state = (uint8_t)TEAM_RGB_STATE_OFF;
        g_team_rt.ws2812_flash_pulses_left = 0U;
        g_team_rt.ws2812_flash_on = 0U;
        g_team_rt.ws2812_flash_step_ms = 0U;
#if CONFIG_SLE_TEAM_WS2812_ENABLE
        osal_printk("[diag] ws2812 init ready pin=%u; boot rgb-test follows\r\n",
            g_team_rt.ws2812_pin);
        team_ws2812_test_pattern();
#else
        team_ws2812_force_off();
        osal_printk("[diag] ws2812 disabled by %s (fw=%s), pin=%u cleared and held idle\r\n",
            SLE_TEAM_HW_CONSTRAINTS, SLE_TEAM_FW_VERSION, g_team_rt.ws2812_pin);
#endif
    } else {
        g_team_rt.ws2812_ready = 0U;
        osal_printk("[diag] ws2812 disabled by %s (fw=%s), pin=%u clear failed\r\n",
            SLE_TEAM_HW_CONSTRAINTS, SLE_TEAM_FW_VERSION, g_team_rt.ws2812_pin);
    }
}

static void team_gps_init(void)
{
    g_team_rt.gps_uart_bus = (uint8_t)CONFIG_SLE_TEAM_GPS_UART_BUS;
    g_team_rt.gps_txd_pin = (uint8_t)CONFIG_SLE_TEAM_GPS_UART_TXD_PIN;
    g_team_rt.gps_rxd_pin = (uint8_t)CONFIG_SLE_TEAM_GPS_UART_RXD_PIN;
    g_team_rt.gps_ready = 0U;
#if CONFIG_SLE_TEAM_GPS_ENABLE
    if (g_team_rt.gps_txd_pin <= 31U && g_team_rt.gps_rxd_pin <= 31U) {
        (void)uapi_pin_set_mode(g_team_rt.gps_txd_pin, PIN_MODE_1);
        (void)uapi_pin_set_mode(g_team_rt.gps_rxd_pin, PIN_MODE_1);
        g_team_rt.gps_ready = 1U;
    }
#else
#endif
    /*
     * GPS_ENABLE only means the UART pinmap is configured. These product boards
     * are not populated with a GPS module, and no NMEA parser is enabled here.
     */
    osal_printk("[hw] gps configured=%u present=0 ready=%u uart=%u tx=%u rx=%u parser=0 module=none\r\n",
        (uint8_t)CONFIG_SLE_TEAM_GPS_ENABLE, g_team_rt.gps_ready, g_team_rt.gps_uart_bus,
        g_team_rt.gps_txd_pin, g_team_rt.gps_rxd_pin);
}

static void team_gpio_config_output_level(uint8_t pin, uint8_t level)
{
    (void)uapi_pin_set_mode(pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_pull(pin, PIN_PULL_TYPE_DOWN);
    (void)uapi_gpio_set_val(pin, level);
    (void)uapi_gpio_set_dir(pin, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_val(pin, level);
}

static uint8_t team_adc_ctrl_off_level(void) SLE_TEAM_UNUSED_FUNC;
static uint8_t team_adc_ctrl_on_level(void) SLE_TEAM_UNUSED_FUNC;
static void team_adc_ctrl_set(uint8_t enabled) SLE_TEAM_UNUSED_FUNC;

static void team_chrg_sample(void)
{
#if CONFIG_SLE_TEAM_CHRG_ENABLE
    if (g_team_rt.chrg_ready == 0U || g_team_rt.chrg_pin > 31U) {
        return;
    }
    g_team_rt.chrg_raw = (uint8_t)uapi_gpio_get_val(g_team_rt.chrg_pin);
    if (g_team_rt.chrg_active_low != 0U) {
        g_team_rt.charging = g_team_rt.chrg_raw == (uint8_t)GPIO_LEVEL_LOW ? 1U : 0U;
    } else {
        g_team_rt.charging = g_team_rt.chrg_raw == (uint8_t)GPIO_LEVEL_HIGH ? 1U : 0U;
    }
#endif
}

static const char *team_power_source_name(void)
{
    if (g_team_rt.chrg_ready == 0U) {
        return "unknown";
    }
    if (g_team_rt.charging != 0U) {
        return "pwr-charging";
    }
    return "battery-or-full";
}

static uint8_t team_power_source_certain(void)
{
    return (uint8_t)(g_team_rt.chrg_ready != 0U && g_team_rt.charging != 0U);
}

static void team_chrg_init(void)
{
    g_team_rt.chrg_pin = (uint8_t)CONFIG_SLE_TEAM_CHRG_PIN;
    g_team_rt.chrg_active_low = (uint8_t)CONFIG_SLE_TEAM_CHRG_ACTIVE_LOW;
    g_team_rt.chrg_ready = 0U;
    g_team_rt.chrg_raw = GPIO_LEVEL_HIGH;
    g_team_rt.charging = 0U;
#if CONFIG_SLE_TEAM_CHRG_ENABLE
    if (g_team_rt.chrg_pin <= 31U) {
        (void)uapi_pin_set_mode(g_team_rt.chrg_pin, HAL_PIO_FUNC_GPIO);
        (void)uapi_gpio_set_dir(g_team_rt.chrg_pin, GPIO_DIRECTION_INPUT);
        g_team_rt.chrg_ready = 1U;
        team_chrg_sample();
    }
#endif
    osal_printk("[hw] chrg present=%u ready=%u pin=%u active_low=%u external_pullup=%u raw=%u "
        "charging=%u source=%s source_certain=%u\r\n",
        (uint8_t)CONFIG_SLE_TEAM_CHRG_ENABLE, g_team_rt.chrg_ready, g_team_rt.chrg_pin,
        g_team_rt.chrg_active_low, (uint8_t)CONFIG_SLE_TEAM_CHRG_EXTERNAL_PULLUP,
        g_team_rt.chrg_raw, g_team_rt.charging, team_power_source_name(), team_power_source_certain());
}

static uint8_t team_adc_ctrl_off_level(void)
{
    return CONFIG_SLE_TEAM_ADC_CTRL_ACTIVE_HIGH ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
}

static uint8_t team_adc_ctrl_on_level(void)
{
    return CONFIG_SLE_TEAM_ADC_CTRL_ACTIVE_HIGH ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
}

static void team_adc_ctrl_set(uint8_t enabled)
{
    if (g_team_rt.adc_ctrl_pin > 31U) {
        return;
    }
    (void)uapi_gpio_set_val(g_team_rt.adc_ctrl_pin,
        enabled != 0U ? team_adc_ctrl_on_level() : team_adc_ctrl_off_level());
}

static uint16_t team_battery_vbat_mv_from_adc_mv(uint16_t adc_mv)
{
    uint32_t vbat_mv = ((uint32_t)adc_mv * (SLE_TEAM_ADC_DIVIDER_TOP_KOHM +
        SLE_TEAM_ADC_DIVIDER_BOTTOM_KOHM) + (SLE_TEAM_ADC_DIVIDER_BOTTOM_KOHM / 2U)) /
        SLE_TEAM_ADC_DIVIDER_BOTTOM_KOHM;

    return vbat_mv > 65535U ? 65535U : (uint16_t)vbat_mv;
}

static uint8_t team_battery_percent_from_vbat_mv(uint16_t vbat_mv)
{
    uint32_t range_mv = SLE_TEAM_BATTERY_FULL_MV - SLE_TEAM_BATTERY_EMPTY_MV;
    uint32_t above_empty_mv;

    if (vbat_mv <= SLE_TEAM_BATTERY_EMPTY_MV) {
        return 0U;
    }
    if (vbat_mv >= SLE_TEAM_BATTERY_FULL_MV) {
        return 100U;
    }
    above_empty_mv = (uint32_t)vbat_mv - SLE_TEAM_BATTERY_EMPTY_MV;
    return (uint8_t)((above_empty_mv * 100U + (range_mv / 2U)) / range_mv);
}

static int team_battery_sample_once(uint8_t log_result)
{
    int ret = SLE_TEAM_ERR_UNSUPPORTED;

    team_chrg_sample();
#if CONFIG_SLE_TEAM_ADC_ENABLE
    uint16_t adc_mv = 0U;

    if (g_team_rt.adc_ready == 0U) {
        g_team_rt.battery_sample_last_ret = ret;
        return ret;
    }

    team_adc_ctrl_set(1U);
    osal_msleep((uint32_t)CONFIG_SLE_TEAM_ADC_SAMPLE_SETTLE_MS);
    ret = (int)adc_port_read(g_team_rt.adc_vbat_channel, &adc_mv);
    uapi_adc_power_en(AFE_GADC_MODE, false);
    team_adc_ctrl_set(0U);

    g_team_rt.battery_sample_last_ms = uapi_tcxo_get_ms();
    g_team_rt.battery_sample_last_ret = ret;
    if (ret == (int)ERRCODE_SUCC) {
        g_team_rt.adc_sample_mv = adc_mv;
        g_team_rt.battery_mv = team_battery_vbat_mv_from_adc_mv(adc_mv);
        g_team_rt.battery_percent = team_battery_percent_from_vbat_mv(g_team_rt.battery_mv);
        g_team_rt.battery_valid = 1U;
    }
#else
    unused(log_result);
#endif
    if (log_result != 0U) {
        osal_printk("[battery] sample valid=%u adc_mv=%u vbat_mv=%u percent=%u ctrl=%u vbat=%u "
            "channel=%u ratio=%u/%u ret=%ld source=%s source_certain=%u charging=%u chrg_raw=%u chrg_pin=%u\r\n",
            g_team_rt.battery_valid, g_team_rt.adc_sample_mv, g_team_rt.battery_mv,
            g_team_rt.battery_percent, g_team_rt.adc_ctrl_pin, g_team_rt.adc_vbat_pin,
            g_team_rt.adc_vbat_channel, SLE_TEAM_ADC_DIVIDER_TOP_KOHM,
            SLE_TEAM_ADC_DIVIDER_BOTTOM_KOHM, (long)g_team_rt.battery_sample_last_ret,
            team_power_source_name(), team_power_source_certain(), g_team_rt.charging,
            g_team_rt.chrg_raw, g_team_rt.chrg_pin);
    }
    return ret;
}

static void team_battery_sample_tick(uint8_t force_now)
{
    uint32_t now_s;

    if (g_team_rt.adc_ready == 0U) {
        return;
    }
    now_s = team_now_s(NULL);
    if (force_now == 0U && g_team_rt.battery_sample_last_ms != 0U &&
        team_interval_not_reached(now_s, g_team_rt.battery_sample_last_ms / 1000U,
            (uint32_t)CONFIG_SLE_TEAM_ADC_SAMPLE_INTERVAL_S) != 0U) {
        return;
    }
    (void)team_battery_sample_once(0U);
}

static uint8_t team_battery_percent_cb(void *user_ctx)
{
    unused(user_ctx);
    return g_team_rt.battery_valid != 0U ? g_team_rt.battery_percent : 100U;
}

static void team_adc_init(void)
{
    g_team_rt.adc_ctrl_pin = (uint8_t)CONFIG_SLE_TEAM_ADC_CTRL_PIN;
    g_team_rt.adc_vbat_pin = (uint8_t)CONFIG_SLE_TEAM_ADC_VBAT_PIN;
    g_team_rt.adc_ctrl_active_high = (uint8_t)CONFIG_SLE_TEAM_ADC_CTRL_ACTIVE_HIGH;
    g_team_rt.adc_vbat_channel = (uint8_t)CONFIG_SLE_TEAM_ADC_VBAT_CHANNEL;
    g_team_rt.adc_ready = 0U;
    g_team_rt.battery_valid = 0U;
    g_team_rt.battery_percent = 100U;
    g_team_rt.adc_sample_mv = 0U;
    g_team_rt.battery_mv = 0U;
    g_team_rt.battery_sample_last_ms = 0U;
    g_team_rt.battery_sample_last_ret = SLE_TEAM_ERR_UNSUPPORTED;
#if CONFIG_SLE_TEAM_ADC_ENABLE
    if (g_team_rt.adc_ctrl_pin <= 31U && g_team_rt.adc_vbat_pin <= 31U) {
        uint8_t off_level;
        errcode_t init_ret;

        off_level = team_adc_ctrl_off_level();
        team_gpio_config_output_level(g_team_rt.adc_ctrl_pin, off_level);
        init_ret = uapi_adc_init(ADC_CLOCK_500KHZ);
        g_team_rt.battery_sample_last_ret = (int32_t)init_ret;
        g_team_rt.adc_ready = init_ret == ERRCODE_SUCC ? 1U : 0U;
        if (g_team_rt.adc_ready != 0U) {
            (void)team_battery_sample_once(0U);
        }
    }
#endif
    osal_printk("[hw] adc present=%u ready=%u ctrl=%u vbat=%u channel=%u ctrl_active_high=%u "
        "ctrl_level=off valid=%u adc_mv=%u vbat_mv=%u battery=%u ret=%ld\r\n",
        (uint8_t)CONFIG_SLE_TEAM_ADC_ENABLE, g_team_rt.adc_ready, g_team_rt.adc_ctrl_pin,
        g_team_rt.adc_vbat_pin, g_team_rt.adc_vbat_channel, g_team_rt.adc_ctrl_active_high,
        g_team_rt.battery_valid, g_team_rt.adc_sample_mv, g_team_rt.battery_mv,
        g_team_rt.battery_percent, (long)g_team_rt.battery_sample_last_ret);
}

static void team_battery_cli_status(void)
{
    team_chrg_sample();
    osal_printk("[cli] bat fw=%s ready=%u valid=%u adc_mv=%u vbat_mv=%u percent=%u ctrl=%u vbat=%u "
        "channel=%u ratio=%u/%u empty_mv=%u full_mv=%u settle_ms=%u interval_s=%u ret=%ld "
        "source=%s source_certain=%u charging=%u chrg_ready=%u chrg_pin=%u chrg_raw=%u chrg_active_low=%u\r\n",
        SLE_TEAM_FW_VERSION, g_team_rt.adc_ready, g_team_rt.battery_valid,
        g_team_rt.adc_sample_mv, g_team_rt.battery_mv, g_team_rt.battery_percent,
        g_team_rt.adc_ctrl_pin, g_team_rt.adc_vbat_pin, g_team_rt.adc_vbat_channel,
        SLE_TEAM_ADC_DIVIDER_TOP_KOHM, SLE_TEAM_ADC_DIVIDER_BOTTOM_KOHM,
        SLE_TEAM_BATTERY_EMPTY_MV, SLE_TEAM_BATTERY_FULL_MV,
        (uint32_t)CONFIG_SLE_TEAM_ADC_SAMPLE_SETTLE_MS,
        (uint32_t)CONFIG_SLE_TEAM_ADC_SAMPLE_INTERVAL_S,
        (long)g_team_rt.battery_sample_last_ret,
        team_power_source_name(), team_power_source_certain(), g_team_rt.charging,
        g_team_rt.chrg_ready, g_team_rt.chrg_pin, g_team_rt.chrg_raw, g_team_rt.chrg_active_low);
}

static uint8_t team_cli_match2(const char *line, const char *first, const char *second)
{
    return (uint8_t)(strcmp(line, first) == 0 || strcmp(line, second) == 0);
}

static int team_battery_cli_handle(const char *line)
{
    if (team_cli_match2(line, "bat", "bat status") != 0U ||
        team_cli_match2(line, "adc", "adc status") != 0U ||
        team_cli_match2(line, "power", "power status") != 0U ||
        team_cli_match2(line, "pwr", "pwr status") != 0U) {
        team_battery_cli_status();
        return 1;
    }
    if (team_cli_match2(line, "bat sample", "adc sample") != 0U ||
        team_cli_match2(line, "power sample", "pwr sample") != 0U) {
        (void)team_battery_sample_once(1U);
        team_battery_cli_status();
        return 1;
    }
    if (team_cli_match2(line, "bat help", "adc help") != 0U ||
        team_cli_match2(line, "power help", "pwr help") != 0U) {
        osal_printk("[cli] bat commands: status|sample; adc aliases: adc status|adc sample; "
            "power aliases: power status|power sample|pwr status|pwr sample\r\n");
        return 1;
    }
    return 0;
}

static void team_buzzer_force_pin_off(uint8_t pin)
{
    uint8_t off_level;

    if (pin > 31U) {
        return;
    }
    off_level = CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
    team_gpio_config_output_level(pin, off_level);
}

static void team_buzzer_apply_level(uint8_t level) SLE_TEAM_UNUSED_FUNC;

static void team_buzzer_apply_level(uint8_t level)
{
#if !CONFIG_SLE_TEAM_BUZZER_ENABLE
    unused(level);
    return;
#else
    uint8_t gpio_level = level;

    if (g_team_rt.buzzer_pin > 31U) {
        return;
    }
#if CONFIG_SLE_TEAM_BUZZER_MUTED
    gpio_level = CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
    g_team_rt.buzzer_level_on = 0U;
#else
    gpio_level = (g_team_rt.buzzer_active_high != 0U) ? level :
        (level != 0U ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH);
#endif
    (void)uapi_pin_set_mode(g_team_rt.buzzer_pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_pull(g_team_rt.buzzer_pin, PIN_PULL_TYPE_DOWN);
    (void)uapi_gpio_set_dir(g_team_rt.buzzer_pin, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_val(g_team_rt.buzzer_pin, gpio_level != 0U ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
#endif
}

static void team_buzzer_set(uint8_t on)
{
#if !CONFIG_SLE_TEAM_BUZZER_ENABLE
    unused(on);
    g_team_rt.buzzer_level_on = 0U;
    return;
#else
#if CONFIG_SLE_TEAM_BUZZER_MUTED
    unused(on);
    g_team_rt.buzzer_level_on = 0U;
    team_buzzer_force_pin_off(g_team_rt.buzzer_pin);
    return;
#else
    g_team_rt.buzzer_level_on = on != 0U ? 1U : 0U;
    team_buzzer_apply_level(g_team_rt.buzzer_level_on != 0U ? SLE_TEAM_BUZZER_FORCE_ON_LEVEL :
        SLE_TEAM_BUZZER_FORCE_OFF_LEVEL);
#endif
#endif
}

static void team_buzzer_toggle_tick(void)
{
#if !CONFIG_SLE_TEAM_BUZZER_ENABLE || CONFIG_SLE_TEAM_BUZZER_MUTED || !CONFIG_SLE_TEAM_BUZZER_AUTO_TOGGLE
    return;
#else
    uint32_t now_ms;

    if (g_team_rt.buzzer_ready == 0U) {
        return;
    }
    now_ms = uapi_tcxo_get_ms();
    if (g_team_rt.buzzer_toggle_last_ms != 0U &&
        (now_ms - g_team_rt.buzzer_toggle_last_ms) < SLE_TEAM_BUZZER_TOGGLE_INTERVAL_MS) {
        return;
    }
    g_team_rt.buzzer_toggle_last_ms = now_ms;
    team_buzzer_set(g_team_rt.buzzer_level_on == 0U ? 1U : 0U);
    osal_printk("[diag] buzzer io14 toggled level=%u interval_ms=%u\r\n",
        g_team_rt.buzzer_level_on, (uint32_t)SLE_TEAM_BUZZER_TOGGLE_INTERVAL_MS);
#endif
}

static void team_buzzer_init(void)
{
    g_team_rt.buzzer_pin = (uint8_t)CONFIG_SLE_TEAM_BUZZER_PIN;
    g_team_rt.buzzer_active_high = (uint8_t)CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH;
    g_team_rt.buzzer_ready = 0U;
    g_team_rt.buzzer_level_on = 0U;
    g_team_rt.buzzer_toggle_last_ms = 0U;
#if !CONFIG_SLE_TEAM_BUZZER_ENABLE
    osal_printk("[diag] buzzer disabled by Kconfig (fw=%s)\r\n", SLE_TEAM_FW_VERSION);
    return;
#endif
    if (g_team_rt.buzzer_pin > 31U) {
        osal_printk("[diag] buzzer disabled by %s (fw=%s) pin=%u\r\n",
            SLE_TEAM_HW_CONSTRAINTS, SLE_TEAM_FW_VERSION, g_team_rt.buzzer_pin);
        return;
    }
    team_buzzer_set(0U);
    g_team_rt.buzzer_ready = 1U;
    g_team_rt.buzzer_toggle_last_ms = uapi_tcxo_get_ms();
    osal_printk("[diag] buzzer init by %s (fw=%s) pin=%u active_high=%u muted=%u auto_toggle=%u level_on=%u\r\n",
        SLE_TEAM_HW_CONSTRAINTS, SLE_TEAM_FW_VERSION, g_team_rt.buzzer_pin,
        g_team_rt.buzzer_active_high, (uint8_t)CONFIG_SLE_TEAM_BUZZER_MUTED,
        (uint8_t)CONFIG_SLE_TEAM_BUZZER_AUTO_TOGGLE, g_team_rt.buzzer_level_on);
}

void OHOS_SystemInit(void)
{
    team_buzzer_force_pin_off((uint8_t)CONFIG_SLE_TEAM_BUZZER_PIN);
}

static void team_led_set(uint8_t on)
{
    if (g_team_rt.led_pin > 31U) {
        return;
    }
    (void)uapi_gpio_set_val(g_team_rt.led_pin,
        ((on != 0U) ^ (g_team_rt.led_active_low != 0U)) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
}

static void team_led_configure(uint8_t pin, uint8_t active_low)
{
    g_team_rt.led_pin = pin;
    g_team_rt.led_active_low = active_low != 0U ? 1U : 0U;
    if (g_team_rt.led_pin > 31U) {
        osal_printk("[state] led disabled pin=%u\r\n", g_team_rt.led_pin);
        return;
    }
    (void)uapi_pin_set_mode(g_team_rt.led_pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_gpio_set_dir(g_team_rt.led_pin, GPIO_DIRECTION_OUTPUT);
    team_led_set(0U);
    osal_printk("[state] led pin=%u active_low=%u level_off=%u\r\n",
        g_team_rt.led_pin,
        g_team_rt.led_active_low,
        g_team_rt.led_active_low != 0U ? (uint8_t)GPIO_LEVEL_HIGH : (uint8_t)GPIO_LEVEL_LOW);
}

static void team_led_post(sle_team_led_event_t event)
{
    uint8_t msg = (uint8_t)event;

    if (g_team_rt.led_queue_ready == 0U) {
        return;
    }
    (void)osal_msg_queue_write_copy(g_team_rt.led_queue_id, &msg, (uint32_t)sizeof(msg), 0);
}

static void team_led_post_seek_throttled(void)
{
    uint32_t now_ms;

    if (g_team_rt.led_queue_ready == 0U || g_team_rt.sle_started == 0U) {
        return;
    }
    now_ms = uapi_tcxo_get_ms();
    if (g_team_rt.last_seek_led_ms != 0U && (now_ms - g_team_rt.last_seek_led_ms) < SLE_TEAM_LED_SEEK_INTERVAL_MS) {
        return;
    }
    g_team_rt.last_seek_led_ms = now_ms;
    team_led_post(SLE_TEAM_LED_EVENT_SEEK);
}

static void team_led_blink(uint8_t pulses, uint32_t on_ms, uint32_t off_ms)
{
    uint8_t i;

    for (i = 0U; i < pulses; i++) {
        team_led_set(1U);
        osal_msleep(on_ms);
        team_led_set(0U);
        if (i + 1U < pulses) {
            osal_msleep(off_ms);
        }
    }
}

static void *team_led_task(const char *arg)
{
    uint8_t event = 0U;
    uint32_t msg_size;

    unused(arg);

    team_led_configure((uint8_t)CONFIG_SLE_TEAM_LED_PIN, CONFIG_SLE_TEAM_LED_ACTIVE_LOW ? 1U : 0U);

    while (1) {
        msg_size = sizeof(event);
        if (osal_msg_queue_read_copy(g_team_rt.led_queue_id, &event, &msg_size, SLE_TEAM_LED_QUEUE_TIMEOUT_MS) !=
            OSAL_SUCCESS) {
            team_led_set(0U);
            continue;
        }
        if (event == (uint8_t)SLE_TEAM_LED_EVENT_TX) {
            team_led_blink(SLE_TEAM_LED_TX_PULSES, SLE_TEAM_LED_TX_ON_MS, SLE_TEAM_LED_TX_OFF_MS);
        } else if (event == (uint8_t)SLE_TEAM_LED_EVENT_RX) {
            team_led_blink(SLE_TEAM_LED_RX_PULSES, SLE_TEAM_LED_RX_ON_MS, SLE_TEAM_LED_RX_OFF_MS);
        } else if (event == (uint8_t)SLE_TEAM_LED_EVENT_SEEK) {
            team_led_blink(SLE_TEAM_LED_SEEK_PULSES, SLE_TEAM_LED_SEEK_ON_MS, SLE_TEAM_LED_SEEK_OFF_MS);
        }
    }
    return NULL;
}

static void team_led_cli_status(void)
{
    osal_printk("[cli] led pin=%u active_low=%u\r\n", g_team_rt.led_pin, g_team_rt.led_active_low);
}

static void team_ws2812_cli_set_rgb(uint8_t red, uint8_t green, uint8_t blue, const char *name)
{
    (void)team_ws2812_set_rgb(red, green, blue);
    osal_printk("[cli] rgb %s ready=%u color=%u,%u,%u\r\n",
        name, g_team_rt.ws2812_ready, g_team_rt.ws2812_r, g_team_rt.ws2812_g, g_team_rt.ws2812_b);
}

static int team_ws2812_cli_handle(const char *line)
{
    unsigned int r;
    unsigned int g;
    unsigned int b;

    if (team_cli_match2(line, "rgb", "rgb status") != 0U) {
        osal_printk("[cli] rgb status %s (fw=%s) ready=%u pin=%u state=%s flash=%s color=%u,%u,%u\r\n",
            SLE_TEAM_HW_CONSTRAINTS, SLE_TEAM_FW_VERSION, g_team_rt.ws2812_ready, g_team_rt.ws2812_pin,
            team_rgb_state_name((team_rgb_state_t)g_team_rt.ws2812_state),
            team_rgb_state_name((team_rgb_state_t)g_team_rt.ws2812_flash_state),
            g_team_rt.ws2812_r, g_team_rt.ws2812_g, g_team_rt.ws2812_b);
        return 1;
    }
    if (strcmp(line, "rgb off") == 0) {
        team_ws2812_force_off();
        return 1;
    }
    if (strcmp(line, "rgb red") == 0 || strcmp(line, "rgb green") == 0 ||
        strcmp(line, "rgb blue") == 0 || strcmp(line, "rgb white") == 0) {
        team_ws2812_cli_set_rgb(line[4] == 'r' ? 16U : (line[4] == 'w' ? 10U : 0U),
            line[4] == 'g' ? 16U : (line[4] == 'w' ? 10U : 0U),
            line[4] == 'b' ? 16U : (line[4] == 'w' ? 10U : 0U), line + 4);
        return 1;
    }
    if (strcmp(line, "rgb test") == 0) {
        team_ws2812_test_pattern();
        return 1;
    }
    if (sscanf(line, "rgb set %u %u %u", &r, &g, &b) == 3 && r <= 255U && g <= 255U && b <= 255U) {
        (void)team_ws2812_set_rgb((uint8_t)r, (uint8_t)g, (uint8_t)b);
        osal_printk("[cli] rgb set ready=%u color=%u,%u,%u\r\n",
            g_team_rt.ws2812_ready, g_team_rt.ws2812_r, g_team_rt.ws2812_g, g_team_rt.ws2812_b);
        return 1;
    }
    if (strcmp(line, "rgb help") == 0) {
        osal_printk("[cli] rgb commands: status|off|red|green|blue|white|test|set <r> <g> <b>\r\n");
        return 1;
    }
    return 0;
}

static int team_buzzer_cli_handle(const char *line)
{
    unsigned int ms;

#if !CONFIG_SLE_TEAM_BUZZER_ENABLE
    if (team_cli_match2(line, "buzz", "buzz status") != 0U ||
        team_cli_match2(line, "buzz on", "buzz off") != 0U ||
        team_cli_match2(line, "buzz beep", "buzz test") != 0U ||
        strcmp(line, "buzz help") == 0 ||
        (sscanf(line, "buzz beep %u", &ms) == 1 && ms > 0U && ms <= 2000U)) {
        osal_printk("[cli] buzz disabled by Kconfig (fw=%s)\r\n", SLE_TEAM_FW_VERSION);
        return 1;
    }
    return 0;
#else
    if (team_cli_match2(line, "buzz", "buzz status") != 0U) {
        osal_printk("[cli] buzz fw=%s ready=%u pin=%u active_high=%u muted=%u auto_toggle=%u level_on=%u\r\n",
            SLE_TEAM_FW_VERSION, g_team_rt.buzzer_ready, g_team_rt.buzzer_pin, g_team_rt.buzzer_active_high,
            (uint8_t)CONFIG_SLE_TEAM_BUZZER_MUTED, (uint8_t)CONFIG_SLE_TEAM_BUZZER_AUTO_TOGGLE,
            g_team_rt.buzzer_level_on);
        return 1;
    }
#if CONFIG_SLE_TEAM_BUZZER_MUTED
    if (team_cli_match2(line, "buzz off", "buzz on") != 0U ||
        team_cli_match2(line, "buzz beep", "buzz test") != 0U ||
        (sscanf(line, "buzz beep %u", &ms) == 1 && ms > 0U && ms <= 2000U)) {
        uint8_t is_off = strcmp(line, "buzz off") == 0 ? 1U : 0U;
        team_buzzer_set(0U);
        osal_printk(is_off != 0U ? "[cli] buzz forced off pin=%u muted=1\r\n" :
            "[cli] buzz muted by firmware; command ignored pin=%u\r\n", g_team_rt.buzzer_pin);
        return 1;
    }
    if (strcmp(line, "buzz help") == 0) {
        osal_printk("[cli] buzz commands: status|off; sound commands are muted by firmware\r\n");
        return 1;
    }
    return 0;
#else
    if (team_cli_match2(line, "buzz on", "buzz off") != 0U) {
        uint8_t is_on = strcmp(line, "buzz on") == 0 ? 1U : 0U;
        team_buzzer_set(is_on);
        osal_printk(is_on != 0U ? "[cli] buzz set on pin=%u level_on=%u\r\n" :
            "[cli] buzz set off pin=%u level_on=%u\r\n", g_team_rt.buzzer_pin, g_team_rt.buzzer_level_on);
        return 1;
    }
    if (team_cli_match2(line, "buzz beep", "buzz test") != 0U ||
        (sscanf(line, "buzz beep %u", &ms) == 1 && ms > 0U && ms <= 2000U)) {
        team_buzzer_set(g_team_rt.buzzer_level_on == 0U ? 1U : 0U);
        osal_printk("[cli] buzz toggled pin=%u level_on=%u\r\n", g_team_rt.buzzer_pin, g_team_rt.buzzer_level_on);
        return 1;
    }
    if (strcmp(line, "buzz help") == 0) {
        osal_printk("[cli] buzz cmds: status|on|off|beep|test auto_toggle=%u interval_ms=%u\r\n",
            (uint8_t)CONFIG_SLE_TEAM_BUZZER_AUTO_TOGGLE, (uint32_t)SLE_TEAM_BUZZER_TOGGLE_INTERVAL_MS);
        return 1;
    }
    return 0;
#endif
#endif
}

static int team_display_cli_handle(const char *line)
{
    if (team_cli_match2(line, "disp", "disp status") != 0U) {
        osal_printk("[cli] disp ready=%u spi=%u sclk=%u sda=%u cs=%u cs_low=%u rs=%u rst=%u size=%ux%u off=%u,%u\r\n",
            g_team_rt.display_ready,
            (uint8_t)CONFIG_SLE_TEAM_ST7789_SPI_BUS,
            (uint8_t)CONFIG_SLE_TEAM_ST7789_SCLK_PIN,
            (uint8_t)CONFIG_SLE_TEAM_ST7789_MOSI_PIN,
            (uint8_t)CONFIG_SLE_TEAM_ST7789_CS_PIN,
            (uint8_t)CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW,
            (uint8_t)CONFIG_SLE_TEAM_ST7789_DC_PIN,
            (uint8_t)CONFIG_SLE_TEAM_ST7789_RESET_PIN,
            (uint16_t)CONFIG_SLE_TEAM_ST7789_WIDTH,
            (uint16_t)CONFIG_SLE_TEAM_ST7789_HEIGHT,
            (uint16_t)CONFIG_SLE_TEAM_ST7789_X_OFFSET,
            (uint16_t)CONFIG_SLE_TEAM_ST7789_Y_OFFSET);
        return 1;
    }
    if (strcmp(line, "disp refresh") == 0) {
        team_display_refresh_status();
        return 1;
    }
    if (strcmp(line, "disp demo") == 0) {
        team_display_show_event(TEAM_DISPLAY_EVENT_LOST, 99U, 31123456, 121987654, team_now_s(NULL));
        return 1;
    }
    if (strcmp(line, "disp help") == 0) {
        osal_printk("[cli] disp commands: disp status|refresh|demo\r\n");
        return 1;
    }
    return 0;
}

static int team_led_cli_handle(const char *line)
{
    unsigned int pin;
    uint8_t is_on;
    uint8_t is_tx;
    uint8_t is_active_low;

    if (team_cli_match2(line, "led", "led status") != 0U) {
        team_led_cli_status();
        return 1;
    }
    if (team_cli_match2(line, "led on", "led off") != 0U) {
        is_on = strcmp(line, "led on") == 0 ? 1U : 0U;
        team_led_set(is_on);
        team_led_cli_status();
        return 1;
    }
    if (team_cli_match2(line, "led tx", "led rx") != 0U) {
        is_tx = strcmp(line, "led tx") == 0 ? 1U : 0U;
        team_led_blink(is_tx != 0U ? SLE_TEAM_LED_TX_PULSES : SLE_TEAM_LED_RX_PULSES,
            is_tx != 0U ? SLE_TEAM_LED_TX_ON_MS : SLE_TEAM_LED_RX_ON_MS,
            is_tx != 0U ? SLE_TEAM_LED_TX_OFF_MS : SLE_TEAM_LED_RX_OFF_MS);
        team_led_cli_status();
        return 1;
    }
    if (team_cli_match2(line, "led active_low", "led active_high") != 0U) {
        is_active_low = strcmp(line, "led active_low") == 0 ? 1U : 0U;
        team_led_configure(g_team_rt.led_pin, is_active_low);
        return 1;
    }
    if (sscanf(line, "led pin %u", &pin) == 1 && pin <= 31U) {
        team_led_configure((uint8_t)pin, g_team_rt.led_active_low);
        return 1;
    }
    if (strcmp(line, "led help") == 0) {
        osal_printk("[cli] led commands: led status|on|off|tx|rx|active_low|active_high|pin <0-31>\r\n");
        return 1;
    }
    return 0;
}

static uint8_t team_serial_cfg_matches_runtime(const sle_team_web_config_nv_t *cfg)
{
    uint8_t leader_id;

    if (cfg == NULL || g_team_rt.role_configured == 0U) {
        return 0U;
    }
    if (g_team_node.cfg.team_id != cfg->team_id || g_team_node.cfg.channel_hash != cfg->channel_hash) {
        return 0U;
    }
    if (cfg->role == (uint8_t)SLE_TEAM_ROLE_LEADER) {
        return g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER ? 1U : 0U;
    }
    if (cfg->role != (uint8_t)SLE_TEAM_ROLE_MEMBER) {
        return 0U;
    }
    leader_id = team_route_id_from_suffix(cfg->leader_suffix);
    return (g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER && g_team_node.cfg.leader_id == leader_id) ? 1U : 0U;
}

static int team_serial_cfg_apply_loaded(const sle_team_web_config_nv_t *cfg)
{
    if (cfg == NULL) {
        return SLE_TEAM_ERR_FORMAT;
    }
    if (g_team_rt.role_request_pending != 0U) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    if (g_team_rt.role_configured != 0U) {
        if (team_serial_cfg_matches_runtime(cfg) != 0U) {
            osal_printk("[cfg] runtime already matches role=%u team=%u channel=%u leader_suffix=%04X\r\n",
                cfg->role, cfg->team_id, cfg->channel_hash, cfg->leader_suffix);
            return SLE_TEAM_OK;
        }
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    if (cfg->role == (uint8_t)SLE_TEAM_ROLE_LEADER) {
        return team_request_role_config(SLE_TEAM_ROLE_LEADER, g_team_rt.route_id, cfg->team_id, cfg->channel_hash,
            team_self_mac_suffix(), 1U);
    }
    return team_request_role_config(SLE_TEAM_ROLE_MEMBER, team_route_id_from_suffix(cfg->leader_suffix),
        cfg->team_id, cfg->channel_hash, cfg->leader_suffix, 1U);
}

static const char *team_cfg_role_name(uint8_t role, uint8_t valid)
{
    if (valid == 0U) {
        return "none";
    }
    return role == (uint8_t)SLE_TEAM_ROLE_LEADER ? "leader" : "member";
}

static uint8_t team_cfg_default_team(void)
{
    if (g_team_rt.role_configured != 0U &&
        g_team_node.cfg.team_id >= 1U && g_team_node.cfg.team_id != SLE_TEAM_BROADCAST_ID) {
        return g_team_node.cfg.team_id;
    }
    return (uint8_t)CONFIG_SLE_TEAM_TEAM_ID;
}

static uint8_t team_cfg_default_channel(void)
{
    if (g_team_rt.role_configured != 0U) {
        return g_team_node.cfg.channel_hash;
    }
    return (uint8_t)CONFIG_SLE_TEAM_CHANNEL_HASH;
}

static uint8_t team_leader_direct_capacity(void)
{
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER && g_team_node.cfg.max_downstream > 0U &&
        g_team_node.cfg.max_downstream <= SLE_TEAM_MAX_DIRECT_CONNECTIONS) {
        return g_team_node.cfg.max_downstream;
    }
    return SLE_TEAM_MAX_DIRECT_CONNECTIONS;
}

static uint8_t team_min_u8(uint8_t left, uint8_t right)
{
    return left < right ? left : right;
}

static uint8_t team_ceil_div_u16_to_u8(uint16_t numerator, uint16_t denominator)
{
    uint16_t result;

    if (denominator == 0U || numerator == 0U) {
        return 0U;
    }
    result = (uint16_t)((numerator + denominator - 1U) / denominator);
    return result > 255U ? 255U : (uint8_t)result;
}

static uint8_t team_leader_relay_budget_by_ram(void)
{
    uint16_t budget = (uint16_t)(SLE_TEAM_RELAY_MGMT_RAM_BUDGET_BYTES /
        SLE_TEAM_RELAY_MGMT_EST_BYTES_PER_RELAY);

    if (budget == 0U) {
        return 0U;
    }
    if (budget > SLE_TEAM_MAX_MEMBERS) {
        budget = SLE_TEAM_MAX_MEMBERS;
    }
    return (uint8_t)budget;
}

static uint8_t team_leader_relay_budget(void)
{
    uint8_t direct_capacity = team_leader_direct_capacity();
    uint8_t table_budget;
    uint8_t budget;

    if (direct_capacity == 0U || SLE_TEAM_MAX_MEMBERS <= direct_capacity) {
        return 0U;
    }

    table_budget = team_ceil_div_u16_to_u8((uint16_t)(SLE_TEAM_MAX_MEMBERS - direct_capacity),
        (uint16_t)SLE_TEAM_MAX_DIRECT_CONNECTIONS);
    budget = table_budget;
    budget = team_min_u8(budget, team_leader_relay_budget_by_ram());
    budget = team_min_u8(budget, (uint8_t)SLE_TEAM_RELAY_MGMT_HARD_MAX);
    /*
     * Current route policy keeps relay nodes directly acceptable to the leader.
     * Do not create more relay roles than the configured direct-cap can safely host.
     */
    budget = team_min_u8(budget, direct_capacity);
    return budget;
}

static int team_cfg_status_write_json(char *out, size_t out_size)
{
    sle_team_web_config_nv_t cfg;
    uint8_t nv_valid;
    uint8_t runtime_valid = g_team_rt.role_configured != 0U ? 1U : 0U;
    uint8_t runtime_role = runtime_valid != 0U ? (uint8_t)g_team_node.cfg.role : 0xFFU;
    uint8_t runtime_team = runtime_valid != 0U ? g_team_node.cfg.team_id : 0U;
    uint8_t runtime_channel = runtime_valid != 0U ? g_team_node.cfg.channel_hash : 0U;
    uint8_t runtime_leader = runtime_valid != 0U ? g_team_node.cfg.leader_id : 0U;
    uint8_t runtime_self = runtime_valid != 0U ? g_team_node.cfg.self_id : g_team_rt.route_id;
    int len;

    if (out == NULL || out_size == 0U) {
        return SLE_TEAM_ERR_ARG;
    }
    nv_valid = team_nv_config_load(&cfg) == SLE_TEAM_OK ? 1U : 0U;
    if (nv_valid == 0U) {
        (void)memset_s(&cfg, sizeof(cfg), 0, sizeof(cfg));
        cfg.role = 0xFFU;
    }

    len = snprintf(out, out_size,
        "{\"ok\":true,\"fw\":\"%s\",\"selfSuffix\":\"%04X\",\"routeId\":%u,"
        "\"nvValid\":%s,\"nvRole\":\"%s\",\"nvRoleValue\":%u,\"nvTeam\":%u,"
        "\"nvChannel\":%u,\"nvLeaderSuffix\":\"%04X\","
        "\"runtimeConfigured\":%s,\"runtimeRole\":\"%s\",\"runtimeRoleValue\":%u,"
        "\"runtimeTeam\":%u,\"runtimeChannel\":%u,\"runtimeLeader\":%u,\"runtimeSelf\":%u,"
        "\"runtimeDirectCap\":%u,\"runtimeRelayBudget\":%u,"
        "\"roleRequestPending\":%s,\"roleRequestRole\":\"%s\",\"roleRequestTeam\":%u,"
        "\"roleRequestChannel\":%u,\"roleRequestLeader\":%u,\"roleRequestLeaderSuffix\":\"%04X\","
        "\"roleRequestLastRet\":%d}",
        SLE_TEAM_FW_VERSION,
        team_self_mac_suffix(),
        g_team_rt.route_id,
        nv_valid != 0U ? "true" : "false",
        team_cfg_role_name(cfg.role, nv_valid),
        nv_valid != 0U ? cfg.role : 255U,
        nv_valid != 0U ? cfg.team_id : 0U,
        nv_valid != 0U ? cfg.channel_hash : 0U,
        nv_valid != 0U ? cfg.leader_suffix : 0U,
        runtime_valid != 0U ? "true" : "false",
        team_cfg_role_name(runtime_role, runtime_valid),
        runtime_role,
        runtime_team,
        runtime_channel,
        runtime_leader,
        runtime_self,
        runtime_valid != 0U && runtime_role == (uint8_t)SLE_TEAM_ROLE_LEADER ? team_leader_direct_capacity() : 0U,
        runtime_valid != 0U && runtime_role == (uint8_t)SLE_TEAM_ROLE_LEADER ? team_leader_relay_budget() : 0U,
        g_team_rt.role_request_pending != 0U ? "true" : "false",
        team_cfg_role_name(g_team_rt.role_request_role, g_team_rt.role_request_pending),
        g_team_rt.role_request_team,
        g_team_rt.role_request_channel,
        g_team_rt.role_request_leader,
        g_team_rt.role_request_leader_suffix,
        g_team_rt.role_request_last_ret);
    return (len > 0 && len < (int)out_size) ? len : SLE_TEAM_ERR_BUF;
}

static void team_serial_cfg_print_json(void)
{
    char json[704];
    int ret = team_cfg_status_write_json(json, sizeof(json));

    if (ret >= 0) {
        osal_printk("[cfg-json] %s\r\n", json);
    } else {
        osal_printk("[cfg-json] {\"ok\":false,\"error\":\"status\",\"ret\":%d}\r\n", ret);
    }
}

static int team_serial_cfg_cli_done(void)
{
    team_serial_cfg_print_json();
    return 1;
}

static int team_cfg_apply_role(sle_team_node_role_t role, uint8_t team, uint8_t channel, uint16_t leader_suffix)
{
    sle_team_web_config_nv_t cfg;

    (void)memset_s(&cfg, sizeof(cfg), 0, sizeof(cfg));
    cfg.role = (uint8_t)role;
    cfg.team_id = team;
    cfg.channel_hash = channel;
    cfg.leader_suffix = leader_suffix;
    return team_serial_cfg_apply_loaded(&cfg);
}

static int team_cfg_save_leader(uint8_t team, uint8_t channel, uint8_t apply_now)
{
    int ret;

    ret = team_nv_config_save(SLE_TEAM_ROLE_LEADER, team, team_self_mac_suffix(), channel);
    if (ret != SLE_TEAM_OK || apply_now == 0U) {
        return ret;
    }
    return team_cfg_apply_role(SLE_TEAM_ROLE_LEADER, team, channel, team_self_mac_suffix());
}

static int team_cfg_save_member(uint16_t leader_suffix, uint8_t team, uint8_t channel, uint8_t apply_now)
{
    int ret;

    ret = team_nv_config_save(SLE_TEAM_ROLE_MEMBER, team, leader_suffix, channel);
    if (ret != SLE_TEAM_OK || apply_now == 0U) {
        return ret;
    }
    return team_cfg_apply_role(SLE_TEAM_ROLE_MEMBER, team, channel, leader_suffix);
}

static int team_serial_cfg_cli_save_leader(uint8_t team, uint8_t channel, uint8_t apply_now)
{
    int ret = team_cfg_save_leader(team, channel, apply_now);

    osal_printk(apply_now != 0U ? "[cfg] leader-now queued ret=%d team=%u channel=%u self_suffix=%04X\r\n" :
        "[cfg] save leader ret=%d team=%u channel=%u self_suffix=%04X\r\n",
        ret, (unsigned int)team, (unsigned int)channel, team_self_mac_suffix());
    return team_serial_cfg_cli_done();
}

static int team_cfg_apply_saved(void)
{
    sle_team_web_config_nv_t cfg;

    if (team_nv_config_load(&cfg) != SLE_TEAM_OK) {
        return SLE_TEAM_ERR_FORMAT;
    }
    return team_serial_cfg_apply_loaded(&cfg);
}

static int team_serial_cfg_cli_handle(const char *line)
{
    unsigned int team = 0U;
    unsigned int channel = 0U;
    unsigned int suffix = 0U;
    unsigned int direct_cap = 0U;
    uint8_t save_team;
    uint8_t save_channel;
    int ret;

    if (team_cli_match2(line, "cfg", "cfg status") != 0U) {
        sle_team_web_config_nv_t cfg;
        if (team_nv_config_load(&cfg) == SLE_TEAM_OK) {
            osal_printk("[cfg] nv role=%u team=%u channel=%u leader_suffix=%04X\r\n",
                cfg.role, cfg.team_id, cfg.channel_hash, cfg.leader_suffix);
        } else {
            osal_printk("[cfg] nv empty\r\n");
        }
        osal_printk("[cfg] runtime configured=%u role=%u team=%u channel=%u leader=%u self=%u suffix=%04X direct_cap=%u relay_budget=%u\r\n",
            g_team_rt.role_configured,
            g_team_node.cfg.role,
            g_team_node.cfg.team_id,
            g_team_node.cfg.channel_hash,
            g_team_node.cfg.leader_id,
            g_team_node.cfg.self_id,
            team_self_mac_suffix(),
            g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER ? team_leader_direct_capacity() : 0U,
            g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER ? team_leader_relay_budget() : 0U);
        return team_serial_cfg_cli_done();
    }

    if (strcmp(line, "cfg direct") == 0) {
        osal_printk("[cfg] direct cap=%u relay_budget=%u hw_max=%u ret=0\r\n",
            g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER ? team_leader_direct_capacity() : 0U,
            g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER ? team_leader_relay_budget() : 0U,
            SLE_TEAM_MAX_DIRECT_CONNECTIONS);
        return team_serial_cfg_cli_done();
    }

    if (sscanf(line, "cfg direct %u", &direct_cap) == 1) {
        if (g_team_rt.role_configured == 0U || g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
            osal_printk("[cfg] direct ret=%d reason=not_leader\r\n", SLE_TEAM_ERR_UNSUPPORTED);
            return team_serial_cfg_cli_done();
        }
        if (direct_cap < 1U || direct_cap > SLE_TEAM_MAX_DIRECT_CONNECTIONS) {
            osal_printk("[cfg] direct ret=%d reason=range value=%u hw_max=%u\r\n",
                SLE_TEAM_ERR_ARG, direct_cap, SLE_TEAM_MAX_DIRECT_CONNECTIONS);
            return team_serial_cfg_cli_done();
        }
        g_team_node.cfg.max_downstream = (uint8_t)direct_cap;
        g_team_rt.relay_rebalance_last_s = 0U;
        team_leader_rebalance_relays(1U);
        team_leader_enforce_direct_capacity(1U);
        osal_printk("[cfg] direct cap=%u relay_budget=%u hw_max=%u ret=0\r\n",
            team_leader_direct_capacity(), team_leader_relay_budget(), SLE_TEAM_MAX_DIRECT_CONNECTIONS);
        return team_serial_cfg_cli_done();
    }

    if (strcmp(line, "cfg clear") == 0) {
        ret = team_cfg_clear_all_saved();
        osal_printk("[cfg] clear ret=%d scope=config+allowlist (reboot to apply unconfigured state)\r\n", ret);
        return team_serial_cfg_cli_done();
    }

    if (strcmp(line, "cfg reboot") == 0) {
        team_reboot_schedule("cfg-cli");
        osal_printk("[cfg] reboot scheduled\r\n");
        return 1;
    }

    if (strcmp(line, "cfg leader") == 0) {
        save_team = (g_team_node.cfg.team_id >= 1U && g_team_node.cfg.team_id != SLE_TEAM_BROADCAST_ID) ?
            g_team_node.cfg.team_id : (uint8_t)CONFIG_SLE_TEAM_TEAM_ID;
        save_channel = g_team_rt.role_configured != 0U ? g_team_node.cfg.channel_hash : (uint8_t)CONFIG_SLE_TEAM_CHANNEL_HASH;
        return team_serial_cfg_cli_save_leader(save_team, save_channel, 0U);
    }

    if (sscanf(line, "cfg leader %u %u", &team, &channel) == 2 &&
        team >= 1U && team <= 254U && channel <= 255U) {
        return team_serial_cfg_cli_save_leader((uint8_t)team, (uint8_t)channel, 0U);
    }

    if (sscanf(line, "cfg leader now %u %u", &team, &channel) == 2 &&
        team >= 1U && team <= 254U && channel <= 255U) {
        return team_serial_cfg_cli_save_leader((uint8_t)team, (uint8_t)channel, 1U);
    }

    if (sscanf(line, "cfg member %x %u %u", &suffix, &team, &channel) == 3 &&
        suffix >= 1U && suffix <= 0xFFFFU && team >= 1U && team <= 254U && channel <= 255U) {
        ret = team_cfg_save_member((uint16_t)suffix, (uint8_t)team, (uint8_t)channel, 0U);
        osal_printk("[cfg] save member ret=%d leader_suffix=%04X leader=%u team=%u channel=%u\r\n",
            ret, suffix, team_route_id_from_suffix((uint16_t)suffix), team, channel);
        return team_serial_cfg_cli_done();
    }

    if (sscanf(line, "cfg member now %x %u %u", &suffix, &team, &channel) == 3 &&
        suffix >= 1U && suffix <= 0xFFFFU && team >= 1U && team <= 254U && channel <= 255U) {
        ret = team_cfg_save_member((uint16_t)suffix, (uint8_t)team, (uint8_t)channel, 1U);
        osal_printk("[cfg] member-now queued ret=%d leader_suffix=%04X leader=%u team=%u channel=%u\r\n",
            ret, suffix, team_route_id_from_suffix((uint16_t)suffix), team, channel);
        return team_serial_cfg_cli_done();
    }

    if (strcmp(line, "cfg apply") == 0) {
        sle_team_web_config_nv_t cfg;
        if (team_nv_config_load(&cfg) != SLE_TEAM_OK) {
            osal_printk("[cfg] apply failed ret=%d reason=nv_empty\r\n", SLE_TEAM_ERR_FORMAT);
            return team_serial_cfg_cli_done();
        }
        ret = team_cfg_apply_saved();
        osal_printk("[cfg] apply queued ret=%d role=%u team=%u channel=%u leader_suffix=%04X\r\n",
            ret, cfg.role, cfg.team_id, cfg.channel_hash, cfg.leader_suffix);
        return team_serial_cfg_cli_done();
    }

    if (strcmp(line, "cfg help") == 0) {
        osal_printk("[cfg] cmds:\r\n");
        osal_printk("[cfg]   cfg status\r\n");
        osal_printk("[cfg]   cfg leader [team channel]\r\n");
        osal_printk("[cfg]   cfg leader now <team> <channel>\r\n");
        osal_printk("[cfg]   cfg direct [1-%u] (leader runtime direct capacity)\r\n",
            SLE_TEAM_MAX_DIRECT_CONNECTIONS);
        osal_printk("[cfg]   cfg member <leader_suffix_hex> <team> <channel>\r\n");
        osal_printk("[cfg]   cfg member now <leader_suffix_hex> <team> <channel>\r\n");
        osal_printk("[cfg]   cfg apply | cfg clear | cfg reboot\r\n");
        return 1;
    }
    return 0;
}

static void team_led_start(void)
{
    osal_task *task = NULL;

    if (osal_msg_queue_create("team_led_q", SLE_TEAM_LED_QUEUE_LEN, &g_team_rt.led_queue_id, 0,
        sizeof(uint8_t)) == OSAL_SUCCESS) {
        g_team_rt.led_queue_ready = 1U;
    } else {
        team_print("led queue create failed");
        return;
    }

    osal_kthread_lock();
    task = osal_kthread_create((osal_kthread_handler)team_led_task, NULL, "TeamLedTask",
        SLE_TEAM_LED_TASK_STACK_SIZE);
    if (task != NULL) {
        osal_kthread_set_priority(task, SLE_TEAM_LED_TASK_PRIO);
    }
    osal_kthread_unlock();
}

#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
static void team_wifi_print(const char *text)
{
    osal_printk("[team-wifi] %s\r\n", text);
}

static void team_identity_set_fallback(void)
{
    g_team_rt.route_id = CONFIG_SLE_TEAM_SELF_ID;
    (void)snprintf(g_team_rt.self_label, sizeof(g_team_rt.self_label), "U%02X",
        g_team_rt.route_id);
    (void)snprintf(g_team_rt.leader_label, sizeof(g_team_rt.leader_label), "L%02X",
        CONFIG_SLE_TEAM_LEADER_ID);
    (void)snprintf(g_team_rt.softap_ssid, sizeof(g_team_rt.softap_ssid), "%s-%02X",
        CONFIG_SLE_TEAM_WIFI_AP_SSID, g_team_rt.route_id);
}

static void team_identity_apply_to_node(void)
{
    if (g_team_rt.self_mac_ready == 0U) {
        return;
    }
    (void)memcpy(g_team_node.cfg.self_mac, g_team_rt.self_mac, sizeof(g_team_node.cfg.self_mac));
    g_team_node.cfg.self_mac_ready = 1U;
}

static uint16_t team_self_mac_suffix(void)
{
    if (g_team_rt.self_mac_ready == 0U) {
        return (uint16_t)g_team_rt.route_id;
    }
    return (uint16_t)(((uint16_t)g_team_rt.self_mac[4] << 8) | g_team_rt.self_mac[5]);
}

static uint8_t team_route_id_from_mac(const uint8_t mac[6])
{
    uint8_t id;

    if (mac == NULL) {
        return SLE_TEAM_ROUTE_ID_FALLBACK;
    }
    id = mac[5];
    if (id == 0U || id == SLE_TEAM_BROADCAST_ID) {
        id = (uint8_t)((mac[4] % 254U) + 1U);
    }
    return id;
}

static char team_role_prefix(void)
{
    if (g_team_rt.role_configured == 0U) {
        return 'U';
    }
    return g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER ? 'L' : 'M';
}

static void team_identity_refresh_labels(void)
{
    char prefix = team_role_prefix();

    if (g_team_rt.self_mac_ready != 0U) {
        (void)snprintf(g_team_rt.self_label, sizeof(g_team_rt.self_label), "%c%02X%02X",
            prefix, g_team_rt.self_mac[4], g_team_rt.self_mac[5]);
        (void)snprintf(g_team_rt.softap_ssid, sizeof(g_team_rt.softap_ssid), "%s-%02X%02X",
            CONFIG_SLE_TEAM_WIFI_AP_SSID, g_team_rt.self_mac[4], g_team_rt.self_mac[5]);
    } else {
        (void)snprintf(g_team_rt.self_label, sizeof(g_team_rt.self_label), "%c%02X",
            prefix, g_team_rt.route_id);
        (void)snprintf(g_team_rt.softap_ssid, sizeof(g_team_rt.softap_ssid), "%s-%02X",
            CONFIG_SLE_TEAM_WIFI_AP_SSID, g_team_rt.route_id);
    }
    if (g_team_rt.role_configured != 0U && g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        (void)snprintf(g_team_rt.leader_label, sizeof(g_team_rt.leader_label), "%s", g_team_rt.self_label);
    } else {
        (void)snprintf(g_team_rt.leader_label, sizeof(g_team_rt.leader_label), "L%02X",
            g_team_node.cfg.leader_id != 0U ? g_team_node.cfg.leader_id : CONFIG_SLE_TEAM_LEADER_ID);
    }
}

static void team_identity_format_from_mac(const uint8_t mac[6])
{
    unused(mac);
    team_identity_refresh_labels();
}

static void team_identity_format_route_label(uint8_t node_id, uint8_t role, const uint8_t mac[6], uint8_t mac_ready,
    char *out, size_t out_size)
{
    char prefix;
    uint8_t i;

    if (out == NULL || out_size == 0U) {
        return;
    }
    if (node_id == SLE_TEAM_BROADCAST_ID) {
        (void)snprintf(out, out_size, "ALL");
        return;
    }
    if (node_id == g_team_node.cfg.self_id && g_team_rt.self_label[0] != '\0') {
        (void)snprintf(out, out_size, "%s", g_team_rt.self_label);
        return;
    }
    if (node_id == g_team_node.cfg.leader_id && g_team_rt.leader_label[0] != '\0') {
        (void)snprintf(out, out_size, "%s", g_team_rt.leader_label);
        return;
    }
    prefix = (role == (uint8_t)SLE_TEAM_ROLE_LEADER) ? 'L' : 'M';
    if (mac_ready != 0U && mac != NULL) {
        (void)snprintf(out, out_size, "%c%02X%02X", prefix, mac[4], mac[5]);
        return;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];
        if (member->online != 0U && member->member_id == node_id && member->mac_ready != 0U) {
            (void)snprintf(out, out_size, "M%02X%02X", member->mac[4], member->mac[5]);
            return;
        }
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_pending_member_t *member = &g_team_node.pending_members[i];
        if (member->active != 0U && member->member_id == node_id && member->mac_ready != 0U) {
            (void)snprintf(out, out_size, "M%02X%02X", member->mac[4], member->mac[5]);
            return;
        }
    }
    (void)snprintf(out, out_size, "%c%02X", prefix, node_id);
}

static void team_identity_init_from_wifi_mac(void)
{
    int8_t mac[6] = {0};
    errcode_t ret;

    team_identity_set_fallback();
    ret = wifi_get_base_mac_addr(mac, sizeof(mac));
    if (ret != ERRCODE_SUCC) {
        osal_printk("[team-wifi] mac read failed ret=0x%x fallback_label=%s\r\n",
            ret, g_team_rt.self_label);
        return;
    }
    (void)memcpy(g_team_rt.self_mac, mac, sizeof(g_team_rt.self_mac));
    g_team_rt.self_mac_ready = 1U;
    g_team_rt.route_id = team_route_id_from_mac(g_team_rt.self_mac);
    team_identity_apply_to_node();
    team_identity_format_from_mac(g_team_rt.self_mac);
    team_display_refresh_status();
    osal_printk("[team-wifi] identity label=%s mac=%02X:%02X:%02X:%02X:%02X:%02X ssid=%s route=%u\r\n",
        g_team_rt.self_label,
        g_team_rt.self_mac[0], g_team_rt.self_mac[1], g_team_rt.self_mac[2],
        g_team_rt.self_mac[3], g_team_rt.self_mac[4], g_team_rt.self_mac[5],
        g_team_rt.softap_ssid,
        g_team_rt.route_id);
}

static void team_sle_prepare_local_addr(void)
{
    uint8_t sle_addr[6];

    if (g_team_rt.self_mac_ready == 0U) {
        return;
    }
    (void)memcpy_s(sle_addr, sizeof(sle_addr), g_team_rt.self_mac, sizeof(g_team_rt.self_mac));
    sle_addr[0] = (uint8_t)((sle_addr[0] | 0x02U) & 0xFEU);
    sle_uart_server_adv_set_local_addr(sle_addr);
    osal_printk("[team] sle local addr=%02X:%02X:%02X:%02X:%02X:%02X\r\n",
        sle_addr[0], sle_addr[1], sle_addr[2], sle_addr[3], sle_addr[4], sle_addr[5]);
}

static void team_wifi_print_status(void)
{
    osal_printk("[team-wifi] status wifi_inited=%ld softap_enabled=%ld ssid=%s label=%s ip=192.168.43.%u\r\n",
        (long)wifi_is_wifi_inited(),
        (long)wifi_is_softap_enabled(),
        g_team_rt.softap_ssid[0] != '\0' ? g_team_rt.softap_ssid : CONFIG_SLE_TEAM_WIFI_AP_SSID,
        g_team_rt.self_label[0] != '\0' ? g_team_rt.self_label : "NA",
        CONFIG_SLE_TEAM_WIFI_AP_IP_LAST);
}

static int team_tcpip_init_wait(void)
{
    uint32_t wait_ms = 0;

    if (tcpip_init_finish == 0) {
        team_wifi_print("tcpip init start");
        tcpip_init(NULL, NULL);
    }
    while (tcpip_init_finish == 0) {
        osal_msleep(100);
        wait_ms += 100;
        if (wait_ms >= SLE_TEAM_TCPIP_INIT_WAIT_MAX_MS) {
            team_wifi_print("tcpip init timeout");
            return -1;
        }
    }
    team_wifi_print("tcpip init ready");
    return 0;
}

static void team_http_print_status(void)
{
    osal_printk("[team-wifi] http ready=%u fd=%d errno=%d accepts=%lu tcpip=%ld role_pending=%u role_last_ret=%d\r\n",
        g_team_http_ready,
        g_team_http_listen_fd,
        g_team_http_last_errno,
        (unsigned long)g_team_http_accept_count,
        (long)tcpip_init_finish,
        g_team_rt.role_request_pending,
        g_team_rt.role_request_last_ret);
}
#endif

typedef enum {
    TEAM_LINK_UPSTREAM = 0,
    TEAM_LINK_DOWNSTREAM = 1,
} team_link_dir_t;

static uint8_t team_route_bucket_from_ids(uint8_t node_id, uint8_t leader_id)
{
    /* Keep the tier stable from the node id itself; leader is bucket 0. */
    if (node_id == 0U || node_id == SLE_TEAM_BROADCAST_ID || node_id == leader_id) {
        return 0U;
    }
    return (uint8_t)(((uint8_t)(node_id - 1U) % 3U) + 1U);
}

static uint8_t team_route_id_from_adv_data(const uint8_t *data, uint16_t len)
{
    uint16_t index = 0U;

    if (data == NULL) {
        return 0U;
    }
    while (index < len) {
        uint8_t field_len = data[index++];

        if (field_len == 0U) {
            continue;
        }
        if ((uint16_t)(index + field_len) > len) {
            break;
        }
        if (data[index] == SLE_ADV_DATA_TYPE_MANUFACTURER_SPECIFIC_DATA && field_len >= 4U &&
            data[index + 1U] == SLE_TEAM_ADV_ROUTE_MAGIC_0 &&
            data[index + 2U] == SLE_TEAM_ADV_ROUTE_MAGIC_1) {
            uint8_t route_id = data[index + 3U];

            if (route_id != 0U && route_id != SLE_TEAM_BROADCAST_ID) {
                return route_id;
            }
        }
        index = (uint16_t)(index + field_len);
    }
    return 0U;
}

static uint8_t team_route_id_from_sle_addr(const uint8_t addr[6])
{
    uint16_t mix;
    uint8_t route_id;

    if (addr == NULL) {
        return SLE_TEAM_ROUTE_ID_FALLBACK;
    }
    if (addr[5] != 0U && addr[5] != SLE_TEAM_BROADCAST_ID) {
        return addr[5];
    }
    /*
     * Fallback route id for legacy advertisements without explicit route byte.
     * Mix all address bytes to reduce collisions versus single-byte derivation.
     */
    mix = ((uint16_t)addr[0] << 8U) | addr[1];
    mix ^= ((uint16_t)addr[2] << 8U) | addr[3];
    mix ^= ((uint16_t)addr[4] << 8U) | addr[5];
    mix ^= (uint16_t)((uint16_t)addr[0] * 29U + (uint16_t)addr[2] * 7U + (uint16_t)addr[5] * 17U);
    route_id = (uint8_t)((mix % 254U) + 1U);
    if (route_id == 0U || route_id == SLE_TEAM_BROADCAST_ID) {
        return SLE_TEAM_ROUTE_ID_FALLBACK;
    }
    return route_id;
}

static uint32_t team_elapsed_s(uint32_t now_s, uint32_t since_s)
{
    return (uint32_t)(now_s - since_s);
}

static uint8_t team_interval_not_reached(uint32_t now_s, uint32_t last_s, uint32_t interval_s)
{
    if (last_s == 0U || interval_s == 0U) {
        return 0U;
    }
    return (uint8_t)(team_elapsed_s(now_s, last_s) < interval_s);
}

static uint8_t team_elapsed_exceeds(uint32_t now_s, uint32_t since_s, uint32_t limit_s)
{
    if (since_s == 0U || limit_s == 0U) {
        return 0U;
    }
    return (uint8_t)(team_elapsed_s(now_s, since_s) > limit_s);
}

static uint8_t team_route_bucket_for_self(void)
{
    return team_route_bucket_from_ids(g_team_node.cfg.self_id, g_team_node.cfg.leader_id);
}

static uint8_t team_conn_track_route_id_is_trusted(const team_conn_track_t *track)
{
    return (uint8_t)(track != NULL && track->route_id != 0U &&
        track->route_id != SLE_TEAM_BROADCAST_ID && track->route_id_provisional == 0U);
}

static uint8_t team_conn_track_route_id_is_valid(const team_conn_track_t *track)
{
    return (uint8_t)(track != NULL && track->route_id != 0U &&
        track->route_id != SLE_TEAM_BROADCAST_ID);
}

static uint8_t team_member_has_reselect_target(uint8_t parent_id)
{
    return (uint8_t)(g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER &&
        g_team_node.upstream_parent_state == SLE_TEAM_PARENT_RESELECTING &&
        g_team_node.upstream_parent_reselect_pending != 0U &&
        g_team_node.upstream_parent_id == parent_id &&
        parent_id != 0U && parent_id != SLE_TEAM_BROADCAST_ID &&
        parent_id != g_team_node.cfg.leader_id);
}

static uint8_t team_route_is_relay_enabled(void)
{
    return (uint8_t)(g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER && g_team_node.joined != 0U &&
        g_team_node.cfg.relay_allowed != 0U && g_team_node.cfg.relay_enabled != 0U);
}

static uint8_t team_route_control_packet_can_bridge(uint8_t app_msg_type)
{
    return (uint8_t)(app_msg_type == SLE_TEAM_APP_HELLO ||
        app_msg_type == SLE_TEAM_APP_ROUTE_UPDATE ||
        app_msg_type == SLE_TEAM_APP_CONFIG ||
        app_msg_type == SLE_TEAM_APP_ACK);
}

static uint8_t team_route_should_bridge_relay_control(const sle_team_app_packet_t *app_packet)
{
    if (app_packet == NULL || g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER ||
        g_team_node.cfg.relay_allowed == 0U ||
        app_packet->src_id == g_team_node.cfg.self_id ||
        app_packet->src_id == SLE_TEAM_BROADCAST_ID ||
        team_route_control_packet_can_bridge(app_packet->app_msg_type) == 0U) {
        return 0U;
    }
    if (g_team_node.state == SLE_TEAM_NET_IDLE) {
        return 0U;
    }
    return (uint8_t)(g_team_node.joined == 0U ||
        g_team_node.cfg.relay_enabled == 0U ||
        g_team_node.upstream_parent_state == SLE_TEAM_PARENT_RESELECTING ||
        g_team_node.upstream_parent_reselect_pending != 0U);
}

static uint8_t team_member_relay_can_accept_child(void)
{
    uint8_t max_downstream;

    if (team_route_is_relay_enabled() == 0U) {
        return 0U;
    }
    max_downstream = g_team_node.cfg.max_downstream != 0U ? g_team_node.cfg.max_downstream :
        SLE_TEAM_MAX_DIRECT_CONNECTIONS;
    return sle_uart_client_connected_count() < max_downstream ? 1U : 0U;
}

static uint8_t team_member_reselection_active(void)
{
    if (g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
        return 0U;
    }
    return (uint8_t)(g_team_node.joined == 0U &&
        g_team_node.upstream_parent_state == SLE_TEAM_PARENT_RESELECTING &&
        g_team_node.upstream_parent_reselect_pending != 0U &&
        g_team_node.cfg.relay_allowed != 0U);
}

static uint8_t team_sle_addr_equal(const sle_addr_t *left, const sle_addr_t *right)
{
    if (left == NULL || right == NULL) {
        return 0U;
    }
    return (uint8_t)(left->type == right->type && memcmp(left->addr, right->addr, sizeof(left->addr)) == 0);
}

static team_pending_conn_t *team_pending_conn_find(const sle_addr_t *addr)
{
    uint8_t i;

    if (addr == NULL) {
        return NULL;
    }
    for (i = 0U; i < TEAM_PENDING_CONN_MAX; i++) {
        if (g_team_pending_conns[i].active != 0U &&
            team_sle_addr_equal(&g_team_pending_conns[i].addr, addr) != 0U) {
            return &g_team_pending_conns[i];
        }
    }
    return NULL;
}

static void team_pending_conn_note(const sle_addr_t *addr, uint8_t route_id, uint8_t provisional)
{
    team_pending_conn_t *slot;
    team_pending_conn_t *oldest = NULL;
    uint8_t i;
    uint32_t now_s;

    if (addr == NULL || route_id == 0U || route_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    now_s = (uint32_t)(uapi_tcxo_get_ms() / 1000U);
    slot = team_pending_conn_find(addr);
    if (slot == NULL) {
        for (i = 0U; i < TEAM_PENDING_CONN_MAX; i++) {
            if (g_team_pending_conns[i].active == 0U) {
                slot = &g_team_pending_conns[i];
                break;
            }
            if (oldest == NULL ||
                team_elapsed_s(now_s, g_team_pending_conns[i].last_seen_s) >
                    team_elapsed_s(now_s, oldest->last_seen_s)) {
                oldest = &g_team_pending_conns[i];
            }
        }
    }
    if (slot == NULL) {
        slot = oldest;
    }
    if (slot == NULL) {
        return;
    }
    (void)memset_s(slot, sizeof(*slot), 0, sizeof(*slot));
    slot->active = 1U;
    (void)memcpy_s(&slot->addr, sizeof(slot->addr), addr, sizeof(*addr));
    slot->route_id = route_id;
    slot->route_id_provisional = provisional != 0U ? 1U : 0U;
    slot->last_seen_s = now_s;
}

static void team_pending_conn_clear(const sle_addr_t *addr)
{
    team_pending_conn_t *slot = team_pending_conn_find(addr);

    if (slot != NULL) {
        (void)memset_s(slot, sizeof(*slot), 0, sizeof(*slot));
    }
}

static team_route_entry_t *team_route_entry_find(uint8_t member_id)
{
    uint8_t i;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return NULL;
    }
    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        if (g_team_routes[i].active != 0U && g_team_routes[i].member_id == member_id) {
            return &g_team_routes[i];
        }
    }
    return NULL;
}

static team_route_entry_t *team_route_entry_alloc(uint8_t member_id)
{
    uint8_t i;
    team_route_entry_t *route = team_route_entry_find(member_id);

    if (route != NULL) {
        return route;
    }
    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        if (g_team_routes[i].active == 0U) {
            (void)memset_s(&g_team_routes[i], sizeof(g_team_routes[i]), 0, sizeof(g_team_routes[i]));
            g_team_routes[i].active = 1U;
            g_team_routes[i].member_id = member_id;
            return &g_team_routes[i];
        }
    }
    return NULL;
}

static uint8_t team_route_conn_is_active(team_conn_dir_t dir, uint16_t conn_id)
{
    /* WS63 SLE reports conn_id 0 as a valid ACL connection; let the owner table decide liveness. */
    if (dir == TEAM_CONN_DIR_DOWNSTREAM) {
        return sle_uart_client_has_conn(conn_id);
    }
    if (dir == TEAM_CONN_DIR_UPSTREAM) {
        return sle_uart_server_has_conn(conn_id);
    }
    return 0U;
}

static void team_route_metrics_mark_dirty(void)
{
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        g_team_rt.route_metrics_last_s = 0U;
    }
}

static void team_route_note(uint8_t member_id, uint16_t conn_id, team_conn_dir_t dir, uint8_t next_hop_id)
{
    team_route_entry_t *route;
    uint8_t is_new;
    uint8_t changed;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID || dir == TEAM_CONN_DIR_UNKNOWN) {
        return;
    }
    route = team_route_entry_find(member_id);
    is_new = route == NULL ? 1U : 0U;
    route = team_route_entry_alloc(member_id);
    if (route == NULL) {
        return;
    }
    changed = (uint8_t)(is_new != 0U || route->conn_id != conn_id || route->dir != dir ||
        route->next_hop_id != next_hop_id);
    route->conn_id = conn_id;
    route->dir = dir;
    route->next_hop_id = next_hop_id;
    route->last_seen_s = (uint32_t)(uapi_tcxo_get_ms() / 1000U);
    if (changed != 0U) {
        team_route_metrics_mark_dirty();
        osal_printk("[team] route note member=%u conn=%u dir=%u next=%u new=%u\r\n",
            member_id, conn_id, (uint8_t)dir, next_hop_id, is_new);
    }
    team_leader_failover_note_route_recovered(member_id, next_hop_id);
}

static uint8_t team_leader_route_physical_parent_matches(const team_route_entry_t *route, uint8_t parent_id)
{
    uint8_t conn_member_id = 0U;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER || route == NULL || route->active == 0U ||
        route->dir != TEAM_CONN_DIR_DOWNSTREAM ||
        parent_id == 0U || parent_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    if (sle_uart_client_get_conn_member(route->conn_id, &conn_member_id) == 0U ||
        conn_member_id == 0U || conn_member_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    if (parent_id == g_team_node.cfg.leader_id || parent_id == g_team_node.cfg.self_id) {
        return conn_member_id == route->member_id ? 1U : 0U;
    }
    return conn_member_id == parent_id ? 1U : 0U;
}

static uint8_t team_leader_failover_has_member(uint8_t member_id)
{
    uint8_t i;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (g_team_rt.relay_failover_member_ids[i] == member_id) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t team_leader_failover_watch_member(uint8_t member_id)
{
    uint8_t i;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID || team_leader_failover_has_member(member_id) != 0U) {
        return 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (g_team_rt.relay_failover_member_ids[i] == 0U) {
            g_team_rt.relay_failover_member_ids[i] = member_id;
            return 1U;
        }
    }
    return 0U;
}

static uint8_t team_leader_failover_should_seek_member(uint8_t member_id)
{
    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID ||
        team_leader_failover_window_active(team_now_s(NULL)) == 0U) {
        return 0U;
    }
    if (member_id == g_team_rt.relay_failover_lost_id) {
        return 1U;
    }
    return team_leader_failover_has_member(member_id);
}

static uint8_t team_leader_failover_window_active(uint32_t now_s)
{
    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER || g_team_rt.relay_failover_active == 0U) {
        return 0U;
    }
    if (team_elapsed_exceeds(now_s, g_team_rt.relay_failover_start_s, SLE_TEAM_RELAY_FAILOVER_GRACE_S) != 0U) {
        osal_printk("[team] relay failover expired lost=%u grace=%u\r\n",
            g_team_rt.relay_failover_lost_id, SLE_TEAM_RELAY_FAILOVER_GRACE_S);
        g_team_rt.relay_failover_active = 0U;
        g_team_rt.relay_failover_lost_id = 0U;
        (void)memset_s(g_team_rt.relay_failover_member_ids, sizeof(g_team_rt.relay_failover_member_ids), 0,
            sizeof(g_team_rt.relay_failover_member_ids));
        return 0U;
    }
    return 1U;
}

static void team_leader_failover_clear_member(uint8_t member_id)
{
    uint8_t i;
    uint8_t watched_left = 0U;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (g_team_rt.relay_failover_member_ids[i] == member_id) {
            g_team_rt.relay_failover_member_ids[i] = 0U;
        } else if (g_team_rt.relay_failover_member_ids[i] != 0U) {
            watched_left = 1U;
        }
    }
    if (watched_left == 0U && g_team_rt.relay_failover_active != 0U) {
        osal_printk("[team] relay failover recovered lost=%u\r\n", g_team_rt.relay_failover_lost_id);
        g_team_rt.relay_failover_active = 0U;
        g_team_rt.relay_failover_lost_id = 0U;
    }
}

static uint8_t team_leader_relay_config_is_pending(uint8_t member_id)
{
    uint8_t i;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (g_team_rt.relay_config_pending_member_ids[i] == member_id) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t team_leader_relay_parent_is_ready(uint8_t parent_id)
{
    uint8_t i;
    uint16_t conn_id = 0U;

    if (parent_id == 0U || parent_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    if (parent_id == g_team_node.cfg.leader_id || parent_id == g_team_node.cfg.self_id) {
        return 1U;
    }
    if (team_leader_relay_config_is_pending(parent_id) != 0U) {
        return 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];

        if (member->member_id == parent_id) {
            if (member->online == 0U || member->relay_allowed == 0U) {
                return 0U;
            }
            break;
        }
    }
    if (i == SLE_TEAM_MAX_MEMBERS) {
        return 0U;
    }
    if (sle_uart_client_find_conn_by_member(parent_id, &conn_id) == 0U) {
        return 0U;
    }
    return team_route_conn_is_active(TEAM_CONN_DIR_DOWNSTREAM, conn_id);
}

static uint8_t team_leader_failover_recovery_next_hop_is_valid(uint8_t member_id, uint8_t next_hop_id)
{
    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID ||
        next_hop_id == 0U || next_hop_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    if (next_hop_id == member_id || next_hop_id == g_team_node.cfg.leader_id ||
        next_hop_id == g_team_node.cfg.self_id) {
        return 1U;
    }
    if (next_hop_id == g_team_rt.relay_failover_lost_id) {
        return 0U;
    }
    return team_leader_relay_parent_is_ready(next_hop_id);
}

static void team_leader_failover_note_route_recovered(uint8_t member_id, uint8_t next_hop_id)
{
    const team_route_entry_t *route;

    if (team_leader_failover_window_active(team_now_s(NULL)) == 0U ||
        team_leader_failover_has_member(member_id) == 0U) {
        return;
    }
    if (team_leader_failover_recovery_next_hop_is_valid(member_id, next_hop_id) == 0U) {
        osal_printk("[team] relay failover member=%u route pending next_hop=%u\r\n", member_id, next_hop_id);
        return;
    }
    route = team_route_entry_find(member_id);
    if (team_leader_route_physical_parent_matches(route, next_hop_id) == 0U) {
        osal_printk("[team] relay failover member=%u route pending next_hop=%u reason=physical-parent\r\n",
            member_id, next_hop_id);
        return;
    }
    osal_printk("[team] relay failover member=%u route recovered next_hop=%u\r\n", member_id, next_hop_id);
    team_leader_failover_clear_member(member_id);
}

static void team_leader_failover_begin(uint8_t lost_relay_id, uint32_t now_s)
{
    uint8_t i;
    uint8_t watched = 0U;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER || lost_relay_id == 0U ||
        lost_relay_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    if (g_team_rt.relay_failover_active != 0U &&
        g_team_rt.relay_failover_lost_id == lost_relay_id &&
        team_leader_failover_window_active(now_s) != 0U) {
        osal_printk("[team] relay failover duplicate lost=%u grace=%u\r\n",
            lost_relay_id, SLE_TEAM_RELAY_FAILOVER_GRACE_S);
        return;
    }
    (void)memset_s(g_team_rt.relay_failover_member_ids, sizeof(g_team_rt.relay_failover_member_ids), 0,
        sizeof(g_team_rt.relay_failover_member_ids));
    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        const team_route_entry_t *route = &g_team_routes[i];

        if (route->active == 0U || route->member_id == lost_relay_id ||
            route->member_id == 0U || route->member_id == SLE_TEAM_BROADCAST_ID ||
            route->next_hop_id != lost_relay_id) {
            continue;
        }
        watched = (uint8_t)(watched + team_leader_failover_watch_member(route->member_id));
    }
    g_team_rt.relay_failover_active = watched != 0U ? 1U : 0U;
    g_team_rt.relay_failover_lost_id = watched != 0U ? lost_relay_id : 0U;
    g_team_rt.relay_failover_start_s = now_s;
    g_team_rt.member_rescan_last_s = 0U;
    g_team_rt.relay_rebalance_last_s = 0U;
    g_team_rt.route_metrics_last_s = 0U;
    osal_printk("[team] relay failover begin lost=%u watched=%u grace=%u\r\n",
        lost_relay_id, watched, SLE_TEAM_RELAY_FAILOVER_GRACE_S);
}

static uint8_t team_route_next_hop_is_direct_peer(uint8_t member_id, uint8_t next_hop_id)
{
    /* Direct next-hop ids use the recorded physical conn_id, not a member lookup. */
    return (uint8_t)(next_hop_id == 0U || next_hop_id == SLE_TEAM_BROADCAST_ID ||
        next_hop_id == member_id || next_hop_id == g_team_node.cfg.leader_id ||
        next_hop_id == g_team_node.cfg.self_id);
}

static uint8_t team_route_find(uint8_t member_id, team_conn_dir_t dir, uint16_t *conn_id)
{
    team_route_entry_t *route = team_route_entry_find(member_id);

    if (route == NULL || route->dir != dir || conn_id == NULL) {
        return 0U;
    }
    if (team_route_next_hop_is_direct_peer(member_id, route->next_hop_id) == 0U) {
        if (dir == TEAM_CONN_DIR_DOWNSTREAM) {
            if (sle_uart_client_find_conn_by_member(route->next_hop_id, conn_id) != 0U &&
                team_route_conn_is_active(dir, *conn_id) != 0U) {
                return 1U;
            }
        } else if (sle_uart_server_find_conn_by_member_ex(route->next_hop_id, conn_id) != 0U &&
            team_route_conn_is_active(dir, *conn_id) != 0U) {
            return 1U;
        }
        /* next-hop route exists but is not currently reachable */
        return 0U;
    }
    if (team_route_conn_is_active(dir, route->conn_id) != 0U) {
        *conn_id = route->conn_id;
        return 1U;
    }
    return 0U;
}

static void team_route_clear_by_conn(uint16_t conn_id)
{
    uint8_t i;
    uint8_t cleared = 0U;

    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        if (g_team_routes[i].active != 0U && g_team_routes[i].conn_id == conn_id) {
            if (team_leader_failover_should_preserve_route(g_team_routes[i].member_id,
                    g_team_routes[i].next_hop_id) != 0U) {
                continue;
            }
            (void)memset_s(&g_team_routes[i], sizeof(g_team_routes[i]), 0, sizeof(g_team_routes[i]));
            cleared = 1U;
        }
    }
    if (cleared != 0U) {
        team_route_metrics_mark_dirty();
        osal_printk("[team] route clear conn=%u\r\n", conn_id);
    }
}

static void team_route_clear_by_next_hop(uint8_t next_hop_id)
{
    uint8_t i;
    uint8_t cleared = 0U;

    if (next_hop_id == 0U || next_hop_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        if (g_team_routes[i].active != 0U && g_team_routes[i].next_hop_id == next_hop_id) {
            if (team_leader_failover_should_preserve_route(g_team_routes[i].member_id,
                    g_team_routes[i].next_hop_id) != 0U) {
                continue;
            }
            (void)memset_s(&g_team_routes[i], sizeof(g_team_routes[i]), 0, sizeof(g_team_routes[i]));
            cleared = 1U;
        }
    }
    if (cleared != 0U) {
        team_route_metrics_mark_dirty();
        osal_printk("[team] route clear next_hop=%u\r\n", next_hop_id);
    }
}

static uint8_t team_leader_failover_should_preserve_route(uint8_t member_id, uint8_t next_hop_id)
{
    unused(member_id);
    unused(next_hop_id);
    /*
     * The watched-member list is enough to keep failover policy active. Keeping
     * stale routes through the lost relay makes route_find() prefer a dead next
     * hop and blocks the direct-cap migration that repairs the topology.
     */
    return 0U;
}

static uint8_t team_route_member_by_conn(uint16_t conn_id, team_conn_dir_t dir, uint8_t *member_id)
{
    uint8_t i;

    if (member_id == NULL) {
        return 0U;
    }
    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        if (g_team_routes[i].active == 0U || g_team_routes[i].conn_id != conn_id) {
            continue;
        }
        if (dir != TEAM_CONN_DIR_UNKNOWN && g_team_routes[i].dir != dir) {
            continue;
        }
        if (g_team_routes[i].member_id == 0U || g_team_routes[i].member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        *member_id = g_team_routes[i].member_id;
        return 1U;
    }
    return 0U;
}

static sle_team_member_record_t *team_leader_find_member_slot(uint8_t member_id)
{
    uint8_t i;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return NULL;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        sle_team_member_record_t *member = &g_team_node.members[i];

        if (member->member_id == member_id) {
            return member;
        }
    }
    return NULL;
}

static uint8_t team_leader_mark_member_offline(uint8_t member_id, const char *reason)
{
    sle_team_member_record_t *member;
    uint8_t was_relay;
    uint32_t now_s;
    uint8_t already_offline = 0U;
    uint8_t last_display_event = (uint8_t)TEAM_DISPLAY_EVENT_NONE;
    team_display_event_t event = TEAM_DISPLAY_EVENT_LOST;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return 0U;
    }
    member = team_leader_find_member_slot(member_id);
    if (member == NULL) {
        osal_printk("[team] member offline skip id=%u reason=%s missing_record=1\r\n",
            member_id, reason != NULL ? reason : "unknown");
        return 0U;
    }

    now_s = team_now_s(NULL);
    if (reason != NULL && strcmp(reason, "member_leave") == 0) {
        event = TEAM_DISPLAY_EVENT_LEFT;
    }
    if (member->online == 0U) {
        already_offline = 1U;
        (void)team_display_member_last_event(member_id, &last_display_event);
        if (reason == NULL || strcmp(reason, "conn_disconnected") != 0) {
            osal_printk("[team] member offline skip id=%u reason=%s already_offline=1 last_event=%u\r\n",
                member_id, reason != NULL ? reason : "unknown", last_display_event);
            return 0U;
        }
        if (last_display_event == (uint8_t)TEAM_DISPLAY_EVENT_LEFT) {
            osal_printk("[team] member offline skip id=%u reason=conn_disconnected already_offline=1 last_event=LEFT\r\n",
                member_id);
            return 0U;
        }
    } else {
        member->online = 0U;
    }
    if (member->last_seen_s == 0U && now_s != 0U) {
        member->last_seen_s = now_s;
    }
    was_relay = already_offline == 0U ? member->relay_allowed : 0U;
    if (was_relay != 0U) {
        team_leader_failover_begin(member_id, now_s);
    }
    member->relay_allowed = 0U;
    member->relay_tier = 0U;
    member->max_downstream = 0U;
    team_route_clear_by_next_hop(member_id);

    if (was_relay != 0U && g_team_node.ops.on_relay_offline != NULL) {
        g_team_node.ops.on_relay_offline(g_team_node.ops.user_ctx, member_id);
    }

    team_display_note_member_offline(member->member_id, event);
    team_display_show_event(event, member->member_id, member->latitude_e6,
        member->longitude_e6, member->last_seen_s);
    g_team_rt.last_online_member_count = team_online_member_count();
    team_display_refresh_status();
    osal_printk("[team] member offline id=%u reason=%s last_seen=%lu already_offline=%u last_event=%u\r\n",
        member->member_id, reason != NULL ? reason : "unknown", (unsigned long)member->last_seen_s,
        already_offline, last_display_event);
    return 1U;
}

static uint8_t team_route_entry_is_stale(const team_route_entry_t *entry, uint32_t now_s, uint16_t timeout_s)
{
    if (entry == NULL || entry->active == 0U) {
        return 0U;
    }
    return team_elapsed_exceeds(now_s, entry->last_seen_s, (uint32_t)timeout_s * SLE_TEAM_ROUTE_STALE_FACTOR);
}

static uint8_t team_leader_member_is_online_relay(uint8_t member_id)
{
    const sle_team_member_record_t *member = team_display_find_member_record(member_id);

    return (uint8_t)(member != NULL && member->online != 0U && member->relay_allowed != 0U);
}

static uint8_t team_leader_online_relay_count(void)
{
    uint8_t i;
    uint8_t count = 0U;

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];

        if (member->online != 0U && member->relay_allowed != 0U &&
            member->member_id != 0U && member->member_id != SLE_TEAM_BROADCAST_ID) {
            count++;
        }
    }
    return count;
}

static uint8_t team_leader_member_has_downstream_children(uint8_t member_id)
{
    uint8_t i;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID ||
        member_id == g_team_node.cfg.self_id || member_id == g_team_node.cfg.leader_id) {
        return 0U;
    }
    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        const team_route_entry_t *route = &g_team_routes[i];

        if (route->active == 0U || route->dir != TEAM_CONN_DIR_DOWNSTREAM ||
            route->member_id == 0U || route->member_id == SLE_TEAM_BROADCAST_ID ||
            route->member_id == member_id || route->next_hop_id != member_id) {
            continue;
        }
        if (team_route_conn_is_active(route->dir, route->conn_id) != 0U) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t team_leader_member_should_stay_direct(const sle_team_member_record_t *member)
{
    uint8_t i;
    uint8_t direct_capacity;
    uint8_t relay_count;
    uint8_t direct_leaf_slots;
    uint8_t leaf_rank = 0U;

    if (member == NULL || member->online == 0U ||
        member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    if (member->relay_allowed != 0U) {
        return 1U;
    }
    if (team_leader_member_has_downstream_children(member->member_id) != 0U) {
        return 1U;
    }

    direct_capacity = team_leader_direct_capacity();
    relay_count = team_leader_online_relay_count();
    if (relay_count >= direct_capacity) {
        return 0U;
    }
    direct_leaf_slots = (uint8_t)(direct_capacity - relay_count);
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *candidate = &g_team_node.members[i];

        if (candidate->online == 0U || candidate->relay_allowed != 0U ||
            candidate->member_id == 0U || candidate->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        if (candidate->member_id <= member->member_id) {
            leaf_rank++;
        }
    }
    return (uint8_t)(leaf_rank != 0U && leaf_rank <= direct_leaf_slots);
}

static void team_leader_direct_prune_mark(uint16_t conn_id, uint8_t member_id)
{
    uint8_t i;
    uint8_t free_index = SLE_TEAM_MAX_DIRECT_CONNECTIONS;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    for (i = 0U; i < SLE_TEAM_MAX_DIRECT_CONNECTIONS; i++) {
        if (g_team_rt.direct_prune_conn_ids[i] == conn_id ||
            g_team_rt.direct_prune_member_ids[i] == member_id) {
            g_team_rt.direct_prune_conn_ids[i] = conn_id;
            g_team_rt.direct_prune_member_ids[i] = member_id;
            return;
        }
        if (g_team_rt.direct_prune_member_ids[i] == 0U && free_index == SLE_TEAM_MAX_DIRECT_CONNECTIONS) {
            free_index = i;
        }
    }
    if (free_index != SLE_TEAM_MAX_DIRECT_CONNECTIONS) {
        g_team_rt.direct_prune_conn_ids[free_index] = conn_id;
        g_team_rt.direct_prune_member_ids[free_index] = member_id;
    }
}

static uint8_t team_leader_direct_prune_consume(uint16_t conn_id, uint8_t member_id)
{
    uint8_t i;

    for (i = 0U; i < SLE_TEAM_MAX_DIRECT_CONNECTIONS; i++) {
        if (g_team_rt.direct_prune_member_ids[i] == 0U) {
            continue;
        }
        if (g_team_rt.direct_prune_conn_ids[i] == conn_id ||
            (member_id != 0U && g_team_rt.direct_prune_member_ids[i] == member_id)) {
            g_team_rt.direct_prune_conn_ids[i] = 0U;
            g_team_rt.direct_prune_member_ids[i] = 0U;
            return 1U;
        }
    }
    return 0U;
}

static uint8_t team_leader_select_relay_parent(uint8_t member_id)
{
    const team_route_entry_t *route;
    uint8_t relay_count;
    uint8_t target_index;
    uint8_t current_index = 0U;
    uint8_t i;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }

    route = team_route_entry_find(member_id);
    if (route != NULL && route->active != 0U &&
        route->next_hop_id != 0U && route->next_hop_id != SLE_TEAM_BROADCAST_ID &&
        route->next_hop_id != member_id && route->next_hop_id != g_team_node.cfg.leader_id &&
        route->next_hop_id != g_team_node.cfg.self_id &&
        team_leader_member_is_online_relay(route->next_hop_id) != 0U) {
        return route->next_hop_id;
    }

    relay_count = team_leader_online_relay_count();
    if (relay_count == 0U) {
        return 0U;
    }
    target_index = (uint8_t)(member_id % relay_count);
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *relay = &g_team_node.members[i];

        if (relay->online == 0U || relay->relay_allowed == 0U ||
            relay->member_id == 0U || relay->member_id == SLE_TEAM_BROADCAST_ID ||
            relay->member_id == member_id) {
            continue;
        }
        if (current_index == target_index) {
            return relay->member_id;
        }
        current_index++;
    }
    return 0U;
}

static uint8_t team_leader_desired_parent_for_member(const sle_team_member_record_t *member, uint8_t *out_parent_id)
{
    uint8_t relay_parent;

    if (member == NULL || out_parent_id == NULL || member->online == 0U ||
        member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID ||
        member->member_id == g_team_node.cfg.self_id) {
        return 0U;
    }
    if (team_leader_member_should_stay_direct(member) != 0U) {
        *out_parent_id = g_team_node.cfg.leader_id;
        return 1U;
    }
    relay_parent = team_leader_select_relay_parent(member->member_id);
    *out_parent_id = relay_parent != 0U ? relay_parent : g_team_node.cfg.leader_id;
    return 1U;
}

static uint8_t team_leader_route_parent_matches(const sle_team_member_record_t *member,
    const team_route_entry_t *route, uint8_t desired_parent_id)
{
    uint8_t actual_parent_id;

    if (member == NULL || route == NULL || route->active == 0U || desired_parent_id == 0U ||
        desired_parent_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    actual_parent_id = route->next_hop_id;
    if (actual_parent_id == 0U || actual_parent_id == SLE_TEAM_BROADCAST_ID ||
        actual_parent_id == member->member_id || actual_parent_id == g_team_node.cfg.leader_id ||
        actual_parent_id == g_team_node.cfg.self_id) {
        actual_parent_id = g_team_node.cfg.leader_id;
    }
    if (actual_parent_id != desired_parent_id) {
        return 0U;
    }
    return team_leader_route_physical_parent_matches(route, desired_parent_id);
}

static uint8_t team_leader_route_hint_parent_for_member(const sle_team_member_record_t *member, uint8_t *out_parent_id)
{
    return team_leader_desired_parent_for_member(member, out_parent_id);
}

static void team_leader_reconcile_online_routes(uint32_t now_s)
{
    uint8_t i;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];
        team_route_entry_t *route;
        uint8_t desired_parent_id = 0U;
        uint16_t parent_conn_id = 0U;
        uint8_t route_missing;
        uint8_t route_unreachable;

        if (member->online == 0U || member->member_id == 0U ||
            member->member_id == SLE_TEAM_BROADCAST_ID ||
            team_leader_desired_parent_for_member(member, &desired_parent_id) == 0U) {
            continue;
        }
        route = team_route_entry_find(member->member_id);
        route_missing = (uint8_t)(route == NULL || route->active == 0U);
        route_unreachable = (uint8_t)(route != NULL && route->active != 0U &&
            team_route_conn_is_active(route->dir, route->conn_id) == 0U);
        if (route_missing == 0U && route_unreachable == 0U) {
            continue;
        }
        if (desired_parent_id == g_team_node.cfg.leader_id || desired_parent_id == g_team_node.cfg.self_id) {
            if (sle_uart_client_find_conn_by_member(member->member_id, &parent_conn_id) == 0U ||
                team_route_conn_is_active(TEAM_CONN_DIR_DOWNSTREAM, parent_conn_id) == 0U) {
                continue;
            }
        } else {
            if (team_leader_relay_parent_is_ready(desired_parent_id) == 0U ||
                sle_uart_client_find_conn_by_member(desired_parent_id, &parent_conn_id) == 0U ||
                team_route_conn_is_active(TEAM_CONN_DIR_DOWNSTREAM, parent_conn_id) == 0U) {
                continue;
            }
        }
        team_route_note(member->member_id, parent_conn_id, TEAM_CONN_DIR_DOWNSTREAM, desired_parent_id);
        osal_printk("[team] route reconcile member=%u parent=%u conn=%u missing=%u unreachable=%u\r\n",
            member->member_id, desired_parent_id, parent_conn_id, route_missing, route_unreachable);
    }
    unused(now_s);
}

static uint8_t team_leader_member_should_accept_direct(uint8_t member_id)
{
    sle_team_member_record_t *member;
    uint8_t desired_parent_id = 0U;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    member = team_leader_find_member_slot(member_id);
    if (member == NULL || member->online == 0U) {
        return team_leader_online_relay_count() == 0U ? 1U : 0U;
    }
    if (member->relay_allowed != 0U || team_leader_member_should_stay_direct(member) != 0U) {
        return 1U;
    }
    if (team_leader_desired_parent_for_member(member, &desired_parent_id) == 0U) {
        return 1U;
    }
    return desired_parent_id == g_team_node.cfg.leader_id ? 1U : 0U;
}

static void team_leader_enforce_direct_capacity(uint8_t force_now)
{
    uint32_t now_s;
    uint16_t conn_ids[SLE_TEAM_MAX_DIRECT_CONNECTIONS];
    uint8_t conn_count;
    uint8_t i;
    uint8_t pruned = 0U;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER || g_team_node.cfg.pairing_enabled != 0U) {
        return;
    }
    now_s = team_now_s(NULL);
    if (force_now == 0U &&
        team_interval_not_reached(now_s, g_team_rt.direct_enforce_last_s, SLE_TEAM_DIRECT_ENFORCE_INTERVAL_S) != 0U) {
        return;
    }
    g_team_rt.direct_enforce_last_s = now_s;

    conn_count = sle_uart_client_get_active_conns(conn_ids, (uint8_t)SLE_TEAM_MAX_DIRECT_CONNECTIONS);
    if (conn_count <= team_leader_direct_capacity()) {
        return;
    }

    for (i = 0U; i < conn_count; i++) {
        uint8_t member_id = 0U;
        sle_team_member_record_t *member;
        uint8_t desired_parent_id = 0U;
        int hint_ret = SLE_TEAM_ERR_UNSUPPORTED;
        uint8_t route_matches;
        uint8_t allow_prune = 1U;

        if (sle_uart_client_get_conn_member(conn_ids[i], &member_id) == 0U ||
            member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        member = team_leader_find_member_slot(member_id);
        if (member == NULL || member->online == 0U ||
            team_leader_member_should_accept_direct(member_id) != 0U ||
            team_leader_desired_parent_for_member(member, &desired_parent_id) == 0U ||
            desired_parent_id == g_team_node.cfg.leader_id) {
            continue;
        }
        if (team_leader_failover_window_active(now_s) != 0U &&
            team_leader_failover_has_member(member_id) != 0U &&
            team_leader_relay_parent_is_ready(desired_parent_id) == 0U) {
            osal_printk("[team] direct cap defer member=%u conn=%u parent=%u reason=failover-parent-not-ready\r\n",
                member_id, conn_ids[i], desired_parent_id);
            continue;
        }
        route_matches = team_leader_route_parent_matches(member, team_route_entry_find(member_id), desired_parent_id);
        if (route_matches == 0U) {
            int parent_cfg_ret = sle_team_node_send_config(&g_team_node, desired_parent_id);

            hint_ret = sle_team_node_send_route_update(&g_team_node, member_id, desired_parent_id,
                (uint8_t)SLE_TEAM_PARENT_RESELECTING, desired_parent_id);
            g_team_rt.route_metrics_last_s = 0U;
            osal_printk("[team] direct cap migrate member=%u conn=%u parent=%u cap=%u hint_ret=%d parent_cfg_ret=%d\r\n",
                member_id, conn_ids[i], desired_parent_id, team_leader_direct_capacity(), hint_ret, parent_cfg_ret);
            allow_prune = hint_ret == SLE_TEAM_OK ? 1U : 0U;
        }
        if (allow_prune == 0U) {
            osal_printk("[team] direct cap prune deferred member=%u conn=%u parent=%u reason=hint-failed ret=%d\r\n",
                member_id, conn_ids[i], desired_parent_id, hint_ret);
            continue;
        }
        team_leader_direct_prune_mark(conn_ids[i], member_id);
        if (sle_uart_client_disconnect_conn(conn_ids[i]) != 0U) {
            pruned++;
            team_route_clear_by_conn(conn_ids[i]);
            g_team_rt.route_metrics_last_s = 0U;
            osal_printk("[team] direct cap prune confirmed member=%u conn=%u parent=%u cap=%u\r\n",
                member_id, conn_ids[i], desired_parent_id, team_leader_direct_capacity());
        } else {
            (void)team_leader_direct_prune_consume(conn_ids[i], member_id);
            osal_printk("[team] direct cap prune failed member=%u conn=%u parent=%u\r\n",
                member_id, conn_ids[i], desired_parent_id);
        }
    }
    if (pruned != 0U) {
        sle_uart_client_force_rescan();
    }
}

static uint8_t team_route_hint_cache_acquire(uint32_t now_s, uint8_t member_id, uint8_t *out_index)
{
    uint8_t i;
    uint8_t free_index = SLE_TEAM_MAX_MEMBERS;
    uint8_t oldest_index = 0U;
    uint8_t oldest_set = 0U;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID || out_index == NULL) {
        return 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (g_team_rt.route_hint_member_ids[i] == member_id) {
            *out_index = i;
            return 1U;
        }
        if (g_team_rt.route_hint_member_ids[i] == 0U && free_index == SLE_TEAM_MAX_MEMBERS) {
            free_index = i;
        }
        if (oldest_set == 0U ||
            team_elapsed_s(now_s, g_team_rt.route_hint_last_sent_s[i]) >
                team_elapsed_s(now_s, g_team_rt.route_hint_last_sent_s[oldest_index])) {
            oldest_index = i;
            oldest_set = 1U;
        }
    }
    if (free_index != SLE_TEAM_MAX_MEMBERS) {
        *out_index = free_index;
        return 1U;
    }
    *out_index = oldest_index;
    return 1U;
}

static uint8_t team_route_hint_should_send(uint32_t now_s, uint8_t member_id, uint8_t parent_id, uint8_t *out_index)
{
    uint8_t index;

    if (team_route_hint_cache_acquire(now_s, member_id, &index) == 0U) {
        return 0U;
    }
    if (g_team_rt.route_hint_member_ids[index] == member_id &&
        g_team_rt.route_hint_parent_ids[index] == parent_id &&
        team_interval_not_reached(now_s, g_team_rt.route_hint_last_sent_s[index], SLE_TEAM_ROUTE_HINT_COOLDOWN_S) !=
            0U) {
        return 0U;
    }
    if (out_index != NULL) {
        *out_index = index;
    }
    return 1U;
}

static void team_route_hint_mark_sent(uint8_t index, uint8_t member_id, uint8_t parent_id, uint32_t now_s)
{
    if (index >= SLE_TEAM_MAX_MEMBERS || member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    g_team_rt.route_hint_member_ids[index] = member_id;
    g_team_rt.route_hint_parent_ids[index] = parent_id;
    g_team_rt.route_hint_last_sent_s[index] = now_s;
}

static void team_route_hint_note_skip(uint32_t now_s)
{
    g_team_rt.route_hint_cooldown_skipped_total++;
    unused(now_s);
}

static void team_route_hint_note_send_result(uint32_t now_s, uint8_t success)
{
    if (success != 0U) {
        g_team_rt.route_hint_sent_total++;
    } else {
        g_team_rt.route_hint_failed_total++;
    }
    unused(now_s);
}

static void team_route_update_observe(const sle_team_app_packet_t *app_packet)
{
    sle_team_route_update_body_t route_update;
    uint8_t previous_next_hop;
    uint8_t observed_next_hop;
    uint32_t now_s;

    if (app_packet == NULL || g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER ||
        app_packet->app_msg_type != SLE_TEAM_APP_ROUTE_UPDATE ||
        app_packet->body_len < sizeof(sle_team_route_update_body_t)) {
        return;
    }
    (void)memcpy(&route_update, app_packet->body, sizeof(route_update));
    previous_next_hop = 0U;
    {
        const team_route_entry_t *existing = team_route_entry_find(app_packet->src_id);
        if (existing != NULL) {
            previous_next_hop = existing->next_hop_id;
        }
    }
    observed_next_hop = route_update.next_hop_id != 0U ? route_update.next_hop_id : route_update.parent_id;
    g_team_rt.route_update_rx_total++;
    if (observed_next_hop != 0U && observed_next_hop != SLE_TEAM_BROADCAST_ID &&
        previous_next_hop != 0U && previous_next_hop != observed_next_hop) {
        now_s = team_now_s(NULL);
        g_team_rt.route_reparent_total++;
        g_team_rt.route_reparent_last_s = now_s;
    }
}

static void team_leader_route_convergence_hint(uint32_t now_s, uint8_t trigger_state_change,
    uint8_t stale_count, uint8_t unreachable_count)
{
    uint8_t i;
    uint8_t sent = 0U;
    uint8_t failed = 0U;
    char summary[SLE_TEAM_WEB_EVENT_SUMMARY_SIZE];

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return;
    }
    if (trigger_state_change == 0U && stale_count == 0U && unreachable_count == 0U) {
        return;
    }

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        sle_team_member_record_t *member = &g_team_node.members[i];
        uint8_t hint_parent_id = 0U;
        uint8_t hint_cache_index = 0U;
        uint16_t hint_conn_id = 0U;
        int ret;

        if (team_leader_route_hint_parent_for_member(member, &hint_parent_id) == 0U) {
            continue;
        }
        if (team_route_find(member->member_id, TEAM_CONN_DIR_DOWNSTREAM, &hint_conn_id) == 0U) {
            team_route_hint_note_skip(now_s);
            osal_printk("[team] route hint skip member=%u parent=%u reason=no-route\r\n",
                member->member_id, hint_parent_id);
            continue;
        }
        if (team_route_hint_should_send(now_s, member->member_id, hint_parent_id, &hint_cache_index) == 0U) {
            team_route_hint_note_skip(now_s);
            continue;
        }
        ret = sle_team_node_send_route_update(&g_team_node, member->member_id, hint_parent_id,
            (uint8_t)SLE_TEAM_PARENT_CONNECTED, hint_parent_id);
        if (ret == SLE_TEAM_OK) {
            sent++;
            team_route_hint_mark_sent(hint_cache_index, member->member_id, hint_parent_id, now_s);
            team_route_hint_note_send_result(now_s, 1U);
        } else {
            failed++;
            team_route_hint_mark_sent(hint_cache_index, member->member_id, hint_parent_id, now_s);
            team_route_hint_note_send_result(now_s, 0U);
            osal_printk("[team] route hint member=%u parent=%u ret=%d\r\n", member->member_id, hint_parent_id, ret);
        }
    }

    if (sent == 0U && failed == 0U) {
        return;
    }
    (void)snprintf(summary, sizeof(summary), "route hint sent=%u fail=%u st=%u un=%u",
        sent, failed, stale_count, unreachable_count);
    sle_team_web_event_push(&g_team_events, now_s, SLE_TEAM_WEB_EVENT_SYSTEM, SLE_TEAM_APP_ROUTE_UPDATE,
        g_team_node.cfg.leader_id, SLE_TEAM_BROADCAST_ID, (uint16_t)(g_team_rt.route_metrics_epoch & 0xFFFFU), summary);
    osal_printk("[team] route hint sent=%u fail=%u stale=%u unreachable=%u\r\n",
        sent, failed, stale_count, unreachable_count);
}

static void team_leader_route_metrics_update(void)
{
    uint32_t now_s;
    uint16_t timeout_s;
    uint8_t i;
    uint8_t active = 0U;
    uint8_t direct = 0U;
    uint8_t relayed = 0U;
    uint8_t stale = 0U;
    uint8_t unreachable = 0U;
    uint8_t plan_mismatch = 0U;
    uint8_t trigger_state_change = 0U;
    uint8_t converged;
    uint8_t changed;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U || g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return;
    }
    now_s = team_now_s(NULL);
    if (team_interval_not_reached(now_s, g_team_rt.route_metrics_last_s, SLE_TEAM_ROUTE_METRICS_INTERVAL_S) != 0U) {
        return;
    }
    g_team_rt.route_metrics_last_s = now_s;
    g_team_rt.relay_budget_count = team_leader_relay_budget();
    team_leader_reconcile_online_routes(now_s);
    timeout_s = g_team_node.cfg.heartbeat_timeout_s != 0U ? g_team_node.cfg.heartbeat_timeout_s :
        CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S;

    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        const team_route_entry_t *entry = &g_team_routes[i];
        const sle_team_member_record_t *member;
        uint8_t desired_parent_id = 0U;

        if (entry->active == 0U || entry->member_id == 0U || entry->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        active++;
        if (team_route_next_hop_is_direct_peer(entry->member_id, entry->next_hop_id) == 0U) {
            relayed++;
        } else {
            direct++;
        }
        if (team_route_entry_is_stale(entry, now_s, timeout_s) != 0U) {
            stale++;
        }
        if (entry->dir == TEAM_CONN_DIR_UNKNOWN || team_route_conn_is_active(entry->dir, entry->conn_id) == 0U) {
            unreachable++;
        }
        member = team_display_find_member_record(entry->member_id);
        if (team_leader_desired_parent_for_member(member, &desired_parent_id) != 0U &&
            team_leader_route_parent_matches(member, entry, desired_parent_id) == 0U) {
            plan_mismatch++;
        }
    }

    converged = (uint8_t)((active != 0U || g_team_rt.relay_online_count == 0U) &&
        unreachable == 0U && stale == 0U && plan_mismatch == 0U);
    changed = (uint8_t)(active != g_team_rt.route_metrics_active ||
        direct != g_team_rt.route_metrics_direct ||
        relayed != g_team_rt.route_metrics_relayed ||
        stale != g_team_rt.route_metrics_stale ||
        unreachable != g_team_rt.route_metrics_unreachable ||
        converged != g_team_rt.route_metrics_converged);
    trigger_state_change = (uint8_t)(converged != g_team_rt.route_metrics_converged || plan_mismatch != 0U);
    if (changed != 0U) {
        char summary[SLE_TEAM_WEB_EVENT_SUMMARY_SIZE];

        g_team_rt.route_metrics_epoch++;
        g_team_rt.route_metrics_last_change_s = now_s;
        (void)snprintf(summary, sizeof(summary), "route conv=%u a=%u r=%u u=%u s=%u e=%lu",
            converged, active, relayed, unreachable, stale, (unsigned long)g_team_rt.route_metrics_epoch);
        sle_team_web_event_push(&g_team_events, now_s, SLE_TEAM_WEB_EVENT_SYSTEM, SLE_TEAM_APP_ROUTE_UPDATE,
            g_team_node.cfg.leader_id, g_team_node.cfg.leader_id,
            (uint16_t)(g_team_rt.route_metrics_epoch & 0xFFFFU), summary);
    }
    if (changed != 0U || plan_mismatch != 0U) {
        team_leader_route_convergence_hint(now_s, trigger_state_change, stale, unreachable);
    }
    if (converged != 0U && g_team_rt.route_metrics_converged == 0U) {
        g_team_rt.route_metrics_last_converged_s = now_s;
    }
    g_team_rt.route_metrics_active = active;
    g_team_rt.route_metrics_direct = direct;
    g_team_rt.route_metrics_relayed = relayed;
    g_team_rt.route_metrics_stale = stale;
    g_team_rt.route_metrics_unreachable = unreachable;
    g_team_rt.route_metrics_converged = converged;
    if (changed != 0U || plan_mismatch != 0U) {
        osal_printk("[team] route metrics active=%u direct=%u relayed=%u stale=%u unreachable=%u plan=%u converged=%u epoch=%lu\r\n",
            active, direct, relayed, stale, unreachable, plan_mismatch, converged,
            (unsigned long)g_team_rt.route_metrics_epoch);
    }
}

static team_conn_track_t *team_conn_track_find(uint16_t conn_id)
{
    uint8_t i;

    for (i = 0U; i < TEAM_CONN_TRACK_MAX; i++) {
        if (g_team_conn_tracks[i].active != 0U && g_team_conn_tracks[i].conn_id == conn_id) {
            return &g_team_conn_tracks[i];
        }
    }
    return NULL;
}

static team_conn_track_t *team_conn_track_alloc(uint16_t conn_id)
{
    uint8_t i;
    team_conn_track_t *track = team_conn_track_find(conn_id);

    if (track != NULL) {
        return track;
    }
    for (i = 0U; i < TEAM_CONN_TRACK_MAX; i++) {
        if (g_team_conn_tracks[i].active == 0U) {
            (void)memset_s(&g_team_conn_tracks[i], sizeof(g_team_conn_tracks[i]), 0, sizeof(g_team_conn_tracks[i]));
            g_team_conn_tracks[i].active = 1U;
            g_team_conn_tracks[i].conn_id = conn_id;
            g_team_conn_tracks[i].dir = TEAM_CONN_DIR_UNKNOWN;
            return &g_team_conn_tracks[i];
        }
    }
    return NULL;
}

static void team_conn_track_clear(uint16_t conn_id)
{
    team_conn_track_t *track = team_conn_track_find(conn_id);

    if (track != NULL) {
        (void)memset_s(track, sizeof(*track), 0, sizeof(*track));
    }
}

static team_conn_track_t *team_conn_track_find_by_route_id(uint8_t route_id)
{
    uint8_t i;

    if (route_id == 0U || route_id == SLE_TEAM_BROADCAST_ID) {
        return NULL;
    }
    for (i = 0U; i < TEAM_CONN_TRACK_MAX; i++) {
        if (g_team_conn_tracks[i].active != 0U && g_team_conn_tracks[i].route_id == route_id &&
            g_team_conn_tracks[i].route_id_provisional == 0U) {
            return &g_team_conn_tracks[i];
        }
    }
    return NULL;
}

static void team_conn_track_update(uint16_t conn_id, team_conn_dir_t dir, const sle_addr_t *addr)
{
    team_conn_track_t *track = team_conn_track_alloc(conn_id);
    team_pending_conn_t *pending;
    uint8_t route_id;

    if (track == NULL) {
        return;
    }
    track->dir = dir;
    if (addr != NULL) {
        pending = team_pending_conn_find(addr);
        if (pending != NULL) {
            track->route_id = pending->route_id;
            track->route_id_provisional = pending->route_id_provisional;
            team_pending_conn_clear(addr);
        } else {
            route_id = team_route_id_from_sle_addr(addr->addr);
            if (track->route_id == 0U || track->route_id == SLE_TEAM_BROADCAST_ID) {
                track->route_id = route_id;
                track->route_id_provisional = 1U;
            }
        }
    }
}

static void team_conn_track_note_packet(uint16_t conn_id, team_conn_dir_t dir, uint8_t route_id)
{
    team_conn_track_t *track = team_conn_track_alloc(conn_id);
    uint8_t route_bucket;

    if (track == NULL) {
        return;
    }
    if (route_id == 0U || route_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    track->dir = dir;
    if (track->route_id_provisional != 0U) {
        if (dir == TEAM_CONN_DIR_UPSTREAM && g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER &&
            route_id == g_team_node.cfg.leader_id &&
            team_member_has_reselect_target(g_team_node.upstream_parent_id) != 0U) {
            track->route_id = g_team_node.upstream_parent_id;
            track->route_id_provisional = 0U;
            return;
        }
        track->route_id = route_id;
        track->route_id_provisional = 0U;
        return;
    }
    route_bucket = team_route_bucket_from_ids(route_id, g_team_node.cfg.leader_id);
    if (dir == TEAM_CONN_DIR_UPSTREAM && g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER &&
        track->route_id != 0U && track->route_id != g_team_node.cfg.leader_id) {
        uint8_t current_bucket = team_route_bucket_from_ids(track->route_id, g_team_node.cfg.leader_id);

        /* Preserve the real first-hop relay when a leader-origin packet is forwarded across the same link. */
        if (route_bucket == 0U && current_bucket > 0U) {
            return;
        }
    }
    track->route_id = route_id;
    track->route_id_provisional = 0U;
}

static team_conn_dir_t team_conn_guess_direction_from_addr(const sle_addr_t *addr)
{
    team_pending_conn_t *pending;

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        return TEAM_CONN_DIR_DOWNSTREAM;
    }
    if (addr == NULL) {
        return TEAM_CONN_DIR_UNKNOWN;
    }
    pending = team_pending_conn_find(addr);
    if (pending != NULL) {
        return TEAM_CONN_DIR_DOWNSTREAM;
    }
    if (sle_uart_client_is_pending_remote_addr(addr) != 0U) {
        return TEAM_CONN_DIR_DOWNSTREAM;
    }
    return TEAM_CONN_DIR_UNKNOWN;
}

static team_conn_dir_t team_conn_direction_for_event(uint16_t conn_id, const sle_addr_t *addr)
{
    team_conn_track_t *track = team_conn_track_find(conn_id);
    team_conn_dir_t dir = TEAM_CONN_DIR_UNKNOWN;

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        return TEAM_CONN_DIR_DOWNSTREAM;
    }
    if (track != NULL && track->dir != TEAM_CONN_DIR_UNKNOWN) {
        return track->dir;
    }
    dir = team_conn_guess_direction_from_addr(addr);
    if (dir != TEAM_CONN_DIR_UNKNOWN) {
        return dir;
    }
    return sle_uart_client_has_conn(conn_id) != 0U ? TEAM_CONN_DIR_DOWNSTREAM : TEAM_CONN_DIR_UPSTREAM;
}

static uint8_t team_conn_should_use_client(uint16_t conn_id, const sle_addr_t *addr, sle_acb_state_t conn_state)
{
    team_conn_dir_t dir;
    team_conn_track_t *track = team_conn_track_find(conn_id);

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        return 1U;
    }
    if (conn_state == SLE_ACB_STATE_DISCONNECTED && track != NULL) {
        if (track->dir != TEAM_CONN_DIR_UNKNOWN) {
            return track->dir == TEAM_CONN_DIR_DOWNSTREAM ? 1U : 0U;
        }
        dir = team_conn_direction_for_event(conn_id, addr);
        return dir == TEAM_CONN_DIR_DOWNSTREAM ? 1U : 0U;
    }
    dir = team_conn_direction_for_event(conn_id, addr);
    return dir == TEAM_CONN_DIR_DOWNSTREAM ? 1U : 0U;
}

static void team_connection_state_changed_cbk(uint16_t conn_id, const sle_addr_t *addr,
    sle_acb_state_t conn_state, sle_pair_state_t pair_state, sle_disc_reason_t disc_reason)
{
    team_conn_dir_t dir = team_conn_should_use_client(conn_id, addr, conn_state) != 0U ?
        TEAM_CONN_DIR_DOWNSTREAM : TEAM_CONN_DIR_UPSTREAM;
    team_conn_track_t *track = team_conn_track_find(conn_id);
    uint8_t disconnected_member_id = 0U;
    uint8_t disconnected_member_known = 0U;
    uint8_t tracked_member_id = 0U;
    uint8_t routed_member_id = 0U;

    if (conn_state == SLE_ACB_STATE_DISCONNECTED && g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        if (track != NULL && track->route_id != 0U && track->route_id != SLE_TEAM_BROADCAST_ID) {
            tracked_member_id = track->route_id;
        }
        (void)team_route_member_by_conn(conn_id, dir, &routed_member_id);
    }

    team_conn_track_update(conn_id, dir, addr);
    track = team_conn_track_find(conn_id);
    if (conn_state == SLE_ACB_STATE_DISCONNECTED) {
        if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
            uint8_t planned_direct_prune = 0U;

            if (dir == TEAM_CONN_DIR_DOWNSTREAM &&
                sle_uart_client_get_conn_member(conn_id, &disconnected_member_id) != 0U &&
                disconnected_member_id != 0U && disconnected_member_id != SLE_TEAM_BROADCAST_ID) {
                disconnected_member_known = 1U;
            } else if (dir == TEAM_CONN_DIR_UPSTREAM &&
                sle_uart_server_get_conn_member(conn_id, &disconnected_member_id) != 0U &&
                disconnected_member_id != 0U && disconnected_member_id != SLE_TEAM_BROADCAST_ID) {
                disconnected_member_known = 1U;
            }
            if (disconnected_member_known == 0U && tracked_member_id != 0U &&
                tracked_member_id != SLE_TEAM_BROADCAST_ID) {
                disconnected_member_id = tracked_member_id;
                disconnected_member_known = 1U;
            }
            if (disconnected_member_known == 0U && routed_member_id != 0U &&
                routed_member_id != SLE_TEAM_BROADCAST_ID) {
                disconnected_member_id = routed_member_id;
                disconnected_member_known = 1U;
            }
            if (disconnected_member_known == 0U && track != NULL && track->route_id != 0U &&
                track->route_id != SLE_TEAM_BROADCAST_ID) {
                disconnected_member_id = track->route_id;
                disconnected_member_known = 1U;
            }
            planned_direct_prune = team_leader_direct_prune_consume(conn_id, disconnected_member_id);
            osal_printk("[team] disconnect lookup conn=%u dir=%u known=%u member=%u tracked=%u routed=%u\r\n",
                conn_id, (uint8_t)dir, disconnected_member_known, disconnected_member_id,
                tracked_member_id, routed_member_id);
            if (planned_direct_prune != 0U) {
                osal_printk("[team] direct cap prune disconnect member=%u conn=%u no-offline\r\n",
                    disconnected_member_id, conn_id);
                sle_uart_client_handle_connect_state_changed(conn_id, addr, conn_state, pair_state, disc_reason);
                team_route_clear_by_conn(conn_id);
                team_conn_track_clear(conn_id);
                return;
            }
        }
        if (dir == TEAM_CONN_DIR_DOWNSTREAM) {
            sle_uart_client_handle_connect_state_changed(conn_id, addr, conn_state, pair_state, disc_reason);
        } else {
            sle_uart_server_handle_connect_state_changed(conn_id, addr, conn_state, pair_state, disc_reason);
            if (g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER) {
                uint8_t disconnect_parent = 0U;
                if (track != NULL && track->route_id != 0U && track->route_id == g_team_node.upstream_parent_id) {
                    disconnect_parent = 1U;
                }
                if (disconnect_parent == 0U && g_team_node.upstream_parent_id == 0U) {
                    disconnect_parent = 1U;
                }
                if (disconnect_parent != 0U && g_team_node.joined != 0U) {
                    int switch_ret = sle_team_node_try_parent_switch(&g_team_node);
                    if (switch_ret != SLE_TEAM_OK) {
                        int recover_ret = sle_team_node_member_link_lost(&g_team_node);
                        osal_printk("[team] upstream link lost switch_ret=%d recover_ret=%d\r\n",
                            switch_ret, recover_ret);
                    }
                    team_upstream_parent_reset("disconnect");
                    if (g_team_rt.relay_client_started != 0U) {
                        sle_uart_client_force_rescan();
                    }
                }
            }
        }
        if (disconnected_member_known != 0U &&
            team_leader_mark_member_offline(disconnected_member_id, "conn_disconnected") != 0U &&
            dir == TEAM_CONN_DIR_DOWNSTREAM) {
            sle_uart_client_force_rescan();
        }
        team_route_clear_by_conn(conn_id);
        team_conn_track_clear(conn_id);
        return;
    }
    if (dir == TEAM_CONN_DIR_DOWNSTREAM) {
        sle_uart_client_handle_connect_state_changed(conn_id, addr, conn_state, pair_state, disc_reason);
    } else {
        sle_uart_server_handle_connect_state_changed(conn_id, addr, conn_state, pair_state, disc_reason);
        if (g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER) {
            track = team_conn_track_find(conn_id);
            if (track != NULL && track->route_id != 0U) {
                team_upstream_parent_note(track->route_id, SLE_TEAM_PARENT_CONNECTED, "connect");
            }
        }
    }
}

static void team_read_rssi_cbk(uint16_t conn_id, int8_t rssi, errcode_t status)
{
    team_conn_dir_t dir;

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        sle_uart_client_handle_read_rssi(conn_id, rssi, status);
        return;
    }
    dir = team_conn_direction_for_event(conn_id, NULL);
    if (dir == TEAM_CONN_DIR_DOWNSTREAM) {
        sle_uart_client_handle_read_rssi(conn_id, rssi, status);
    } else {
        sle_uart_server_handle_read_rssi(conn_id, rssi, status);
    }
}

static void team_pair_complete_cbk(uint16_t conn_id, const sle_addr_t *addr, errcode_t status)
{
    team_conn_dir_t dir;

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        sle_uart_client_handle_pair_complete(conn_id, addr, status);
        return;
    }
    dir = team_conn_direction_for_event(conn_id, addr);
    if (dir == TEAM_CONN_DIR_DOWNSTREAM) {
        sle_uart_client_handle_pair_complete(conn_id, addr, status);
    } else {
        sle_uart_server_handle_pair_complete(conn_id, addr, status);
    }
}

static void team_register_connection_callbacks(void)
{
    errcode_t ret;

    (void)memset_s(&g_team_conn_cbks, sizeof(g_team_conn_cbks), 0, sizeof(g_team_conn_cbks));
    g_team_conn_cbks.connect_state_changed_cb = team_connection_state_changed_cbk;
    g_team_conn_cbks.pair_complete_cb = team_pair_complete_cbk;
    g_team_conn_cbks.read_rssi_cb = team_read_rssi_cbk;
    ret = sle_connection_register_callbacks(&g_team_conn_cbks);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[team] connection callbacks register failed ret=0x%x\r\n", ret);
    }
}

static uint8_t team_buffer_contains(const uint8_t *buf, size_t buf_len, const char *needle, size_t needle_len)
{
    size_t i;

    if (buf == NULL || needle == NULL || needle_len == 0U || buf_len < needle_len) {
        return 0U;
    }
    for (i = 0U; i + needle_len <= buf_len; i++) {
        if (memcmp(buf + i, needle, needle_len) == 0) {
            return 1U;
        }
    }
    return 0U;
}

static void team_refresh_relay_mode(void)
{
    if (g_team_rt.role_configured == 0U) {
        return;
    }
    if (g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
        g_team_node.cfg.relay_enabled = 0U;
        return;
    }
    g_team_node.cfg.relay_enabled = (g_team_node.joined != 0U && g_team_node.cfg.relay_allowed != 0U) ? 1U : 0U;
}

static void team_upstream_parent_note(uint8_t parent_id, sle_team_parent_state_t state, const char *reason)
{
    int8_t parent_rssi = SLE_TEAM_RSSI_UNKNOWN;
    team_conn_track_t *track;
    uint32_t now_s;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER || parent_id == 0U || parent_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    now_s = team_now_s(NULL);
    if (g_team_node.upstream_parent_id == parent_id && g_team_node.upstream_parent_state == state &&
        g_team_node.upstream_parent_reselect_pending == 0U) {
        g_team_node.last_parent_seen_s = now_s;
        return;
    }
    g_team_node.upstream_parent_id = parent_id;
    g_team_node.upstream_parent_state = state;
    g_team_node.upstream_parent_reselect_pending = 0U;
    g_team_node.last_parent_seen_s = now_s;
    track = team_conn_track_find_by_route_id(parent_id);
    if (track != NULL) {
        if (track->dir == TEAM_CONN_DIR_UPSTREAM) {
            (void)sle_uart_server_get_conn_rssi(track->conn_id, &parent_rssi);
        } else if (track->dir == TEAM_CONN_DIR_DOWNSTREAM) {
            /* client side currently exposes latest rssi; keep it as fallback */
            parent_rssi = sle_uart_client_get_last_rssi();
        }
    }
    g_team_rt.parent_selected_rssi = parent_rssi;
    g_team_rt.parent_switch_last_s = now_s;
    osal_printk("[team] upstream parent=%u state=%u reason=%s\r\n",
        parent_id, (uint8_t)state, reason != NULL ? reason : "unknown");
}

static void team_upstream_parent_reset(const char *reason)
{
    if (g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
        return;
    }
    if (g_team_node.upstream_parent_state == SLE_TEAM_PARENT_RESELECTING &&
        g_team_node.upstream_parent_reselect_pending != 0U) {
        return;
    }
    g_team_node.upstream_parent_state = SLE_TEAM_PARENT_RESELECTING;
    g_team_node.upstream_parent_reselect_pending = 1U;
    g_team_rt.parent_selected_rssi = SLE_TEAM_RSSI_UNKNOWN;
    g_team_rt.parent_switch_last_s = 0U;
    osal_printk("[team] upstream parent reselect parent=%u reason=%s\r\n",
        g_team_node.upstream_parent_id, reason != NULL ? reason : "unknown");
}

static void team_member_upstream_recover_clear(const char *reason)
{
    if (g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
        return;
    }
    if (g_team_rt.member_upstream_not_ready_since_s == 0U &&
        g_team_rt.member_upstream_not_ready_count == 0U) {
        return;
    }
    osal_printk("[team] member upstream ready reason=%s fails=%u\r\n",
        reason != NULL ? reason : "ready", g_team_rt.member_upstream_not_ready_count);
    g_team_rt.member_upstream_recover_last_s = 0U;
    g_team_rt.member_upstream_not_ready_since_s = 0U;
    g_team_rt.member_upstream_not_ready_count = 0U;
}

static void team_member_upstream_prepare_retry(const char *reason)
{
    if (g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER || g_team_node.state == SLE_TEAM_NET_IDLE) {
        return;
    }
    if (g_team_node.joined == 0U) {
        g_team_node.state = SLE_TEAM_NET_DISCOVERING;
        g_team_node.last_hello_s = 0U;
    }
    if (g_team_node.upstream_parent_id != 0U &&
        g_team_node.upstream_parent_id != SLE_TEAM_BROADCAST_ID) {
        team_upstream_parent_reset(reason);
    } else {
        g_team_node.upstream_parent_state = SLE_TEAM_PARENT_DISCOVERING;
        g_team_node.upstream_parent_reselect_pending = 0U;
        g_team_rt.parent_selected_rssi = SLE_TEAM_RSSI_UNKNOWN;
        g_team_rt.parent_switch_last_s = 0U;
    }
}

static void team_member_upstream_recover_after_tx_fail(const char *reason, uint8_t dst_id, int ret)
{
    uint32_t now_s;
    uint32_t fail_age_s;
    errcode_t disconnect_ret = ERRCODE_SUCC;
    errcode_t adv_ret;
    uint8_t stuck;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
        return;
    }
    now_s = team_now_s(NULL);
    if (g_team_rt.member_upstream_not_ready_since_s == 0U) {
        g_team_rt.member_upstream_not_ready_since_s = now_s;
    }
    if (g_team_rt.member_upstream_not_ready_count < 0xFFFFU) {
        g_team_rt.member_upstream_not_ready_count++;
    }
    if (team_interval_not_reached(now_s, g_team_rt.member_upstream_recover_last_s,
            SLE_TEAM_MEMBER_UPSTREAM_RECOVER_INTERVAL_S) != 0U) {
        return;
    }
    g_team_rt.member_upstream_recover_last_s = now_s;
    fail_age_s = team_elapsed_s(now_s, g_team_rt.member_upstream_not_ready_since_s);
    stuck = (uint8_t)(fail_age_s >= SLE_TEAM_MEMBER_UPSTREAM_STUCK_S);
    if (stuck != 0U) {
        team_member_upstream_prepare_retry("transport-stuck");
        disconnect_ret = sle_uart_server_disconnect_current();
    }
    adv_ret = sle_uart_server_adv_restart();
    osal_printk("[team] member upstream recover reason=%s dst=%u ret=%d fails=%u age=%u stuck=%u disc=0x%x adv=0x%x joined=%u parent=%u state=%u\r\n",
        reason != NULL ? reason : "tx-fail", dst_id, ret, g_team_rt.member_upstream_not_ready_count,
        fail_age_s, stuck, disconnect_ret, adv_ret, g_team_node.joined, g_team_node.upstream_parent_id,
        (uint8_t)g_team_node.upstream_parent_state);
}

static void team_member_upstream_recover_tick(void)
{
    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER ||
        g_team_node.cfg.leader_id == 0U || g_team_node.cfg.leader_id == SLE_TEAM_BROADCAST_ID ||
        g_team_node.state == SLE_TEAM_NET_IDLE) {
        return;
    }
    if (sle_uart_server_connected_count() != 0U) {
        team_member_upstream_recover_clear("conn");
        return;
    }
    team_member_upstream_recover_after_tx_fail("no-upstream", g_team_node.cfg.leader_id,
        SLE_TEAM_ERR_UNSUPPORTED);
}

static void team_member_parent_reselect_disconnect_tick(void)
{
    uint16_t conn_id = 0U;
    uint32_t now_s;
    errcode_t ret;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        team_member_has_reselect_target(g_team_node.upstream_parent_id) == 0U) {
        return;
    }
    now_s = team_now_s(NULL);
    if (team_interval_not_reached(now_s, g_team_rt.parent_reselect_disconnect_last_s,
            SLE_TEAM_PARENT_RESELECT_DISCONNECT_DELAY_S) != 0U) {
        return;
    }
    g_team_rt.parent_reselect_disconnect_last_s = now_s;
    if (sle_uart_server_connected_count() == 0U) {
        (void)sle_uart_server_adv_restart();
        osal_printk("[team] parent reselect waiting target=%u reason=no-upstream\r\n",
            g_team_node.upstream_parent_id);
        return;
    }
    if (sle_uart_server_find_conn_by_member_ex(g_team_node.cfg.leader_id, &conn_id) != 0U) {
        ret = sle_uart_server_disconnect_conn(conn_id);
        osal_printk("[team] parent reselect drop old leader conn=%u target=%u ret=0x%x\r\n",
            conn_id, g_team_node.upstream_parent_id, ret);
        (void)sle_uart_server_adv_restart();
        return;
    }
    ret = sle_uart_server_disconnect_current();
    osal_printk("[team] parent reselect drop upstream target=%u ret=0x%x\r\n",
        g_team_node.upstream_parent_id, ret);
    (void)sle_uart_server_adv_restart();
}

static void team_member_autoselect_parent(void)
{
    uint16_t conn_ids[SLE_TEAM_MAX_DIRECT_CONNECTIONS];
    uint8_t conn_count;
    uint8_t i;
    uint8_t best_parent_id = 0U;
    int8_t best_rssi = SLE_TEAM_RSSI_UNKNOWN;
    uint32_t now_s;
    uint8_t current_parent_id;
    int8_t current_parent_rssi = SLE_TEAM_RSSI_UNKNOWN;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
        return;
    }
    /* V2 goal: both relay and leaf members can run parent RSSI autoselect. */

    conn_count = sle_uart_server_get_active_conns(conn_ids, (uint8_t)SLE_TEAM_MAX_DIRECT_CONNECTIONS);
    if (conn_count == 0U) {
        return;
    }

    for (i = 0U; i < conn_count; i++) {
        team_conn_track_t *track = team_conn_track_find(conn_ids[i]);
        int8_t rssi = SLE_TEAM_RSSI_UNKNOWN;

        if (track == NULL || track->route_id == 0U || track->route_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        if (track->route_id == g_team_node.cfg.self_id) {
            continue;
        }
        (void)sle_uart_server_get_conn_rssi(conn_ids[i], &rssi);
        if (rssi == SLE_TEAM_RSSI_UNKNOWN || rssi < SLE_TEAM_PARENT_CANDIDATE_MIN_RSSI) {
            continue;
        }
        if (best_parent_id == 0U || rssi > best_rssi) {
            best_parent_id = track->route_id;
            best_rssi = rssi;
        }
    }

    if (best_parent_id == 0U) {
        return;
    }

    current_parent_id = g_team_node.upstream_parent_id;
    if (current_parent_id != 0U && current_parent_id != SLE_TEAM_BROADCAST_ID) {
        team_conn_track_t *current_track = team_conn_track_find_by_route_id(current_parent_id);

        if (current_track != NULL) {
            (void)sle_uart_server_get_conn_rssi(current_track->conn_id, &current_parent_rssi);
        } else {
            current_parent_rssi = g_team_rt.parent_selected_rssi;
        }
    }

    if (current_parent_id == best_parent_id) {
        if (g_team_node.upstream_parent_state != SLE_TEAM_PARENT_CONNECTED ||
            g_team_node.upstream_parent_reselect_pending != 0U) {
            team_upstream_parent_note(best_parent_id, SLE_TEAM_PARENT_CONNECTED, "autoselect-keep");
        }
        return;
    }

    now_s = team_now_s(NULL);
    if (team_interval_not_reached(now_s, g_team_rt.parent_switch_last_s, SLE_TEAM_PARENT_SWITCH_COOLDOWN_S) != 0U) {
        return;
    }
    if (current_parent_rssi != SLE_TEAM_RSSI_UNKNOWN &&
        best_rssi < (int8_t)(current_parent_rssi + SLE_TEAM_PARENT_SWITCH_RSSI_HYST_DBM)) {
        return;
    }

    team_upstream_parent_note(best_parent_id, SLE_TEAM_PARENT_CONNECTED, "autoselect-switch");
    if (g_team_node.joined != 0U) {
        int ret = sle_team_node_send_route_update(&g_team_node, g_team_node.cfg.leader_id,
            g_team_node.cfg.leader_id, (uint8_t)SLE_TEAM_PARENT_CONNECTED, best_parent_id);
        osal_printk("[team] auto parent switch %u->%u rssi=%d ret=%d\r\n",
            current_parent_id, best_parent_id, best_rssi, ret);
    }
}

static uint8_t team_client_seek_filter(const sle_seek_result_info_t *seek_result_data, void *user_ctx)
{
    static const char relay_server_name[] = "sle_uart_server";
    uint8_t candidate_bucket;
    uint8_t candidate_id;
    uint8_t candidate_provisional = 0U;
    size_t relay_server_name_len = sizeof(relay_server_name) - 1U;

    unused(user_ctx);
    if (seek_result_data == NULL || seek_result_data->data == NULL || seek_result_data->data_length == 0U) {
        osal_printk("[team] seek filter reject reason=bad-data\r\n");
        return 0U;
    }
    if (seek_result_data->rssi == SLE_TEAM_RSSI_UNKNOWN) {
        osal_printk("[team] seek filter reject reason=unknown-rssi\r\n");
        return 0U;
    }
    if (team_buffer_contains(seek_result_data->data, seek_result_data->data_length,
            relay_server_name, relay_server_name_len) == 0U) {
        osal_printk("[team] seek filter reject reason=name\r\n");
        return 0U;
    }

    candidate_id = team_route_id_from_adv_data(seek_result_data->data, seek_result_data->data_length);
    if (candidate_id == 0U) {
        /* Legacy fallback: address-derived route ids are only provisional. */
        candidate_id = team_route_id_from_sle_addr(seek_result_data->addr.addr);
        candidate_provisional = 1U;
    }
    if (candidate_id == 0U || candidate_id == g_team_node.cfg.self_id || candidate_id == g_team_node.cfg.leader_id) {
        osal_printk("[team] seek filter reject id=%u reason=self-or-leader self=%u leader=%u\r\n",
            candidate_id, g_team_node.cfg.self_id, g_team_node.cfg.leader_id);
        return 0U;
    }

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        candidate_bucket = team_route_bucket_from_ids(candidate_id, g_team_node.cfg.leader_id);
        if (team_leader_should_seek_member(candidate_id, candidate_bucket) != 0U) {
            team_pending_conn_note(&seek_result_data->addr, candidate_id, candidate_provisional);
            return 1U;
        }
        osal_printk("[team] seek filter reject id=%u bucket=%u reason=leader-policy\r\n",
            candidate_id, candidate_bucket);
        return 0U;
    }
    if (team_member_reselection_active() != 0U &&
        team_member_has_reselect_target(g_team_node.upstream_parent_id) != 0U &&
        candidate_id != g_team_node.upstream_parent_id) {
        osal_printk("[team] seek filter reject id=%u target=%u reason=reselect-target\r\n",
            candidate_id, g_team_node.upstream_parent_id);
        return 0U;
    }
    if (team_member_relay_can_accept_child() == 0U && team_member_reselection_active() == 0U) {
        osal_printk("[team] seek filter reject id=%u reason=relay-capacity relay_allowed=%u relay_enabled=%u joined=%u down=%u\r\n",
            candidate_id, g_team_node.cfg.relay_allowed, g_team_node.cfg.relay_enabled, g_team_node.joined,
            sle_uart_client_connected_count());
        return 0U;
    }
    team_pending_conn_note(&seek_result_data->addr, candidate_id, candidate_provisional);
    return 1U;
}

static uint32_t team_now_s(void *user_ctx)
{
    unused(user_ctx);
    return (uint32_t)(uapi_tcxo_get_ms() / 1000U);
}

static int8_t team_rssi_dbm(void *user_ctx)
{
    unused(user_ctx);

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        return sle_uart_client_get_last_rssi();
    }
    return sle_uart_server_get_last_rssi();
}

static void team_request_sle_rssi(void)
{
    errcode_t ret;
    errcode_t relay_ret;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U) {
        return;
    }
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        /*
         * Keep probing RSSI as long as a client link is tracked.
         * read_remote_rssi() also drives stale-connection recovery, so gating
         * only on "ready" can leave a ghost link that never gets cleaned up.
         */
        if (sle_uart_client_connected_count() == 0U) {
            return;
        }
        ret = sle_uart_client_read_remote_rssi();
    } else {
        ret = ERRCODE_SLE_FAIL;
        if (sle_uart_server_connected_count() == 0U) {
            ret = ERRCODE_SLE_FAIL;
        } else {
            ret = sle_uart_server_read_remote_rssi();
        }
        if (team_route_is_relay_enabled() != 0U && g_team_rt.relay_client_started != 0U &&
            sle_uart_client_connected_count() != 0U) {
            relay_ret = sle_uart_client_read_remote_rssi();
            if (ret != ERRCODE_SLE_SUCCESS) {
                ret = relay_ret;
            }
        }
    }
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[team] read sle rssi ret=0x%x\r\n", ret);
    }
}

static void team_leader_rescan_if_needed(const char *reason)
{
    uint32_t now_s;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return;
    }
    now_s = team_now_s(NULL);
    if (team_interval_not_reached(now_s, g_team_rt.member_rescan_last_s, SLE_TEAM_MEMBER_RESCAN_INTERVAL_S) != 0U) {
        return;
    }
    g_team_rt.member_rescan_last_s = now_s;
    osal_printk("[team] leader force rescan reason=%s\r\n", reason != NULL ? reason : "unknown");
    sle_uart_client_force_rescan();
}

static void team_leader_pairing_restart_scan(const char *reason)
{
    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER || g_team_node.cfg.pairing_enabled == 0U) {
        return;
    }
    (void)memset_s(g_team_pending_conns, sizeof(g_team_pending_conns), 0, sizeof(g_team_pending_conns));
    g_team_rt.member_rescan_last_s = 0U;
    g_team_rt.pairing_rotate_last_s = 0U;
    g_team_rt.pairing_rotate_index = 0U;
    osal_printk("[team] pairing restart scan reason=%s\r\n", reason != NULL ? reason : "unknown");
    sle_uart_client_force_rescan();
}

static uint8_t team_leader_has_offline_members(void)
{
    uint8_t i;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];

        if (member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        if (member->online == 0U) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t team_member_is_pending_or_online(uint8_t member_id)
{
    uint8_t i;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (g_team_node.pending_members[i].active != 0U && g_team_node.pending_members[i].member_id == member_id) {
            return 1U;
        }
        if (g_team_node.members[i].online != 0U && g_team_node.members[i].member_id == member_id) {
            return 1U;
        }
    }
    return 0U;
}

static void team_leader_pairing_rotate_connections(void)
{
    uint32_t now_s;
    uint16_t conn_ids[SLE_TEAM_MAX_DIRECT_CONNECTIONS];
    uint8_t conn_count;
    uint8_t keep_count = 0U;
    uint8_t i;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER || g_team_node.cfg.pairing_enabled == 0U) {
        return;
    }
    now_s = team_now_s(NULL);
    if (team_interval_not_reached(now_s, g_team_rt.pairing_rotate_last_s, SLE_TEAM_PAIRING_ROTATE_INTERVAL_S) != 0U) {
        return;
    }
    g_team_rt.pairing_rotate_last_s = now_s;
    conn_count = sle_uart_client_get_active_conns(conn_ids, (uint8_t)SLE_TEAM_MAX_DIRECT_CONNECTIONS);
    if (conn_count <= SLE_TEAM_PAIRING_KEEP_CONNECTED) {
        return;
    }
    for (i = 0U; i < conn_count; i++) {
        uint8_t member_id = 0U;

        if (sle_uart_client_get_conn_member(conn_ids[i], &member_id) != 0U &&
            team_member_is_pending_or_online(member_id) != 0U) {
            keep_count++;
        }
    }
    for (i = 0U; i < conn_count &&
        ((int32_t)conn_count - (int32_t)keep_count) > (int32_t)SLE_TEAM_PAIRING_KEEP_CONNECTED; i++) {
        uint8_t index = (uint8_t)((g_team_rt.pairing_rotate_index + i) % conn_count);
        uint8_t member_id = 0U;

        if (sle_uart_client_get_conn_member(conn_ids[index], &member_id) != 0U &&
            team_member_is_pending_or_online(member_id) != 0U) {
            continue;
        }
        if (sle_uart_client_disconnect_conn(conn_ids[index]) != 0U) {
            osal_printk("[team] pairing rotate disconnect conn=%u\r\n", conn_ids[index]);
            g_team_rt.pairing_rotate_index = (uint8_t)((index + 1U) % conn_count);
            return;
        }
    }
}

static void team_leader_auto_approve_pending(void)
{
    uint8_t i;
    uint8_t relay_quota = team_leader_relay_budget();

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];

        if (member->online == 0U) {
            continue;
        }
        if (member->relay_allowed != 0U && relay_quota > 0U) {
            relay_quota--;
        }
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_pending_member_t *pending = &g_team_node.pending_members[i];
        uint8_t relay_allowed;
        int ret;

        if (pending->active == 0U || pending->member_id == 0U || pending->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        relay_allowed = relay_quota > 0U ? 1U : 0U;
        ret = sle_team_node_pairing_approve_with_relay(&g_team_node, pending->member_id, relay_allowed);
        osal_printk("[team] auto approve pending member=%u relay=%u ret=%d\r\n",
            pending->member_id, relay_allowed, ret);
        if (ret == SLE_TEAM_OK && relay_allowed != 0U && relay_quota > 0U) {
            relay_quota--;
        }
    }
}

static uint8_t team_leader_relay_target_for_member_count(uint8_t member_count)
{
    uint8_t direct_capacity = team_leader_direct_capacity();
    uint8_t relay_budget = team_leader_relay_budget();
    uint8_t downstream_count;
    uint8_t target;

    if (member_count == 0U || member_count <= direct_capacity || relay_budget == 0U) {
        return 0U;
    }

    downstream_count = (uint8_t)(member_count - direct_capacity);
    target = team_ceil_div_u16_to_u8(downstream_count, (uint16_t)SLE_TEAM_MAX_DIRECT_CONNECTIONS);
    if (target == 0U) {
        target = 1U;
    }
    if (direct_capacity >= SLE_TEAM_MAX_DIRECT_CONNECTIONS && target < SLE_TEAM_RELAY_DENSE_MIN &&
        relay_budget >= SLE_TEAM_RELAY_DENSE_MIN) {
        target = SLE_TEAM_RELAY_DENSE_MIN;
    }
    if (target > relay_budget) {
        target = relay_budget;
    }
    return target;
}

static uint8_t team_leader_member_id_is_counted(const uint8_t *member_ids, uint8_t count, uint8_t member_id)
{
    uint8_t i;

    if (member_ids == NULL || member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    for (i = 0U; i < count; i++) {
        if (member_ids[i] == member_id) {
            return 1U;
        }
    }
    return 0U;
}

static void team_leader_count_member_id(uint8_t *member_ids, uint8_t *count, uint8_t member_id)
{
    if (member_ids == NULL || count == NULL || *count >= SLE_TEAM_MAX_MEMBERS ||
        member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID ||
        member_id == g_team_node.cfg.self_id || member_id == g_team_node.cfg.leader_id) {
        return;
    }
    if (team_leader_member_id_is_counted(member_ids, *count, member_id) != 0U) {
        return;
    }
    member_ids[*count] = member_id;
    *count = (uint8_t)(*count + 1U);
}

static uint8_t team_leader_known_member_count(void)
{
    uint8_t member_ids[SLE_TEAM_MAX_MEMBERS];
    uint8_t count = 0U;
    uint8_t i;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        team_leader_count_member_id(member_ids, &count, g_team_node.members[i].member_id);
    }
    if (g_team_node.cfg.member_filter_enabled != 0U) {
        for (i = 0U; i < g_team_node.cfg.allowed_member_count; i++) {
            team_leader_count_member_id(member_ids, &count, g_team_node.cfg.allowed_member_ids[i]);
        }
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        team_leader_count_member_id(member_ids, &count, g_team_rt.relay_failover_member_ids[i]);
    }
    return count;
}

static uint8_t team_leader_pending_member_count(void)
{
    uint8_t i;
    uint8_t count = 0U;

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_pending_member_t *pending = &g_team_node.pending_members[i];
        const sle_team_member_record_t *member;

        if (pending->active == 0U || pending->member_id == 0U ||
            pending->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        member = team_leader_find_member_slot(pending->member_id);
        if (member != NULL) {
            continue;
        }
        count++;
    }
    return count;
}

static uint8_t team_leader_relay_target_with_failover(uint8_t member_count, uint8_t relay_count, uint32_t now_s)
{
    uint8_t target = team_leader_relay_target_for_member_count(member_count);

    /*
     * Relay loss temporarily hides downstream children from the leader. During
     * that grace window, keep the newly promoted relay alive instead of letting
     * a low demand count make the normal target calculation demote it to zero.
     */
    if (target == 0U && relay_count != 0U && team_leader_failover_window_active(now_s) != 0U) {
        target = 1U;
        osal_printk("[team] relay failover holding relay target demand=%u relay=%u target=%u lost=%u\r\n",
            member_count, relay_count, target, g_team_rt.relay_failover_lost_id);
    }
    return target;
}

static uint8_t team_leader_relay_is_candidate(const sle_team_member_record_t *member, uint32_t now_s, uint16_t timeout_s)
{
    if (member == NULL || member->online == 0U) {
        return 0U;
    }
    if (member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID ||
        member->member_id == g_team_node.cfg.self_id || member->member_id == g_team_node.cfg.leader_id) {
        return 0U;
    }
    if (member->relay_allowed != 0U) {
        return 0U;
    }
    if (team_leader_failover_window_active(now_s) != 0U &&
        team_leader_failover_has_member(member->member_id) != 0U) {
        return 1U;
    }
    if (team_elapsed_exceeds(now_s, member->last_seen_s, (uint32_t)timeout_s) != 0U) {
        return 0U;
    }
    if (member->last_rssi_dbm != SLE_TEAM_RSSI_UNKNOWN && member->last_rssi_dbm < SLE_TEAM_RELAY_CANDIDATE_MIN_RSSI) {
        return 0U;
    }
    return 1U;
}

static sle_team_member_record_t *team_leader_pick_best_relay_candidate(uint32_t now_s, uint16_t timeout_s)
{
    uint8_t i;
    sle_team_member_record_t *best = NULL;
    int8_t best_rssi = SLE_TEAM_RSSI_UNKNOWN;

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        sle_team_member_record_t *member = &g_team_node.members[i];
        int8_t member_rssi;

        if (team_leader_relay_is_candidate(member, now_s, timeout_s) == 0U) {
            continue;
        }
        /* Unknown RSSI should be treated as weakest in auto-demote selection. */
        member_rssi = member->last_rssi_dbm == SLE_TEAM_RSSI_UNKNOWN ? (int8_t)(-128) : member->last_rssi_dbm;
        if (best == NULL || member_rssi > best_rssi) {
            best = member;
            best_rssi = member_rssi;
        }
    }
    return best;
}

static sle_team_member_record_t *team_leader_pick_worst_active_relay(uint32_t now_s, uint16_t timeout_s)
{
    uint8_t i;
    sle_team_member_record_t *worst = NULL;
    int8_t worst_rssi = (int8_t)(SLE_TEAM_RSSI_UNKNOWN + 1);
    uint32_t worst_age_s = 0U;

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        sle_team_member_record_t *member = &g_team_node.members[i];
        int8_t member_rssi;
        uint32_t member_age_s;

        if (member->online == 0U || member->relay_allowed == 0U ||
            member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID ||
            member->member_id == g_team_node.cfg.self_id || member->member_id == g_team_node.cfg.leader_id) {
            continue;
        }
        if (team_elapsed_exceeds(now_s, member->last_seen_s,
                (uint32_t)timeout_s * SLE_TEAM_RELAY_REVOKE_STALE_FACTOR) != 0U) {
            continue;
        }
        if (team_leader_member_has_downstream_children(member->member_id) != 0U) {
            continue;
        }
        /* Unknown RSSI must be treated as weakest so known links are demoted first. */
        member_rssi = member->last_rssi_dbm == SLE_TEAM_RSSI_UNKNOWN ? (int8_t)(-128) : member->last_rssi_dbm;
        member_age_s = team_elapsed_s(now_s, member->last_seen_s);
        if (worst == NULL || member_rssi < worst_rssi ||
            (member_rssi == worst_rssi && member_age_s > worst_age_s) ||
            (member_rssi == worst_rssi && member_age_s == worst_age_s && member->member_id > worst->member_id)) {
            worst = member;
            worst_rssi = member_rssi;
            worst_age_s = member_age_s;
        }
    }
    return worst;
}

static void team_leader_relay_config_pending_set(uint8_t member_id, uint8_t pending)
{
    uint8_t i;
    uint8_t free_index = SLE_TEAM_MAX_MEMBERS;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (g_team_rt.relay_config_pending_member_ids[i] == member_id) {
            if (pending == 0U) {
                g_team_rt.relay_config_pending_member_ids[i] = 0U;
            }
            return;
        }
        if (g_team_rt.relay_config_pending_member_ids[i] == 0U && free_index == SLE_TEAM_MAX_MEMBERS) {
            free_index = i;
        }
    }
    if (pending != 0U && free_index != SLE_TEAM_MAX_MEMBERS) {
        g_team_rt.relay_config_pending_member_ids[free_index] = member_id;
    }
}

static int team_leader_set_member_relay_allowed(sle_team_member_record_t *member, uint8_t relay_allowed,
    const char *reason, uint8_t notify_member)
{
    uint8_t old_allowed;
    int ret;

    if (member == NULL || member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID) {
        return SLE_TEAM_ERR_ARG;
    }
    old_allowed = member->relay_allowed;
    if (old_allowed == (relay_allowed != 0U ? 1U : 0U)) {
        return SLE_TEAM_OK;
    }
    member->relay_allowed = relay_allowed != 0U ? 1U : 0U;
    if (member->relay_allowed != 0U) {
        member->relay_tier = team_route_bucket_from_ids(member->member_id, g_team_node.cfg.leader_id);
        member->max_downstream = SLE_TEAM_MAX_DIRECT_CONNECTIONS;
    } else {
        member->relay_tier = 0U;
        member->max_downstream = 0U;
    }
    if (notify_member != 0U) {
        ret = sle_team_node_send_config(&g_team_node, member->member_id);
        if (ret != SLE_TEAM_OK) {
            team_leader_relay_config_pending_set(member->member_id, 1U);
            g_team_rt.relay_config_retry_last_s = 0U;
            osal_printk("[team] relay config notify pending member=%u allow=%u reason=%s ret=%d\r\n",
                member->member_id, member->relay_allowed, reason != NULL ? reason : "unknown", ret);
        } else {
            team_leader_relay_config_pending_set(member->member_id, 0U);
        }
    } else {
        team_leader_relay_config_pending_set(member->member_id, 0U);
        ret = SLE_TEAM_OK;
    }
    if (member->relay_allowed == 0U) {
        team_route_clear_by_next_hop(member->member_id);
    }
    osal_printk("[team] relay set member=%u allow=%u notify=%u reason=%s ret=%d\r\n",
        member->member_id, member->relay_allowed, notify_member, reason != NULL ? reason : "unknown", ret);
    return SLE_TEAM_OK;
}

static void team_leader_relay_swap_reset(void)
{
    g_team_rt.relay_swap_candidate_id = 0U;
    g_team_rt.relay_swap_victim_id = 0U;
    g_team_rt.relay_swap_since_s = 0U;
}

static uint8_t team_leader_relay_swap_tick(uint32_t now_s, uint16_t timeout_s, uint8_t stable_topology)
{
    sle_team_member_record_t *candidate;
    sle_team_member_record_t *victim;
    int16_t rssi_delta;
    uint32_t stable_s;

    if (stable_topology == 0U || team_leader_failover_window_active(now_s) != 0U) {
        team_leader_relay_swap_reset();
        return 0U;
    }

    candidate = team_leader_pick_best_relay_candidate(now_s, timeout_s);
    victim = team_leader_pick_worst_active_relay(now_s, timeout_s);
    if (candidate == NULL || victim == NULL ||
        candidate->last_rssi_dbm == SLE_TEAM_RSSI_UNKNOWN ||
        victim->last_rssi_dbm == SLE_TEAM_RSSI_UNKNOWN) {
        team_leader_relay_swap_reset();
        return 0U;
    }

    rssi_delta = (int16_t)candidate->last_rssi_dbm - (int16_t)victim->last_rssi_dbm;
    if (rssi_delta < (int16_t)SLE_TEAM_RELAY_SWAP_RSSI_HYST_DBM) {
        team_leader_relay_swap_reset();
        return 0U;
    }

    if (g_team_rt.relay_swap_candidate_id != candidate->member_id ||
        g_team_rt.relay_swap_victim_id != victim->member_id ||
        g_team_rt.relay_swap_since_s == 0U) {
        g_team_rt.relay_swap_candidate_id = candidate->member_id;
        g_team_rt.relay_swap_victim_id = victim->member_id;
        g_team_rt.relay_swap_since_s = now_s;
        osal_printk("[team] relay swap observe candidate=%u victim=%u delta=%d hysteresis=%d stable_need=%u\r\n",
            candidate->member_id, victim->member_id, (int)rssi_delta, SLE_TEAM_RELAY_SWAP_RSSI_HYST_DBM,
            SLE_TEAM_RELAY_SWAP_STABLE_S);
        return 0U;
    }

    stable_s = team_elapsed_s(now_s, g_team_rt.relay_swap_since_s);
    if (stable_s < SLE_TEAM_RELAY_SWAP_STABLE_S) {
        return 0U;
    }

    if (team_leader_set_member_relay_allowed(candidate, 1U, "swap-promote", 1U) != SLE_TEAM_OK) {
        team_leader_relay_swap_reset();
        return 0U;
    }
    if (team_leader_set_member_relay_allowed(victim, 0U, "swap-demote", 1U) != SLE_TEAM_OK) {
        team_leader_relay_swap_reset();
        return 1U;
    }
    osal_printk("[team] relay swap candidate=%u victim=%u delta=%d stable=%lu hysteresis=%d\r\n",
        candidate->member_id, victim->member_id, (int)rssi_delta, (unsigned long)stable_s,
        SLE_TEAM_RELAY_SWAP_RSSI_HYST_DBM);
    team_leader_relay_swap_reset();
    return 1U;
}

static uint8_t team_leader_should_seek_member(uint8_t candidate_id, uint8_t candidate_bucket)
{
    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return 0U;
    }
    if (g_team_node.cfg.pairing_enabled != 0U) {
        return 1U;
    }
    if (team_leader_failover_should_seek_member(candidate_id) != 0U) {
        return 1U;
    }
    if (team_leader_find_member_slot(candidate_id) != NULL) {
        return team_leader_member_should_accept_direct(candidate_id);
    }
    if (g_team_node.cfg.member_filter_enabled != 0U && g_team_node.cfg.allowed_member_count != 0U &&
        sle_team_node_is_member_allowed(&g_team_node, candidate_id) != 0U) {
        return team_leader_online_relay_count() == 0U ? 1U : 0U;
    }
    if (candidate_bucket == 1U) {
        return 1U;
    }
    return 0U;
}

static uint8_t team_should_defer_member_timeout(void *user_ctx, uint8_t member_id,
    uint32_t now_s, uint32_t last_seen_s)
{
    unused(user_ctx);
    unused(last_seen_s);

    if (team_leader_failover_window_active(now_s) == 0U) {
        return 0U;
    }
    return team_leader_failover_has_member(member_id);
}

static void team_leader_relay_config_retry_tick(void)
{
    uint8_t i;
    uint32_t now_s;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        return;
    }
    now_s = team_now_s(NULL);
    if (team_interval_not_reached(now_s, g_team_rt.relay_config_retry_last_s,
            SLE_TEAM_RELAY_CONFIG_RETRY_INTERVAL_S) != 0U) {
        return;
    }
    g_team_rt.relay_config_retry_last_s = now_s;
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        uint8_t member_id = g_team_rt.relay_config_pending_member_ids[i];
        sle_team_member_record_t *member;
        int ret;

        if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        member = team_leader_find_member_slot(member_id);
        if (member == NULL || member->online == 0U) {
            g_team_rt.relay_config_pending_member_ids[i] = 0U;
            continue;
        }
        ret = sle_team_node_send_config(&g_team_node, member_id);
        if (ret == SLE_TEAM_OK) {
            g_team_rt.relay_config_pending_member_ids[i] = 0U;
            osal_printk("[team] relay config retry sent member=%u allow=%u ret=0\r\n",
                member_id, member->relay_allowed);
        } else {
            osal_printk("[team] relay config notify pending member=%u allow=%u ret=%d\r\n",
                member_id, member->relay_allowed, ret);
        }
    }
}

static void team_leader_rebalance_relays(uint8_t force_now)
{
    uint32_t now_s;
    uint16_t timeout_s;
    uint8_t i;
    uint8_t online_count = 0U;
    uint8_t known_count;
    uint8_t pending_count;
    uint8_t demand_count;
    uint8_t relay_count = 0U;
    uint8_t relay_target;
    uint8_t relay_budget;
    uint8_t changed = 0U;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER || g_team_node.cfg.pairing_enabled != 0U) {
        return;
    }
    now_s = team_now_s(NULL);
    if (force_now == 0U &&
        team_interval_not_reached(now_s, g_team_rt.relay_rebalance_last_s, SLE_TEAM_RELAY_REBALANCE_INTERVAL_S) !=
            0U) {
        return;
    }
    g_team_rt.relay_rebalance_last_s = now_s;
    timeout_s = g_team_node.cfg.heartbeat_timeout_s != 0U ? g_team_node.cfg.heartbeat_timeout_s :
        CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S;

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        sle_team_member_record_t *member = &g_team_node.members[i];

        if (member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        if (member->relay_allowed == 0U) {
            continue;
        }
        if (member->online == 0U) {
            if (team_leader_set_member_relay_allowed(member, 0U, "offline", 0U) == SLE_TEAM_OK) {
                changed = 1U;
            }
            continue;
        }
        if (team_elapsed_exceeds(now_s, member->last_seen_s,
                (uint32_t)timeout_s * SLE_TEAM_RELAY_REVOKE_STALE_FACTOR) != 0U) {
            if (team_leader_set_member_relay_allowed(member, 0U, "stale", 1U) == SLE_TEAM_OK) {
                changed = 1U;
            }
        }
    }

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];

        if (member->online == 0U || member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        online_count++;
        if (member->relay_allowed != 0U) {
            relay_count++;
        }
    }

    known_count = team_leader_known_member_count();
    pending_count = team_leader_pending_member_count();
    demand_count = known_count > online_count ? known_count : online_count;
    if ((uint16_t)demand_count + (uint16_t)pending_count > SLE_TEAM_MAX_MEMBERS) {
        demand_count = SLE_TEAM_MAX_MEMBERS;
    } else {
        demand_count = (uint8_t)(demand_count + pending_count);
    }
    relay_target = team_leader_relay_target_with_failover(demand_count, relay_count, now_s);
    relay_budget = team_leader_relay_budget();
    if (relay_target > relay_budget) {
        relay_target = relay_budget;
    }
    if (pending_count != 0U || known_count != online_count || relay_count != relay_target) {
        osal_printk("[team] relay rebalance demand online=%u known=%u pending=%u demand=%u direct_cap=%u relay=%u target=%u budget=%u\r\n",
            online_count, known_count, pending_count, demand_count, team_leader_direct_capacity(),
            relay_count, relay_target, relay_budget);
    }

    while (relay_count > relay_target) {
        sle_team_member_record_t *victim = team_leader_pick_worst_active_relay(now_s, timeout_s);

        if (victim == NULL) {
            break;
        }
        if (team_leader_set_member_relay_allowed(victim, 0U, "auto-demote", 1U) != SLE_TEAM_OK) {
            break;
        }
        relay_count--;
        changed = 1U;
    }

    while (relay_count < relay_target) {
        sle_team_member_record_t *candidate = team_leader_pick_best_relay_candidate(now_s, timeout_s);

        if (candidate == NULL) {
            break;
        }
        if (team_leader_set_member_relay_allowed(candidate, 1U, "auto-promote", 1U) != SLE_TEAM_OK) {
            break;
        }
        relay_count++;
        changed = 1U;
    }

    if (relay_count == relay_target && relay_target != 0U &&
        pending_count == 0U && known_count == online_count) {
        changed = (uint8_t)(changed | team_leader_relay_swap_tick(now_s, timeout_s, 1U));
    } else {
        team_leader_relay_swap_reset();
    }

    g_team_rt.relay_online_count = relay_count;
    g_team_rt.relay_target_count = relay_target;
    g_team_rt.relay_budget_count = relay_budget;
    if (changed != 0U || relay_count < relay_target) {
        osal_printk("[team] relay rebalance online=%u relay=%u target=%u changed=%u\r\n",
            online_count, relay_count, relay_target, changed);
    }
}

static void team_log(void *user_ctx, const char *text)
{
    unused(user_ctx);
    osal_printk("[state] %s\r\n", text);
}

static uint8_t team_member_current_next_hop_id(void)
{
    uint16_t conn_id;
    team_conn_track_t *track;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
        return 0U;
    }
    if (g_team_node.upstream_parent_state == SLE_TEAM_PARENT_CONNECTED &&
        g_team_node.upstream_parent_id != 0U && g_team_node.upstream_parent_id != SLE_TEAM_BROADCAST_ID) {
        return g_team_node.upstream_parent_id;
    }
    if (team_route_find(g_team_node.cfg.leader_id, TEAM_CONN_DIR_UPSTREAM, &conn_id) != 0U) {
        track = team_conn_track_find(conn_id);
        if (team_conn_track_route_id_is_trusted(track) != 0U) {
            return track->route_id;
        }
    }
    conn_id = get_connect_id();
    track = team_conn_track_find(conn_id);
    if (team_conn_track_route_id_is_trusted(track) != 0U) {
        return track->route_id;
    }
    return g_team_node.cfg.leader_id;
}

static void team_cli_print(void *user_ctx, const char *text)
{
    unused(user_ctx);
    if (strncmp(text, "pairing start ret=0", 19) == 0) {
        team_leader_pairing_restart_scan("cli");
    }
    if (strncmp(text, "allow all ret=0", 15) == 0 ||
        strncmp(text, "allow only", 10) == 0 ||
        strncmp(text, "allow add", 9) == 0 ||
        strncmp(text, "allow del", 9) == 0 ||
        strncmp(text, "pairing approve", 15) == 0 ||
        strncmp(text, "pairing stopped", 15) == 0) {
        (void)team_nv_allowed_save_from_node();
    }
    if (strncmp(text, "sle_tx_ok", 9) == 0) {
        osal_printk("[sle-tx-ok] %s\r\n", text + 10);
    } else if (strncmp(text, "sle_tx_fail", 11) == 0) {
        osal_printk("[sle-tx-fail] %s\r\n", text + 12);
    } else {
        osal_printk("[cli-rx] %s\r\n", text);
    }
}

static void team_joined(void *user_ctx, uint8_t member_id)
{
    int ret;
    uint8_t next_hop_id;

    unused(user_ctx);
    osal_printk("[team] joined member=%u\r\n", member_id);
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        const sle_team_member_record_t *member = team_display_find_member_record(member_id);
        team_display_event_t event = team_display_note_member_joined(member_id);

        if (event != TEAM_DISPLAY_EVENT_NONE) {
            if (member != NULL) {
                team_display_show_event(event, member_id, member->latitude_e6,
                    member->longitude_e6, member->last_seen_s);
            } else {
                team_display_show_event(event, member_id, 0, 0, team_now_s(NULL));
            }
        }
        g_team_rt.last_online_member_count = team_online_member_count();
    }
    team_display_refresh_status();
    team_ws2812_refresh_network_state();
    if (g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER || member_id != g_team_node.cfg.self_id) {
        return;
    }
    team_member_upstream_recover_clear("joined");
    next_hop_id = team_member_current_next_hop_id();
    if (next_hop_id == 0U) {
        return;
    }
    team_upstream_parent_note(next_hop_id, SLE_TEAM_PARENT_CONNECTED, "joined");
    ret = sle_team_node_send_route_update(&g_team_node, g_team_node.cfg.leader_id,
        g_team_node.cfg.leader_id, (uint8_t)SLE_TEAM_PARENT_CONNECTED, next_hop_id);
    osal_printk("[team] route update next_hop=%u ret=%d\r\n", next_hop_id, ret);
}

static void team_position(void *user_ctx, uint8_t member_id, const sle_team_pos_body_t *pos)
{
    unused(user_ctx);
    osal_printk("[team] pos member=%u lat=%ld lon=%ld battery=%u fix=%u sat=%u\r\n",
        member_id,
        (long)pos->latitude_e6,
        (long)pos->longitude_e6,
        pos->battery_percent,
        pos->fix_status,
        pos->sat_count);
    team_display_refresh_status();
}

static void team_alert(void *user_ctx, uint8_t member_id, uint8_t reason)
{
    unused(user_ctx);
    osal_printk("[team] alert member=%u reason=%u\r\n", member_id, reason);
    if (reason == SLE_TEAM_ALERT_LEAVE) {
        (void)team_leader_mark_member_offline(member_id, "member_leave");
        return;
    }
    if (reason == SLE_TEAM_ALERT_TIMEOUT) {
        const sle_team_member_record_t *member = team_display_find_member_record(member_id);

        team_display_note_member_offline(member_id, TEAM_DISPLAY_EVENT_TIMEOUT);
        if (member != NULL) {
            team_display_show_event(TEAM_DISPLAY_EVENT_TIMEOUT, member_id, member->latitude_e6,
                member->longitude_e6, member->last_seen_s);
        } else {
            team_display_show_event(TEAM_DISPLAY_EVENT_TIMEOUT, member_id, 0, 0, team_now_s(NULL));
        }
    }
}

static void team_member_reset_after_leave(void)
{
    team_member_drop_relay_children("member-leave");
    g_team_rt.member_upstream_recover_last_s = 0U;
    g_team_rt.member_upstream_not_ready_since_s = 0U;
    g_team_rt.member_upstream_not_ready_count = 0U;
    (void)team_nv_config_clear();
    g_team_rt.role_configured = 0U;
    g_team_rt.role_request_pending = 0U;
    g_team_rt.role_request_last_ret = SLE_TEAM_OK;
    (void)memset_s(&g_team_node, sizeof(g_team_node), 0, sizeof(g_team_node));
    (void)memset_s(&g_team_cli, sizeof(g_team_cli), 0, sizeof(g_team_cli));
    g_team_rt.leader_label[0] = '\0';
    team_identity_refresh_labels();
    team_display_refresh_status();
    team_ws2812_refresh_network_state();
}

static void team_on_relay_offline(void *user_ctx, uint8_t member_id)
{
    unused(user_ctx);
    osal_printk("[team] relay offline event member=%u trigger immediate rebalance\r\n", member_id);
    team_leader_failover_begin(member_id, team_now_s(NULL));
    team_leader_rebalance_relays(1U);
}

static void team_web_record_packet(sle_team_web_event_direction_t direction, const uint8_t *buf, uint16_t len,
    const char *fallback)
{
    sle_team_mesh_packet_t mesh;
    sle_team_app_packet_t app;
    const uint8_t *app_payload = NULL;
    uint16_t app_payload_len = 0;
    uint8_t channel_hash = 0;
    uint8_t cipher_mac[2] = {0};
    char summary[SLE_TEAM_WEB_EVENT_SUMMARY_SIZE];

    if (buf != NULL && len > 0U && sle_team_decode_mesh_packet(&mesh, buf, len) == SLE_TEAM_OK &&
        sle_team_unwrap_mesh_group_data(&mesh, &channel_hash, cipher_mac, &app_payload, &app_payload_len) ==
            SLE_TEAM_OK &&
        sle_team_decode_app_packet(&app, app_payload, app_payload_len) == SLE_TEAM_OK) {
        (void)snprintf(summary, sizeof(summary), "%s %u->%u seq=%u",
            sle_team_web_msg_type_name(app.app_msg_type), app.src_id, app.dst_id, app.seq);
        osal_printk("[%s] %s\r\n", direction == SLE_TEAM_WEB_EVENT_TX ? "sle-tx-ok" : "sle-rx", summary);
        team_led_post(direction == SLE_TEAM_WEB_EVENT_TX ? SLE_TEAM_LED_EVENT_TX : SLE_TEAM_LED_EVENT_RX);
        sle_team_web_event_push(&g_team_events, team_now_s(NULL), direction, app.app_msg_type, app.src_id, app.dst_id,
            app.seq, summary);
        return;
    }

    sle_team_web_event_push(&g_team_events, team_now_s(NULL), direction, 0U, 0U, 0U, 0U,
        fallback != NULL ? fallback : "packet");
}

static void *team_reboot_task(const char *arg)
{
    const char *reason = (arg != NULL) ? arg : "manual";

    osal_msleep(800);
    osal_printk("[team] reboot now reason=%s\r\n", reason);
    (void)uapi_watchdog_init(1);
    (void)uapi_watchdog_set_time(1);
    (void)uapi_watchdog_enable(WDT_MODE_RESET);
    while (1) {
    }
    return NULL;
}

static void team_reboot_schedule(const char *reason)
{
    osal_task *task = NULL;

    osal_kthread_lock();
    task = osal_kthread_create((osal_kthread_handler)team_reboot_task, (void *)reason, "TeamReboot",
        SLE_TEAM_FACTORY_RESET_TASK_STACK_SIZE);
    if (task != NULL) {
        osal_kthread_set_priority(task, SLE_TEAM_FACTORY_RESET_TASK_PRIO);
    }
    osal_kthread_unlock();
}

#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
static int team_wifi_ap_start(void)
{
    softap_config_stru ap_config = {0};
    softap_config_advance_stru advance_config = {0};
    softap_config_advance_stru advance_fallback = {0};
    char ifname[SLE_TEAM_WIFI_IFNAME_MAX_SIZE + 1] = "ap0";
    struct netif *netif_p = NULL;
    ip4_addr_t gw;
    ip4_addr_t ipaddr;
    ip4_addr_t netmask;
    errcode_t ret;
    err_t lwip_ret;

    if (wifi_is_softap_enabled() != 0) {
        team_wifi_print("softap already enabled");
        return 0;
    }

    IP4_ADDR(&ipaddr, 192, 168, 43, CONFIG_SLE_TEAM_WIFI_AP_IP_LAST);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 43, CONFIG_SLE_TEAM_WIFI_AP_IP_LAST);

    if (g_team_rt.softap_ssid[0] == '\0') {
        team_identity_set_fallback();
    }
    (void)snprintf((char *)ap_config.ssid, sizeof(ap_config.ssid), "%s", g_team_rt.softap_ssid);
    (void)snprintf((char *)ap_config.pre_shared_key, sizeof(ap_config.pre_shared_key), "%s",
        CONFIG_SLE_TEAM_WIFI_AP_PSK);
    ap_config.security_type = SLE_TEAM_WIFI_SECURITY_COMPAT_MIX;
    ap_config.channel_num = CONFIG_SLE_TEAM_WIFI_AP_CHANNEL;
    ap_config.wifi_psk_type = WIFI_WPA_PSK_NOT_USE;

    advance_config.beacon_interval = 100;
    advance_config.dtim_period = 2;
    advance_config.gi = 0;
    advance_config.group_rekey = 86400;
    /* v2 baseline used ax mix mode (value 4) and is field-proven stable. */
    advance_config.protocol_mode = SLE_TEAM_WIFI_PROTOCOL_COMPAT_AX;
    advance_config.hidden_ssid_flag = 1;

    ret = wifi_set_softap_config_advance(&advance_config);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[team-wifi] softap advance config failed ret=0x%x\r\n", ret);
        return -1;
    }
    ret = wifi_softap_enable(&ap_config);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[team-wifi] softap enable failed ret=0x%x with mix/ax, fallback to wpa2+11bgn\r\n", ret);
        ap_config.security_type = WIFI_SEC_TYPE_WPA2PSK;
        advance_fallback = advance_config;
        advance_fallback.protocol_mode = WIFI_MODE_11B_G_N;
        ret = wifi_set_softap_config_advance(&advance_fallback);
        if (ret != ERRCODE_SUCC) {
            osal_printk("[team-wifi] softap fallback advance config failed ret=0x%x\r\n", ret);
            return -1;
        }
        ret = wifi_softap_enable(&ap_config);
        if (ret != ERRCODE_SUCC) {
            osal_printk("[team-wifi] softap fallback enable failed ret=0x%x\r\n", ret);
            return -1;
        }
    }
    netif_p = netif_find(ifname);
    if (netif_p == NULL) {
        team_wifi_print("softap netif ap0 not found");
        (void)wifi_softap_disable();
        return -1;
    }
    lwip_ret = netifapi_netif_set_addr(netif_p, &ipaddr, &netmask, &gw);
    if (lwip_ret != ERR_OK) {
        osal_printk("[team-wifi] softap set ip failed ret=%ld\r\n", (long)lwip_ret);
        (void)wifi_softap_disable();
        return -1;
    }
    lwip_ret = netifapi_dhcps_start(netif_p, NULL, 0);
    if (lwip_ret != ERR_OK) {
        osal_printk("[team-wifi] softap dhcp start failed ret=%ld\r\n", (long)lwip_ret);
        team_wifi_print("softap kept up without dhcp; use static client ip if needed");
    }

    osal_printk("[team-wifi] softap started ssid=%s ip=192.168.43.%u channel=%u\r\n",
        g_team_rt.softap_ssid,
        CONFIG_SLE_TEAM_WIFI_AP_IP_LAST,
        CONFIG_SLE_TEAM_WIFI_AP_CHANNEL);
    team_wifi_print_status();
    return 0;
}

static int team_http_send_all(int fd, const char *data, size_t len)
{
    size_t sent_len = 0;

    while (sent_len < len) {
        int ret = send(fd, data + sent_len, len - sent_len, 0);
        if (ret <= 0) {
            g_team_http_last_errno = errno;
            osal_printk("[team-wifi] http send failed fd=%d ret=%d errno=%d sent=%u/%u\r\n",
                fd, ret, g_team_http_last_errno, (unsigned int)sent_len, (unsigned int)len);
            return -1;
        }
        sent_len += (size_t)ret;
    }
    return 0;
}

static void team_http_set_client_timeouts(int fd)
{
    struct timeval recv_timeout = {
        .tv_sec = SLE_TEAM_HTTP_RECV_TIMEOUT_MS / 1000,
        .tv_usec = (SLE_TEAM_HTTP_RECV_TIMEOUT_MS % 1000) * 1000,
    };
    struct timeval send_timeout = {
        .tv_sec = SLE_TEAM_HTTP_SEND_TIMEOUT_MS / 1000,
        .tv_usec = (SLE_TEAM_HTTP_SEND_TIMEOUT_MS % 1000) * 1000,
    };

    (void)setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
    (void)setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));
}

static int team_http_recv_request_line(int fd)
{
    size_t used = 0U;
    size_t cap = sizeof(g_team_http_req_buf) - 1U;

    while (used < cap) {
        int ret = recv(fd, g_team_http_req_buf + used, cap - used, 0);
        if (ret < 0) {
            g_team_http_last_errno = errno;
            if (used == 0U) {
                osal_printk("[team-wifi] http recv failed fd=%d ret=%d errno=%d\r\n", fd, ret, g_team_http_last_errno);
                return -1;
            }
            break;
        }
        if (ret == 0) {
            if (used == 0U) {
                osal_printk("[team-wifi] http recv peer closed fd=%d\r\n", fd);
                return -1;
            }
            break;
        }
        used += (size_t)ret;
        g_team_http_req_buf[used] = '\0';
        if (strstr(g_team_http_req_buf, "\r\n") != NULL || strchr(g_team_http_req_buf, '\n') != NULL) {
            break;
        }
    }

    if (used == 0U) {
        return -1;
    }
    return (int)used;
}

static void team_http_send_response(int fd, const char *status, const char *content_type, const char *body)
{
    char header[320];
    size_t body_len = body != NULL ? strlen(body) : 0U;
    int header_len;

    header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %u\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-store, no-cache, must-revalidate\r\n"
        "Pragma: no-cache\r\n"
        "Expires: 0\r\n"
        "\r\n",
        status, content_type, (unsigned int)body_len);
    if (header_len > 0 && header_len < (int)sizeof(header)) {
        int send_ret = team_http_send_all(fd, header, (size_t)header_len);
        osal_printk("[team-wifi] http header sent fd=%d len=%d ret=%d\r\n", fd, header_len, send_ret);
    } else {
        osal_printk("[team-wifi] http header build failed fd=%d len=%d\r\n", fd, header_len);
    }
    if (body_len > 0U) {
        int send_ret = team_http_send_all(fd, body, body_len);
        osal_printk("[team-wifi] http body sent fd=%d len=%u ret=%d\r\n", fd, (unsigned int)body_len, send_ret);
    }
}

static void team_http_send_redirect(int fd, const char *location)
{
    char header[320];
    int header_len;

    if (location == NULL) {
        location = "/";
    }
    header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 303 See Other\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-store, no-cache, must-revalidate\r\n"
        "Pragma: no-cache\r\n"
        "Expires: 0\r\n"
        "\r\n",
        location);
    if (header_len > 0 && header_len < (int)sizeof(header)) {
        (void)team_http_send_all(fd, header, (size_t)header_len);
    } else {
        osal_printk("[team-wifi] http redirect header build failed fd=%d len=%d\r\n", fd, header_len);
    }
}

static void team_factory_reset_schedule(void)
{
    team_reboot_schedule("factory-reset");
}

static void team_http_send_factory_reset_page(int fd)
{
    const char *body =
        "<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>Factory reset</title></head><body>"
        "<h3>Factory reset requested</h3><p>The board is rebooting to unconfigured mode.</p>"
        "<p>Reconnect to this board WiFi and open /pairing after reboot.</p>"
        "</body></html>";

    team_http_send_response(fd, "200 OK", "text/html; charset=utf-8", body);
}

static void team_http_send_factory_reset_failed_page(int fd)
{
    const char *body =
        "<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>Factory reset failed</title></head><body>"
        "<h3>Factory reset failed</h3><p>Flash config was not cleared. Please retry from /pairing.</p>"
        "</body></html>";

    team_http_send_response(fd, "500 Internal Server Error", "text/html; charset=utf-8", body);
}

static void team_http_get_path(char *out, size_t out_size)
{
    char *start;
    char *end;
    size_t len;

    if (out == NULL || out_size == 0U) {
        return;
    }
    out[0] = '\0';
    start = strchr(g_team_http_req_buf, ' ');
    if (start == NULL) {
        return;
    }
    start++;
    end = strchr(start, ' ');
    if (end == NULL || end <= start) {
        return;
    }
    len = (size_t)(end - start);
    if (len >= out_size) {
        len = out_size - 1U;
    }
    (void)memcpy_s(out, out_size, start, len);
    out[len] = '\0';
}

static const char *team_http_query_value_start(const char *path, const char *key)
{
    char pattern[24];
    const char *p;

    if (path == NULL || key == NULL) {
        return NULL;
    }
    (void)snprintf(pattern, sizeof(pattern), "%s=", key);
    p = strstr(path, pattern);
    if (p == NULL) {
        return NULL;
    }
    return p + strlen(pattern);
}

static int team_http_query_number(const char *path, const char *key, int64_t min_value, int64_t max_value,
    unsigned long positive_limit, unsigned long negative_limit, int64_t *out)
{
    const char *p;
    unsigned long limit;
    uint8_t negative = 0U;
    unsigned long abs_value = 0UL;
    int64_t signed_value;
    uint8_t digits = 0U;

    if (out == NULL) {
        return -1;
    }
    p = team_http_query_value_start(path, key);
    if (p == NULL) {
        return -1;
    }
    if (negative_limit != 0UL && *p == '-') {
        negative = 1U;
        p++;
    } else if (negative_limit != 0UL && *p == '+') {
        p++;
    }
    limit = negative != 0U ? negative_limit : positive_limit;
    while (*p >= '0' && *p <= '9') {
        abs_value = abs_value * 10UL + (unsigned long)(*p - '0');
        p++;
        digits++;
        if (abs_value > limit) {
            return -1;
        }
    }
    if (*p != '\0' && *p != '&') {
        return -1;
    }
    if (digits == 0U) {
        return -1;
    }
    signed_value = negative != 0U ? -(int64_t)abs_value : (int64_t)abs_value;
    if (signed_value < (int64_t)min_value || signed_value > (int64_t)max_value) {
        return -1;
    }
    *out = signed_value;
    return 0;
}

static int team_http_query_u8(const char *path, const char *key, uint8_t min_value, uint8_t max_value,
    uint8_t *out)
{
    int64_t value;

    if (out == NULL) {
        return -1;
    }
    if (team_http_query_number(path, key, (int64_t)min_value, (int64_t)max_value, 255UL, 0UL, &value) != 0) {
        return -1;
    }
    *out = (uint8_t)value;
    return 0;
}

static int team_http_query_u16(const char *path, const char *key, uint16_t min_value, uint16_t max_value,
    uint16_t *out)
{
    int64_t value;

    if (out == NULL) {
        return -1;
    }
    if (team_http_query_number(path, key, (int64_t)min_value, (int64_t)max_value, 65535UL, 0UL, &value) != 0) {
        return -1;
    }
    *out = (uint16_t)value;
    return 0;
}

static int team_http_query_i32(const char *path, const char *key, int32_t min_value, int32_t max_value,
    int32_t *out)
{
    int64_t value;

    if (out == NULL) {
        return -1;
    }
    if (team_http_query_number(path, key, (int64_t)min_value, (int64_t)max_value,
        2147483647UL, 2147483648UL, &value) != 0) {
        return -1;
    }
    *out = (int32_t)value;
    return 0;
}

static int team_http_query_hex16(const char *path, const char *key, uint16_t *out)
{
    const char *p;
    uint16_t value = 0U;
    uint8_t digits = 0U;

    if (out == NULL) {
        return -1;
    }
    p = team_http_query_value_start(path, key);
    if (p == NULL) {
        return -1;
    }
    while (digits < 4U) {
        char ch = *p++;
        uint8_t nibble;
        if (ch >= '0' && ch <= '9') {
            nibble = (uint8_t)(ch - '0');
        } else if (ch >= 'a' && ch <= 'f') {
            nibble = (uint8_t)(ch - 'a' + 10);
        } else if (ch >= 'A' && ch <= 'F') {
            nibble = (uint8_t)(ch - 'A' + 10);
        } else {
            return -1;
        }
        value = (uint16_t)((value << 4U) | nibble);
        digits++;
    }
    *out = value;
    return 0;
}

static uint8_t team_route_id_from_suffix(uint16_t suffix)
{
    uint8_t id = (uint8_t)(suffix & 0xFFU);

    if (id == 0U || id == SLE_TEAM_BROADCAST_ID) {
        id = (uint8_t)(((suffix >> 8U) % 254U) + 1U);
    }
    return id;
}

static int team_http_write_power_json(char *out, size_t out_size)
{
    int len;

    if (out == NULL || out_size == 0U) {
        return SLE_TEAM_ERR_ARG;
    }
    (void)team_battery_sample_once(0U);
    team_chrg_sample();
    len = snprintf(out, out_size,
        "{\"ok\":true,\"fw\":\"%s\",\"adcReady\":%s,\"batteryValid\":%s,"
        "\"adcMv\":%u,\"vbatMv\":%u,\"vbat_mv\":%u,\"batteryPercent\":%u,"
        "\"powerSource\":\"%s\",\"sourceCertain\":%s,\"charging\":%s,"
        "\"chrgReady\":%s,\"chrgPin\":%u,\"chrgRaw\":%u,\"chrgActiveLow\":%s,"
        "\"chrgExternalPullup\":%s,\"lastSampleMs\":%lu,\"sampleRet\":%ld}",
        SLE_TEAM_FW_VERSION,
        g_team_rt.adc_ready != 0U ? "true" : "false",
        g_team_rt.battery_valid != 0U ? "true" : "false",
        g_team_rt.adc_sample_mv,
        g_team_rt.battery_mv,
        g_team_rt.battery_mv,
        g_team_rt.battery_percent,
        team_power_source_name(),
        team_power_source_certain() != 0U ? "true" : "false",
        g_team_rt.charging != 0U ? "true" : "false",
        g_team_rt.chrg_ready != 0U ? "true" : "false",
        g_team_rt.chrg_pin,
        g_team_rt.chrg_raw,
        g_team_rt.chrg_active_low != 0U ? "true" : "false",
        (uint8_t)CONFIG_SLE_TEAM_CHRG_EXTERNAL_PULLUP != 0U ? "true" : "false",
        (unsigned long)g_team_rt.battery_sample_last_ms,
        (long)g_team_rt.battery_sample_last_ret);
    return len < 0 || len >= (int)out_size ? SLE_TEAM_ERR_BUF : len;
}

static int team_http_append_power_to_status_json(char *out, size_t out_size)
{
    char power_json[512];
    size_t used;
    int power_ret;
    int len;

    if (out == NULL || out_size == 0U) {
        return SLE_TEAM_ERR_ARG;
    }
    used = strlen(out);
    if (used == 0U || out[used - 1U] != '}') {
        return SLE_TEAM_ERR_FORMAT;
    }
    power_ret = team_http_write_power_json(power_json, sizeof(power_json));
    if (power_ret < 0) {
        return power_ret;
    }
    used--;
    len = snprintf(&out[used], out_size - used, ",\"power\":%s}", power_json);
    return len < 0 || len >= (int)(out_size - used) ? SLE_TEAM_ERR_BUF : (int)(used + (size_t)len);
}

static void team_http_send_power_json_response(int fd)
{
    int ret = team_http_write_power_json(g_team_http_json_buf, sizeof(g_team_http_json_buf));

    team_http_send_response(fd, ret < 0 ? "500 Internal Server Error" : "200 OK", "application/json",
        ret < 0 ? "{\"ok\":false,\"error\":\"power\"}" : g_team_http_json_buf);
}

static void team_http_send_status_json_response(int fd)
{
    int ret;

    if (g_team_rt.role_configured == 0U) {
        char power_json[512];
        int power_ret = team_http_write_power_json(power_json, sizeof(power_json));

        if (power_ret < 0) {
            (void)snprintf(power_json, sizeof(power_json), "{\"ok\":false,\"error\":\"power\"}");
        }
        (void)snprintf(g_team_http_json_buf, sizeof(g_team_http_json_buf),
            "{\"configured\":false,\"selfLabel\":\"%s\",\"routeId\":%u,\"macReady\":%s,"
            "\"macSuffix\":\"%02X%02X\",\"ssid\":\"%s\",\"transport\":\"ws63-softap\",\"power\":%s}",
            g_team_rt.self_label,
            g_team_rt.route_id,
            g_team_rt.self_mac_ready != 0U ? "true" : "false",
            g_team_rt.self_mac[4],
            g_team_rt.self_mac[5],
            g_team_rt.softap_ssid,
            power_json);
        team_http_send_response(fd, "200 OK", "application/json", g_team_http_json_buf);
        return;
    }

    {
        sle_team_web_route_metrics_t metrics;
        const sle_team_web_route_metrics_t *route_metrics = NULL;

        if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
            (void)memset_s(&metrics, sizeof(metrics), 0, sizeof(metrics));
            metrics.active_count = g_team_rt.route_metrics_active;
            metrics.direct_count = g_team_rt.route_metrics_direct;
            metrics.relayed_count = g_team_rt.route_metrics_relayed;
            metrics.unreachable_count = g_team_rt.route_metrics_unreachable;
            metrics.stale_count = g_team_rt.route_metrics_stale;
            metrics.converged = g_team_rt.route_metrics_converged;
            metrics.relay_target_count = g_team_rt.relay_target_count;
            metrics.relay_online_count = g_team_rt.relay_online_count;
            metrics.relay_budget_count = g_team_rt.relay_budget_count;
            metrics.epoch = g_team_rt.route_metrics_epoch;
            metrics.last_change_s = g_team_rt.route_metrics_last_change_s;
            metrics.last_converged_s = g_team_rt.route_metrics_last_converged_s;
            metrics.hint_sent_total = g_team_rt.route_hint_sent_total;
            metrics.hint_failed_total = g_team_rt.route_hint_failed_total;
            metrics.hint_cooldown_skipped_total = g_team_rt.route_hint_cooldown_skipped_total;
            metrics.route_update_rx_total = g_team_rt.route_update_rx_total;
            metrics.route_reparent_total = g_team_rt.route_reparent_total;
            metrics.route_reparent_last_s = g_team_rt.route_reparent_last_s;
            route_metrics = &metrics;
        }
        ret = sle_team_web_write_status_json(&g_team_node, team_now_s(NULL), "ws63-softap", route_metrics,
            g_team_http_json_buf, sizeof(g_team_http_json_buf));
    }
    if (ret >= 0) {
        ret = team_http_append_power_to_status_json(g_team_http_json_buf, sizeof(g_team_http_json_buf));
    }
    team_http_send_response(fd, ret < 0 ? "500 Internal Server Error" : "200 OK", "application/json",
        ret < 0 ? "{\"error\":\"status\"}" : g_team_http_json_buf);
}

static const char *team_location_send_reason(int send_ret)
{
    switch (send_ret) {
        case SLE_TEAM_OK:
            return "OK";
        case SLE_TEAM_ERR_UNSUPPORTED:
            return "NOT_READY_OR_NO_ROUTE";
        case SLE_TEAM_ERR_FORMAT:
            return "WRITE_FAIL";
        case SLE_TEAM_ERR_ARG:
            return "BAD_ARG";
        case SLE_TEAM_ERR_BUF:
            return "BUF";
        default:
            return "SEND_FAIL";
    }
}

static void team_http_send_location_event(int fd, uint8_t dst_id, int send_ret, int32_t lat, int32_t lon)
{
    const uint32_t now_s = team_now_s(NULL);
    const uint16_t seq = g_team_node.next_seq == 0U ? 0U : (uint16_t)(g_team_node.next_seq - 1U);
    const char *direction = send_ret == SLE_TEAM_OK ? "tx" : "fail";
    const char *summary = send_ret == SLE_TEAM_OK ? "location sent" : "location send failed";
    const char *reason = team_location_send_reason(send_ret);
    const char *status = send_ret == SLE_TEAM_OK ? "200 OK" : "500 Internal Server Error";

    (void)snprintf(g_team_http_json_buf, sizeof(g_team_http_json_buf),
        "{\"id\":\"ws63-location-%lu\",\"time\":\"%lu\",\"direction\":\"%s\","
        "\"type\":\"POS_REPORT\",\"srcId\":%u,\"dstId\":%u,\"seq\":%u,"
        "\"summary\":\"%s\",\"ret\":%d,\"reason\":\"%s\"}",
        (unsigned long)now_s,
        (unsigned long)now_s,
        direction,
        g_team_node.cfg.self_id,
        dst_id,
        seq,
        summary,
        send_ret,
        reason);
    team_http_send_response(fd, status, "application/json", g_team_http_json_buf);

    if (send_ret == SLE_TEAM_OK) {
        char event_summary[SLE_TEAM_WEB_EVENT_SUMMARY_SIZE];
        (void)snprintf(event_summary, sizeof(event_summary), "POS_REPORT %u->%u lat=%ld lon=%ld",
            g_team_node.cfg.self_id, dst_id, (long)lat, (long)lon);
        sle_team_web_event_push(&g_team_events, now_s, SLE_TEAM_WEB_EVENT_SYSTEM, SLE_TEAM_APP_POS_REPORT,
            g_team_node.cfg.self_id, dst_id, seq, event_summary);
    } else {
        char event_summary[SLE_TEAM_WEB_EVENT_SUMMARY_SIZE];
        (void)snprintf(event_summary, sizeof(event_summary), "POS_REPORT fail dst=%u ret=%d reason=%s",
            dst_id, send_ret, reason);
        sle_team_web_event_push(&g_team_events, now_s, SLE_TEAM_WEB_EVENT_SYSTEM, SLE_TEAM_APP_POS_REPORT,
            g_team_node.cfg.self_id, dst_id, seq, event_summary);
    }
}

static void team_http_send_config_status_json(int fd)
{
    int ret = team_cfg_status_write_json(g_team_http_json_buf, sizeof(g_team_http_json_buf));

    team_http_send_response(fd, ret < 0 ? "500 Internal Server Error" : "200 OK", "application/json",
        ret < 0 ? "{\"ok\":false,\"error\":\"config-status\"}" : g_team_http_json_buf);
}

static void team_http_send_config_action_json(int fd, const char *action, int action_ret)
{
    char status_json[640];
    int status_ret = team_cfg_status_write_json(status_json, sizeof(status_json));
    int len;

    if (status_ret < 0) {
        (void)snprintf(status_json, sizeof(status_json), "{\"ok\":false,\"error\":\"status\",\"ret\":%d}",
            status_ret);
    }
    len = snprintf(g_team_http_json_buf, sizeof(g_team_http_json_buf),
        "{\"ok\":%s,\"action\":\"%s\",\"ret\":%d,\"config\":%s}",
        action_ret == SLE_TEAM_OK ? "true" : "false",
        action != NULL ? action : "config",
        action_ret,
        status_json);
    team_http_send_response(fd, len < 0 || len >= (int)sizeof(g_team_http_json_buf) ?
        "500 Internal Server Error" : "200 OK", "application/json",
        len < 0 || len >= (int)sizeof(g_team_http_json_buf) ?
            "{\"ok\":false,\"error\":\"config-action\"}" : g_team_http_json_buf);
}

static void team_http_send_config_bad_request(int fd, const char *error)
{
    int len = snprintf(g_team_http_json_buf, sizeof(g_team_http_json_buf),
        "{\"ok\":false,\"error\":\"%s\"}", error != NULL ? error : "bad-request");

    team_http_send_response(fd, len < 0 || len >= (int)sizeof(g_team_http_json_buf) ?
        "500 Internal Server Error" : "400 Bad Request", "application/json",
        len < 0 || len >= (int)sizeof(g_team_http_json_buf) ?
            "{\"ok\":false,\"error\":\"bad-request\"}" : g_team_http_json_buf);
}

static void team_http_append_str(char *buf, size_t buf_size, size_t *used, const char *text)
{
    size_t text_len;

    if (*used >= buf_size) {
        return;
    }

    text_len = strlen(text);
    if (text_len >= buf_size - *used) {
        text_len = buf_size - *used - 1U;
    }

    if (text_len > 0U) {
        (void)memcpy_s(buf + *used, buf_size - *used, text, text_len);
        *used += text_len;
        buf[*used] = '\0';
    }
}

static void team_http_append_fmt(char *buf, size_t buf_size, size_t *used, const char *fmt, ...)
{
    va_list ap;
    int len;

    if (*used >= buf_size) {
        return;
    }

    va_start(ap, fmt);
    len = vsnprintf(buf + *used, buf_size - *used, fmt, ap);
    va_end(ap);
    if (len <= 0) {
        return;
    }
    if (len >= (int)(buf_size - *used)) {
        *used = buf_size - 1U;
        buf[*used] = '\0';
        return;
    }
    *used += (size_t)len;
}

static void team_http_append_power_rows(char *buf, size_t buf_size, size_t *used)
{
    (void)team_battery_sample_once(0U);
    team_chrg_sample();
    team_http_append_fmt(buf, buf_size, used,
        "<div class=\"row\"><span class=\"k\">Power</span><span class=\"v %s\">%s</span></div>",
        team_power_source_certain() != 0U ? "ok" : "warn",
        team_power_source_name());
    if (g_team_rt.battery_valid != 0U) {
        team_http_append_fmt(buf, buf_size, used,
            "<div class=\"row\"><span class=\"k\">VBAT</span><span class=\"v\">%u mV</span></div>",
            g_team_rt.battery_mv);
    } else {
        team_http_append_str(buf, buf_size, used,
            "<div class=\"row\"><span class=\"k\">VBAT</span><span class=\"v\">NA</span></div>");
    }
    team_http_append_fmt(buf, buf_size, used,
        "<div class=\"row\"><span class=\"k\">Battery</span><span class=\"v\">%u%%</span></div>",
        g_team_rt.battery_percent);
    team_http_append_fmt(buf, buf_size, used,
        "<div class=\"row\"><span class=\"k\">Charging</span><span class=\"v %s\">%s</span></div>",
        g_team_rt.charging != 0U ? "ok" : "warn",
        g_team_rt.charging != 0U ? "true" : "false");
    team_http_append_fmt(buf, buf_size, used,
        "<div class=\"row\"><span class=\"k\">CHRG IO2</span><span class=\"v\">raw=%u active_low=%u</span></div>",
        g_team_rt.chrg_raw, g_team_rt.chrg_active_low);
}

static void team_http_append_html_shell_start(char *buf, size_t buf_size, size_t *used, const char *active)
{
    static const char * const chunks[] = {
        "<!doctype html><html><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">",
        "<title>" WS63_CONSOLE_BOARD_TITLE "</title><style>",
        "body{margin:0;font-family:-apple-system,BlinkMacSystemFont,Segoe UI,sans-serif;"
        "background:" WS63_CONSOLE_COLOR_PAGE_BG ";color:" WS63_CONSOLE_COLOR_TEXT
        "}header{padding:18px 16px;background:" WS63_CONSOLE_COLOR_HEADER_BG ";color:white}",
        "h1{font-size:22px;margin:0 0 4px}.sub{opacity:.72;font-size:13px}"
        "main{padding:14px;display:grid;gap:12px}",
        ".card{background:" WS63_CONSOLE_COLOR_CARD_BG ";border:1px solid " WS63_CONSOLE_COLOR_BORDER
        ";border-radius:8px;padding:14px}"
        ".row{display:flex;justify-content:space-between;gap:12px;padding:7px 0;border-bottom:1px solid #edf0f3}",
        ".row:last-child{border-bottom:0}.k{color:" WS63_CONSOLE_COLOR_MUTED
        "}.v{font-weight:600;text-align:right;word-break:break-word}"
        "pre{white-space:pre-wrap;word-break:break-word;margin:0;font-size:12px;line-height:1.45}",
        ".ok{color:" WS63_CONSOLE_COLOR_OK "}.bad{color:" WS63_CONSOLE_COLOR_BAD "}.warn{color:"
        WS63_CONSOLE_COLOR_WARN "}.bar{display:flex;gap:8px;margin-top:10px;flex-wrap:wrap}"
        ".tag{font-size:12px;color:" WS63_CONSOLE_COLOR_MUTED ";margin-top:8px}"
        "a,button{font:inherit;border:1px solid #c9d0da;border-radius:6px;background:white;color:#182230;padding:8px 10px;text-decoration:none}"
        "input{font:inherit;border:1px solid #c9d0da;border-radius:6px;padding:8px 10px;max-width:70px}"
        "form{display:flex;gap:8px;flex-wrap:wrap;margin-top:10px}"
        "a.on{background:#182230;color:#fff;border-color:#182230}",
        "</style></head><body><header><h1>" WS63_CONSOLE_BOARD_TITLE "</h1>"
        "<div class=\"sub\">" WS63_CONSOLE_BOARD_SUBTITLE "</div></header><main>"
    };
    size_t i;

    buf[0] = '\0';
    *used = 0U;
    for (i = 0; i < sizeof(chunks) / sizeof(chunks[0]); i++) {
        team_http_append_str(buf, buf_size, used, chunks[i]);
        if (i == 0U && strcmp(active, "status") == 0) {
            team_http_append_str(buf, buf_size, used, "<meta http-equiv=\"refresh\" content=\"3\">");
        }
    }
    team_http_append_fmt(buf, buf_size, used,
        "<div class=\"bar\"><a class=\"%s\" href=\"" WS63_CONSOLE_TAB_STATUS_HREF "\">"
        WS63_CONSOLE_TAB_STATUS_LABEL "</a><a class=\"%s\" href=\"" WS63_CONSOLE_TAB_NODES_HREF "\">"
        WS63_CONSOLE_TAB_NODES_LABEL "</a><a class=\"%s\" href=\"" WS63_CONSOLE_TAB_EVENTS_HREF "\">"
        WS63_CONSOLE_TAB_EVENTS_LABEL "</a><a class=\"%s\" href=\"/pairing\">pairing</a><a href=\"" WS63_CONSOLE_TAB_JSON_HREF "\">"
        WS63_CONSOLE_TAB_JSON_LABEL "</a></div>",
        strcmp(active, "status") == 0 ? "on" : "",
        strcmp(active, "nodes") == 0 ? "on" : "",
        strcmp(active, "events") == 0 ? "on" : "",
        strcmp(active, "pairing") == 0 ? "on" : "");
    team_http_append_fmt(buf, buf_size, used, "<div class=\"tag\">page=%s " WS63_CONSOLE_BOARD_VERSION "</div>",
        active);
}

static void team_http_append_html_end(char *buf, size_t buf_size, size_t *used)
{
    team_http_append_str(buf, buf_size, used, "</main></body></html>");
}

static void team_http_send_status_page(int fd)
{
    size_t used;

    team_http_append_html_shell_start(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used, "status");
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used, "<section class=\"card\">");
    if (g_team_rt.role_configured == 0U) {
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">State</span><span class=\"v warn\">%s</span></div>",
            g_team_rt.role_request_pending != 0U ? "starting" : "unconfigured");
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_SELF_LABEL
            "</span><span class=\"v\">%s</span></div>",
            g_team_rt.self_label[0] != '\0' ? g_team_rt.self_label : "NA");
        team_http_append_power_rows(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used);
        team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">SSID</span><span class=\"v\">");
        team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            g_team_rt.softap_ssid[0] != '\0' ? g_team_rt.softap_ssid : CONFIG_SLE_TEAM_WIFI_AP_SSID);
        team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "</span></div><div class=\"bar\"><a href=\"/pairing\">configure</a></div></section>");
        team_http_append_html_end(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used);
        team_http_send_response(fd, "200 OK", "text/html; charset=utf-8", g_team_http_html_buf);
        return;
    }
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_STATE_LABEL
        "</span><span class=\"v ok\">%s</span></div>",
        sle_team_web_state_name((uint8_t)g_team_node.state));
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_ROLE_LABEL
        "</span><span class=\"v\">%s</span></div>",
        sle_team_web_role_name((uint8_t)g_team_node.cfg.role));
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_SELF_LABEL
        "</span><span class=\"v\">%s</span></div>",
        g_team_rt.self_label[0] != '\0' ? g_team_rt.self_label : "NA");
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_LEADER_LABEL
        "</span><span class=\"v\">%s</span></div>",
        g_team_rt.leader_label[0] != '\0' ? g_team_rt.leader_label : "NA");
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_JOINED_LABEL
        "</span><span class=\"v\">%s</span></div>",
        g_team_node.joined != 0U ? "true" : "false");
    team_http_append_power_rows(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used);
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">Relay Allowed</span><span class=\"v\">%s</span></div>",
        g_team_node.cfg.relay_allowed != 0U ? "true" : "false");
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">Relay Enabled</span><span class=\"v\">%s</span></div>",
        g_team_node.cfg.relay_enabled != 0U ? "true" : "false");
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Relay Target</span><span class=\"v\">%u</span></div>",
            g_team_rt.relay_target_count);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Relay Online</span><span class=\"v\">%u</span></div>",
            g_team_rt.relay_online_count);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Relay Budget</span><span class=\"v\">%u</span></div>",
            g_team_rt.relay_budget_count);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Converged</span><span class=\"v %s\">%s</span></div>",
            g_team_rt.route_metrics_converged != 0U ? "ok" : "warn",
            g_team_rt.route_metrics_converged != 0U ? "true" : "false");
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Active</span><span class=\"v\">%u</span></div>",
            g_team_rt.route_metrics_active);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Direct</span><span class=\"v\">%u</span></div>",
            g_team_rt.route_metrics_direct);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Relayed</span><span class=\"v\">%u</span></div>",
            g_team_rt.route_metrics_relayed);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Unreachable</span><span class=\"v\">%u</span></div>",
            g_team_rt.route_metrics_unreachable);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Stale</span><span class=\"v\">%u</span></div>",
            g_team_rt.route_metrics_stale);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Epoch</span><span class=\"v\">%lu</span></div>",
            (unsigned long)g_team_rt.route_metrics_epoch);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Last Change</span><span class=\"v\">%lus</span></div>",
            (unsigned long)g_team_rt.route_metrics_last_change_s);
        if (g_team_rt.route_metrics_last_converged_s == 0U) {
            team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
                "<div class=\"row\"><span class=\"k\">Route Last Converged</span><span class=\"v\">N/A</span></div>");
        } else {
            team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
                "<div class=\"row\"><span class=\"k\">Route Last Converged</span><span class=\"v\">%lus</span></div>",
                (unsigned long)g_team_rt.route_metrics_last_converged_s);
        }
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Hint Sent</span><span class=\"v\">%lu</span></div>",
            (unsigned long)g_team_rt.route_hint_sent_total);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Hint Failed</span><span class=\"v\">%lu</span></div>",
            (unsigned long)g_team_rt.route_hint_failed_total);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Hint Cooldown Skip</span><span class=\"v\">%lu</span></div>",
            (unsigned long)g_team_rt.route_hint_cooldown_skipped_total);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Update RX Total</span><span class=\"v\">%lu</span></div>",
            (unsigned long)g_team_rt.route_update_rx_total);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Route Reparent Total</span><span class=\"v\">%lu</span></div>",
            (unsigned long)g_team_rt.route_reparent_total);
        if (g_team_rt.route_reparent_last_s == 0U) {
            team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
                "<div class=\"row\"><span class=\"k\">Route Reparent Last</span><span class=\"v\">N/A</span></div>");
        } else {
            team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
                "<div class=\"row\"><span class=\"k\">Route Reparent Last</span><span class=\"v\">%lus</span></div>",
                (unsigned long)g_team_rt.route_reparent_last_s);
        }
    }
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">Relay Tier</span><span class=\"v\">%u</span></div>",
        g_team_node.cfg.relay_tier);
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">Max Downstream</span><span class=\"v\">%u</span></div>",
        g_team_node.cfg.max_downstream);
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">Upstream Parent</span><span class=\"v\">%u</span></div>",
        g_team_node.upstream_parent_id);
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">Parent State</span><span class=\"v\">%s</span></div>",
        sle_team_web_parent_state_name((uint8_t)g_team_node.upstream_parent_state));
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">Parent Reselect Pending</span><span class=\"v\">%s</span></div>",
        g_team_node.upstream_parent_reselect_pending != 0U ? "true" : "false");
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_SEQ_LABEL
        "</span><span class=\"v\">%u</span></div>",
        g_team_node.next_seq);
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_UPTIME_LABEL
        "</span><span class=\"v\">%lus</span></div>",
        (unsigned long)team_now_s(NULL));
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_TRANSPORT_LABEL
        "</span><span class=\"v\">ws63-softap</span></div>"
        "<div class=\"row\"><span class=\"k\">SSID</span><span class=\"v\">");
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        g_team_rt.softap_ssid[0] != '\0' ? g_team_rt.softap_ssid : CONFIG_SLE_TEAM_WIFI_AP_SSID);
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "</span></div>"
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_LINK_LABEL
        "</span><span class=\"v ok\">ok</span></div>"
        "</section>");
    team_http_append_html_end(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used);
    team_http_send_response(fd, "200 OK", "text/html; charset=utf-8", g_team_http_html_buf);
}

static void team_http_send_nodes_page(int fd)
{
    uint8_t i;
    uint8_t wrote = 0U;
    size_t used;
    char node_label[8];

    team_http_append_html_shell_start(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used, "nodes");
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<section class=\"card\"><h2 style=\"font-size:16px;margin:0 0 10px\">Nodes</h2>");
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];
        if (member->online == 0U) {
            continue;
        }
        wrote = 1U;
        team_identity_format_route_label(member->member_id, member->role, member->mac, member->mac_ready,
            node_label, sizeof(node_label));
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_NODE_NODE_LABEL
            "</span><span class=\"v\">%s</span></div>",
            node_label);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_NODE_BATTERY_LABEL
            "</span><span class=\"v\">%u%%</span></div>",
            member->battery_percent);
        if (member->last_rssi_dbm == SLE_TEAM_RSSI_UNKNOWN) {
            team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
                "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_NODE_RSSI_LABEL
                "</span><span class=\"v\">NA</span></div>");
        } else {
            team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
                "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_NODE_RSSI_LABEL
                "</span><span class=\"v\">%d dBm</span></div>",
                member->last_rssi_dbm);
        }
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_NODE_SEQ_LABEL
            "</span><span class=\"v\">%u</span></div>",
            member->last_seq);
    }
    if (wrote == 0U) {
        team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<pre>[]\n" WS63_CONSOLE_EMPTY_NODES "</pre>");
    }
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used, "</section>");
    team_http_append_html_end(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used);
    team_http_send_response(fd, "200 OK", "text/html; charset=utf-8", g_team_http_html_buf);
}

static void team_http_send_events_page(int fd)
{
    uint8_t i;
    size_t used;
    char src_label[8];
    char dst_label[8];
    const char *direction;

    team_http_append_html_shell_start(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used, "events");
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<section class=\"card\"><h2 style=\"font-size:16px;margin:0 0 10px\">Events</h2>");
    if (g_team_events.count == 0U) {
        team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<pre>[]\n" WS63_CONSOLE_EMPTY_EVENTS "</pre>");
    }
    for (i = 0U; i < g_team_events.count; i++) {
        uint8_t index = (uint8_t)((g_team_events.head + SLE_TEAM_WEB_EVENT_COUNT - 1U - i) %
            SLE_TEAM_WEB_EVENT_COUNT);
        const sle_team_web_event_t *event = &g_team_events.events[index];
        team_identity_format_route_label(event->src_id,
            event->src_id == g_team_node.cfg.leader_id ? (uint8_t)SLE_TEAM_ROLE_LEADER : (uint8_t)SLE_TEAM_ROLE_MEMBER,
            NULL, 0U, src_label, sizeof(src_label));
        team_identity_format_route_label(event->dst_id,
            event->dst_id == g_team_node.cfg.leader_id ? (uint8_t)SLE_TEAM_ROLE_LEADER : (uint8_t)SLE_TEAM_ROLE_MEMBER,
            NULL, 0U, dst_label, sizeof(dst_label));
        direction = event->direction == SLE_TEAM_WEB_EVENT_RX ? "rx" :
            (event->direction == SLE_TEAM_WEB_EVENT_TX ? "tx" : "system");
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">%lu %s %s</span><span class=\"v\">%s-%s #%u</span></div>",
            (unsigned long)event->time_s, direction, sle_team_web_msg_type_name(event->app_msg_type),
            src_label, dst_label, event->seq);
        if (event->direction == SLE_TEAM_WEB_EVENT_SYSTEM && event->summary[0] != '\0') {
            team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
                "<div class=\"row\"><span class=\"k\">summary</span><span class=\"v\">%s</span></div>",
                event->summary);
        }
    }
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used, "</section>");
    team_http_append_html_end(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used);
    team_http_send_response(fd, "200 OK", "text/html; charset=utf-8", g_team_http_html_buf);
}

static void team_http_send_pairing_page(int fd)
{
    uint8_t i;
    uint8_t wrote = 0U;
    size_t used;
    char node_label[8];

    team_http_append_html_shell_start(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used, "pairing");
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<section class=\"card\"><h2 style=\"font-size:16px;margin:0 0 10px\">Pairing</h2>");
    if (g_team_rt.role_configured == 0U) {
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Device</span><span class=\"v\">%s</span></div>",
            g_team_rt.self_label[0] != '\0' ? g_team_rt.self_label : "NA");
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Default team/channel</span><span class=\"v\">%u / %u</span></div>",
            team_cfg_default_team(), team_cfg_default_channel());
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">State</span><span class=\"v %s\">%s</span></div>",
            g_team_rt.role_request_pending != 0U ? "warn" : "ok",
            g_team_rt.role_request_pending != 0U ? "starting SLE" : "ready");
        team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"tag\">Choose this board role after boot. Config API saves NV; now=1 also starts SLE immediately.</div>"
            "<form action=\"/api/config/leader\" method=\"get\">"
            "<div class=\"tag\">Leader one-click config.</div>"
            "<label>team</label>"
            "<input name=\"team\" type=\"number\" min=\"1\" max=\"254\" value=\"1\">"
            "<label>channel</label>"
            "<input name=\"channel\" type=\"number\" min=\"0\" max=\"255\" value=\"17\">"
            "<label>now</label>"
            "<input name=\"now\" type=\"number\" min=\"0\" max=\"1\" value=\"1\">"
            "<button type=\"submit\">save leader</button></form>"
            "<form action=\"/api/config/member\" method=\"get\">"
            "<div class=\"tag\">Member one-click config: fill leader MAC suffix, team and channel.</div>"
            "<label>team</label>"
            "<input name=\"team\" type=\"number\" min=\"1\" max=\"254\" value=\"1\">"
            "<label>leader suffix</label>"
            "<input name=\"leader\" type=\"text\" maxlength=\"4\" placeholder=\"279A\" value=\"\">"
            "<label>channel</label>"
            "<input name=\"channel\" type=\"number\" min=\"0\" max=\"255\" value=\"17\">"
            "<label>now</label>"
            "<input name=\"now\" type=\"number\" min=\"0\" max=\"1\" value=\"1\">"
            "<button type=\"submit\">save member</button></form>"
            "<div class=\"tag\">Use the leader MAC suffix shown after L, for example A3F7.</div>"
            "<div class=\"bar\"><a href=\"/api/config/status\">config json</a>"
            "<a href=\"/api/config/apply\">apply saved</a>"
            "<a href=\"/api/config/clear\">clear saved</a>"
            "<a href=\"/api/config/reboot\">reboot</a>"
            "<a href=\"/api/factory-reset\">factory reset</a></div>"
            "</section>");
        team_http_append_html_end(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used);
        team_http_send_response(fd, "200 OK", "text/html; charset=utf-8", g_team_http_html_buf);
        return;
    }
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Window</span><span class=\"v %s\">%s</span></div>",
            g_team_node.cfg.pairing_enabled != 0U ? "ok" : "warn",
            g_team_node.cfg.pairing_enabled != 0U ? "open" : "closed");
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"bar\"><a href=\"/api/pairing?action=start\">start</a>"
            "<a href=\"/api/pairing?action=stop\">cancel</a>"
            "<a href=\"/api/factory-reset\">factory reset</a></div>");
        team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"tag\">Pending members send HELLO while the window is open.</div>");
        for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
            const sle_team_pending_member_t *member = &g_team_node.pending_members[i];
            if (member->active == 0U) {
                continue;
            }
            wrote = 1U;
            team_identity_format_route_label(member->member_id, member->role, member->mac, member->mac_ready,
                node_label, sizeof(node_label));
            team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
                "<div class=\"row\"><span class=\"k\">%s</span>"
                "<span class=\"v\"><a href=\"/api/pairing?action=approve&id=%u&relay=1\">approve relay</a> "
                "<a href=\"/api/pairing?action=approve&id=%u&relay=0\">approve no-relay</a></span></div>",
                node_label, member->member_id, member->member_id);
        }
        if (wrote == 0U) {
            team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
                "<pre>[]\nNo pending member yet.</pre>");
        }
    } else {
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">Team</span><span class=\"v\">%u</span></div>"
            "<div class=\"row\"><span class=\"k\">Leader</span><span class=\"v\">%s</span></div>"
            "<div class=\"row\"><span class=\"k\">Channel</span><span class=\"v\">%u</span></div>"
            "<div class=\"row\"><span class=\"k\">Joined</span><span class=\"v\">%s</span></div>",
            g_team_node.cfg.team_id,
            g_team_rt.leader_label[0] != '\0' ? g_team_rt.leader_label : "NA",
            g_team_node.cfg.channel_hash,
            g_team_node.joined != 0U ? "true" : "false");
        team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"bar\"><a href=\"/api/member/leave\">leave</a></div>"
            "<form action=\"/api/member/select\" method=\"get\">"
            "<input name=\"team\" type=\"number\" min=\"1\" max=\"254\" value=\"1\">"
            "<input name=\"leader\" type=\"text\" maxlength=\"4\" value=\"\">"
            "<input name=\"channel\" type=\"number\" min=\"0\" max=\"255\" value=\"17\">"
            "<button type=\"submit\">select leader</button></form>"
            "<div class=\"tag\">Member sends HELLO after selecting a leader.</div>"
            "<div class=\"bar\"><a href=\"/api/factory-reset\">factory reset</a></div>");
    }
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used, "</section>");
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<section class=\"card\"><h2 style=\"font-size:16px;margin:0 0 10px\">Phone Location</h2>"
        "<div class=\"tag\">Use phone gps once, or start auto mode for continuous POS_REPORT upload.</div>"
        "<form id=\"pairing-location-form\" action=\"/api/location\" method=\"get\">"
        "<label>lat_e6</label><input name=\"lat\" type=\"number\" value=\"0\">"
        "<label>lon_e6</label><input name=\"lon\" type=\"number\" value=\"0\">"
        "<label>dst</label><input name=\"dst\" type=\"number\" min=\"1\" max=\"255\" value=\"255\">"
        "<label>battery</label><input name=\"battery\" type=\"number\" min=\"0\" max=\"100\" value=\"88\">"
        "<label>fix</label><input name=\"fix\" type=\"number\" min=\"0\" max=\"255\" value=\"1\">"
        "<label>sat</label><input name=\"sat\" type=\"number\" min=\"0\" max=\"255\" value=\"0\">"
        "<label>speed(cm/s)</label><input name=\"speed\" type=\"number\" min=\"0\" max=\"65535\" value=\"0\">"
        "<label>heading</label><input name=\"heading\" type=\"number\" min=\"0\" max=\"65535\" value=\"0\">"
        "<button type=\"submit\">send location</button>"
        "<button id=\"pairing-location-usegps\" type=\"button\">use phone gps</button>"
        "<button id=\"pairing-location-auto\" type=\"button\" data-run=\"0\">start auto</button>"
        "</form>"
        "<pre id=\"pairing-location-result\" style=\"margin-top:8px\">idle</pre>"
        "<script>(function(){"
        "var form=document.getElementById('pairing-location-form');"
        "var result=document.getElementById('pairing-location-result');"
        "var useGps=document.getElementById('pairing-location-usegps');"
        "var autoBtn=document.getElementById('pairing-location-auto');"
        "if(!form||!result||!useGps||!autoBtn){return;}"
        "if(!navigator.geolocation){result.textContent='geolocation not supported';return;}"
        "var lat=form.elements.namedItem('lat');"
        "var lon=form.elements.namedItem('lon');"
        "var speed=form.elements.namedItem('speed');"
        "var heading=form.elements.namedItem('heading');"
        "var sat=form.elements.namedItem('sat');"
        "var fix=form.elements.namedItem('fix');"
        "var watchId=null;"
        "var lastSendMs=0;"
        "var minGapMs=2500;"
        "function fillFromCoords(c){"
        "if(!c){return;}"
        "if(lat){lat.value=String(Math.round(c.latitude*1000000));}"
        "if(lon){lon.value=String(Math.round(c.longitude*1000000));}"
        "if(speed){"
        "var s=(typeof c.speed==='number'&&!Number.isNaN(c.speed)&&c.speed>0)?Math.round(c.speed*100):0;"
        "speed.value=String(s);"
        "}"
        "if(heading){"
        "var h=(typeof c.heading==='number'&&!Number.isNaN(c.heading)&&c.heading>=0)?Math.round(c.heading):0;"
        "heading.value=String(h);"
        "}"
        "if(sat&&(!sat.value||sat.value==='0')){sat.value='0';}"
        "if(fix){fix.value='1';}"
        "}"
        "function sendNow(fromAuto){"
        "var query=new URLSearchParams(new FormData(form)).toString();"
        "result.textContent=(fromAuto?'auto ':'manual ')+'sending...';"
        "return fetch('/api/location?'+query,{cache:'no-store'})"
        ".then(function(resp){return resp.text().then(function(t){result.textContent=t;return resp.ok;});})"
        ".catch(function(err){result.textContent='send error: '+err;return false;});"
        "}"
        "form.addEventListener('submit',function(ev){ev.preventDefault();sendNow(false);});"
        "useGps.addEventListener('click',function(){"
        "result.textContent='gps locating...';"
        "navigator.geolocation.getCurrentPosition(function(pos){fillFromCoords(pos.coords);sendNow(false);},"
        "function(err){result.textContent='gps error: '+(err&&err.message?err.message:'unknown');},"
        "{enableHighAccuracy:true,timeout:12000,maximumAge:0});"
        "});"
        "autoBtn.addEventListener('click',function(){"
        "if(watchId!==null){"
        "navigator.geolocation.clearWatch(watchId);"
        "watchId=null;"
        "autoBtn.textContent='start auto';"
        "autoBtn.dataset.run='0';"
        "result.textContent='auto location stopped';"
        "return;"
        "}"
        "result.textContent='auto location starting...';"
        "watchId=navigator.geolocation.watchPosition(function(pos){"
        "var now=Date.now();"
        "fillFromCoords(pos.coords);"
        "if(now-lastSendMs<minGapMs){return;}"
        "lastSendMs=now;"
        "sendNow(true);"
        "},function(err){"
        "result.textContent='auto gps error: '+(err&&err.message?err.message:'unknown');"
        "},"
        "{enableHighAccuracy:true,timeout:12000,maximumAge:1000});"
        "autoBtn.textContent='stop auto';"
        "autoBtn.dataset.run='1';"
        "});"
        "})();</script>"
        "</section>");
    team_http_append_html_end(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used);
    team_http_send_response(fd, "200 OK", "text/html; charset=utf-8", g_team_http_html_buf);
}

static void team_http_handle_client(int fd)
{
    int ret;
    char path[SLE_TEAM_HTTP_PATH_BUF_SIZE];

    ret = team_http_recv_request_line(fd);
    if (ret <= 0) {
        return;
    }
    team_http_get_path(path, sizeof(path));
    if (path[0] == '\0') {
        osal_printk("[team-wifi] http bad request line len=%d\r\n", ret);
        team_http_send_response(fd, "400 Bad Request", "text/plain", "bad request");
        return;
    }
    osal_printk("[team-wifi] http recv fd=%d len=%d path=%s\r\n", fd, ret, path);

    if (strncmp(g_team_http_req_buf, "GET /api/status", 15) == 0) {
        osal_printk("[team-wifi] http route api=status\r\n");
        team_http_send_status_json_response(fd);
    } else if (strncmp(g_team_http_req_buf, "GET /api/power", 14) == 0) {
        osal_printk("[team-wifi] http route api=power\r\n");
        team_http_send_power_json_response(fd);
    } else if (strncmp(g_team_http_req_buf, "GET /api/nodes", 14) == 0) {
        osal_printk("[team-wifi] http route api=nodes\r\n");
        ret = sle_team_web_write_nodes_json(&g_team_node, g_team_http_json_buf, sizeof(g_team_http_json_buf));
        team_http_send_response(fd, ret < 0 ? "500 Internal Server Error" : "200 OK", "application/json",
            ret < 0 ? "{\"error\":\"nodes\"}" : g_team_http_json_buf);
    } else if (strncmp(g_team_http_req_buf, "GET /api/location", 17) == 0) {
        sle_team_pos_body_t pos;
        int send_ret;
        int32_t lat = 0;
        int32_t lon = 0;
        uint16_t speed = 0U;
        uint16_t heading = 0U;
        uint8_t battery = team_battery_percent_cb(NULL);
        uint8_t fix = 1U;
        uint8_t sat = 0U;
        uint8_t dst = SLE_TEAM_BROADCAST_ID;

        osal_printk("[team-wifi] http route api=location path=%s\r\n", path);
        if (g_team_rt.role_configured == 0U) {
            team_http_send_response(fd, "400 Bad Request", "application/json",
                "{\"error\":\"role not configured\"}");
            return;
        }
        if (team_http_query_i32(path, "lat", -90000000, 90000000, &lat) != 0 ||
            team_http_query_i32(path, "lon", -180000000, 180000000, &lon) != 0) {
            team_http_send_response(fd, "400 Bad Request", "application/json",
                "{\"error\":\"lat/lon required\"}");
            return;
        }
        (void)team_http_query_u16(path, "speed", 0U, 65535U, &speed);
        (void)team_http_query_u16(path, "heading", 0U, 65535U, &heading);
        (void)team_http_query_u8(path, "battery", 0U, 100U, &battery);
        (void)team_http_query_u8(path, "fix", 0U, 255U, &fix);
        (void)team_http_query_u8(path, "sat", 0U, 255U, &sat);
        (void)team_http_query_u8(path, "dst", 1U, 255U, &dst);

        (void)memset_s(&pos, sizeof(pos), 0, sizeof(pos));
        pos.latitude_e6 = lat;
        pos.longitude_e6 = lon;
        pos.speed_cms = speed;
        pos.heading_deg = heading;
        pos.battery_percent = battery;
        pos.fix_status = fix;
        pos.sat_count = sat;

        send_ret = sle_team_node_send_position(&g_team_node, dst, &pos);
        team_http_send_location_event(fd, dst, send_ret, lat, lon);
    } else if (strncmp(g_team_http_req_buf, "GET /api/pending", 16) == 0) {
        osal_printk("[team-wifi] http route api=pending\r\n");
        ret = sle_team_web_write_pending_json(&g_team_node, g_team_http_json_buf, sizeof(g_team_http_json_buf));
        team_http_send_response(fd, ret < 0 ? "500 Internal Server Error" : "200 OK", "application/json",
            ret < 0 ? "{\"error\":\"pending\"}" : g_team_http_json_buf);
    } else if (strncmp(g_team_http_req_buf, "GET /api/config/status", 22) == 0) {
        osal_printk("[team-wifi] http route api=config_status\r\n");
        team_http_send_config_status_json(fd);
    } else if (strncmp(g_team_http_req_buf, "GET /api/config/leader", 22) == 0) {
        uint8_t team = team_cfg_default_team();
        uint8_t channel = team_cfg_default_channel();
        uint8_t now = 0U;
        osal_printk("[team-wifi] http route api=config_leader path=%s\r\n", path);
        (void)team_http_query_u8(path, "team", 1U, 254U, &team);
        (void)team_http_query_u8(path, "channel", 0U, 255U, &channel);
        (void)team_http_query_u8(path, "now", 0U, 1U, &now);
        ret = team_cfg_save_leader(team, channel, now);
        osal_printk("[team-wifi] config leader ret=%d team=%u channel=%u now=%u\r\n",
            ret, team, channel, now);
        team_http_send_config_action_json(fd, now != 0U ? "leader-now" : "leader", ret);
    } else if (strncmp(g_team_http_req_buf, "GET /api/config/member", 22) == 0) {
        uint8_t team = team_cfg_default_team();
        uint8_t channel = team_cfg_default_channel();
        uint8_t now = 0U;
        uint16_t leader_suffix = 0U;
        osal_printk("[team-wifi] http route api=config_member path=%s\r\n", path);
        if (team_http_query_hex16(path, "leader", &leader_suffix) != 0 || leader_suffix == 0U) {
            team_http_send_config_bad_request(fd, "leader suffix required");
            return;
        }
        (void)team_http_query_u8(path, "team", 1U, 254U, &team);
        (void)team_http_query_u8(path, "channel", 0U, 255U, &channel);
        (void)team_http_query_u8(path, "now", 0U, 1U, &now);
        ret = team_cfg_save_member(leader_suffix, team, channel, now);
        osal_printk("[team-wifi] config member ret=%d leader_suffix=%04X team=%u channel=%u now=%u\r\n",
            ret, leader_suffix, team, channel, now);
        team_http_send_config_action_json(fd, now != 0U ? "member-now" : "member", ret);
    } else if (strncmp(g_team_http_req_buf, "GET /api/config/apply", 21) == 0) {
        osal_printk("[team-wifi] http route api=config_apply\r\n");
        ret = team_cfg_apply_saved();
        osal_printk("[team-wifi] config apply ret=%d\r\n", ret);
        team_http_send_config_action_json(fd, "apply", ret);
    } else if (strncmp(g_team_http_req_buf, "GET /api/config/clear", 21) == 0) {
        osal_printk("[team-wifi] http route api=config_clear\r\n");
        ret = team_nv_config_clear();
        team_http_send_config_action_json(fd, "clear", ret);
    } else if (strncmp(g_team_http_req_buf, "GET /api/config/reboot", 22) == 0) {
        osal_printk("[team-wifi] http route api=config_reboot\r\n");
        team_reboot_schedule("config-api");
        team_http_send_config_action_json(fd, "reboot", SLE_TEAM_OK);
    } else if (strncmp(g_team_http_req_buf, "GET /api/role", 13) == 0) {
        uint8_t team = CONFIG_SLE_TEAM_TEAM_ID;
        uint8_t channel = CONFIG_SLE_TEAM_CHANNEL_HASH;
        uint16_t leader_suffix = 0U;
        uint8_t leader = CONFIG_SLE_TEAM_LEADER_ID;
        osal_printk("[team-wifi] http route api=role path=%s\r\n", path);
        if (strstr(path, "role=leader") != NULL) {
            ret = team_request_role_config(SLE_TEAM_ROLE_LEADER, g_team_rt.route_id, team_cfg_default_team(),
                team_cfg_default_channel(), team_self_mac_suffix(), 1U);
            osal_printk("[team-wifi] set role leader queued ret=%d\r\n", ret);
            team_http_send_redirect(fd, "/pairing");
        } else if (strstr(path, "role=member") != NULL &&
            team_http_query_hex16(path, "leader", &leader_suffix) == 0) {
            (void)team_http_query_u8(path, "team", 1U, 254U, &team);
            (void)team_http_query_u8(path, "channel", 0U, 255U, &channel);
            leader = team_route_id_from_suffix(leader_suffix);
            ret = team_request_role_config(SLE_TEAM_ROLE_MEMBER, leader, team, channel, leader_suffix, 1U);
            osal_printk("[team-wifi] set role member queued leader_suffix=%04X leader=%u ret=%d\r\n",
                leader_suffix, leader, ret);
            team_http_send_redirect(fd, "/pairing");
        } else {
            osal_printk("[team-wifi] set role bad request\r\n");
            team_http_send_redirect(fd, "/pairing");
        }
    } else if (strncmp(g_team_http_req_buf, "GET /api/pairing", 16) == 0) {
        uint8_t id = 0U;
        uint8_t relay_allowed = 0U;
        osal_printk("[team-wifi] http route api=pairing path=%s\r\n", path);
        if (g_team_rt.role_configured == 0U || g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
            osal_printk("[team-wifi] pairing ignored role_configured=%u role=%u\r\n",
                g_team_rt.role_configured, g_team_node.cfg.role);
            team_http_send_redirect(fd, "/pairing");
            return;
        }
        if (strstr(path, "action=start") != NULL) {
            ret = sle_team_node_pairing_start(&g_team_node);
            if (ret == SLE_TEAM_OK) {
                team_leader_pairing_restart_scan("http");
            }
            osal_printk("[team-wifi] pairing start ret=%d\r\n", ret);
            team_http_send_redirect(fd, "/pairing");
        } else if (strstr(path, "action=stop") != NULL) {
            team_leader_auto_approve_pending();
            ret = sle_team_node_pairing_stop(&g_team_node);
            if (ret == SLE_TEAM_OK) {
                (void)team_nv_allowed_save_from_node();
            }
            g_team_rt.pairing_rotate_last_s = 0U;
            osal_printk("[team-wifi] pairing stop ret=%d\r\n", ret);
            team_http_send_redirect(fd, "/pairing");
        } else if (strstr(path, "action=approve") != NULL &&
            team_http_query_u8(path, "id", 1U, 254U, &id) == 0) {
            if (team_http_query_u8(path, "relay", 0U, 1U, &relay_allowed) != 0) {
                relay_allowed = 0U;
            }
            ret = sle_team_node_pairing_approve_with_relay(&g_team_node, id, relay_allowed);
            if (ret == SLE_TEAM_OK) {
                (void)team_nv_allowed_save_from_node();
            }
            osal_printk("[team-wifi] pairing approve id=%u relay=%u ret=%d\r\n", id, relay_allowed, ret);
            team_http_send_redirect(fd, "/pairing");
        } else {
            osal_printk("[team-wifi] pairing bad request\r\n");
            team_http_send_redirect(fd, "/pairing");
        }
    } else if (strncmp(g_team_http_req_buf, "GET /api/member/select", 22) == 0) {
        uint8_t team = 0U;
        uint8_t leader = 0U;
        uint16_t leader_suffix = 0U;
        uint8_t channel = 0U;
        osal_printk("[team-wifi] http route api=member_select path=%s\r\n", path);
        if (g_team_rt.role_configured == 0U || g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
            osal_printk("[team-wifi] member select ignored role_configured=%u role=%u\r\n",
                g_team_rt.role_configured, g_team_node.cfg.role);
            team_http_send_redirect(fd, "/pairing");
            return;
        }
        if (team_http_query_u8(path, "team", 1U, 254U, &team) == 0 &&
            team_http_query_u8(path, "channel", 0U, 255U, &channel) == 0) {
            if (team_http_query_hex16(path, "leader", &leader_suffix) == 0) {
                leader = team_route_id_from_suffix(leader_suffix);
                (void)snprintf(g_team_rt.leader_label, sizeof(g_team_rt.leader_label), "L%04X", leader_suffix);
            } else if (team_http_query_u8(path, "leader", 1U, 254U, &leader) != 0) {
                osal_printk("[team-wifi] member select bad leader\r\n");
                team_http_send_redirect(fd, "/pairing");
                return;
            }
            ret = sle_team_node_member_select_leader(&g_team_node, team, leader, channel);
            if (ret == SLE_TEAM_OK && leader_suffix != 0U) {
                (void)team_nv_config_save(SLE_TEAM_ROLE_MEMBER, team, leader_suffix, channel);
            }
            osal_printk("[team-wifi] member select team=%u leader=%u channel=%u ret=%d\r\n",
                team, leader, channel, ret);
            team_http_send_redirect(fd, "/pairing");
        } else {
            osal_printk("[team-wifi] member select bad request\r\n");
            team_http_send_redirect(fd, "/pairing");
        }
    } else if (strncmp(g_team_http_req_buf, "GET /api/member/leave", 21) == 0) {
        osal_printk("[team-wifi] http route api=member_leave\r\n");
        if (g_team_rt.role_configured == 0U || g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
            osal_printk("[team-wifi] member leave ignored role_configured=%u role=%u\r\n",
                g_team_rt.role_configured, g_team_node.cfg.role);
            team_http_send_redirect(fd, "/pairing");
            return;
        }
        ret = sle_team_node_member_leave(&g_team_node);
        if (ret == SLE_TEAM_OK) {
            team_member_reset_after_leave();
        }
        osal_printk("[team-wifi] member leave ret=%d\r\n", ret);
        team_http_send_redirect(fd, "/pairing");
    } else if (strncmp(g_team_http_req_buf, "GET /api/factory-reset", 22) == 0) {
        osal_printk("[team-wifi] http route api=factory_reset\r\n");
        ret = team_cfg_clear_all_saved();
        if (ret == SLE_TEAM_OK) {
            team_http_send_factory_reset_page(fd);
            team_factory_reset_schedule();
        } else {
            team_http_send_factory_reset_failed_page(fd);
        }
    } else if (strncmp(g_team_http_req_buf, "GET /api/events", 15) == 0) {
        osal_printk("[team-wifi] http route api=events\r\n");
        ret = sle_team_web_write_events_json(&g_team_events, g_team_http_json_buf, sizeof(g_team_http_json_buf));
        team_http_send_response(fd, ret < 0 ? "500 Internal Server Error" : "200 OK", "application/json",
            ret < 0 ? "{\"error\":\"events\"}" : g_team_http_json_buf);
    } else if (strncmp(g_team_http_req_buf, "GET /nodes", 10) == 0) {
        osal_printk("[team-wifi] http route page=nodes\r\n");
        team_http_send_nodes_page(fd);
    } else if (strncmp(g_team_http_req_buf, "GET /events", 11) == 0) {
        osal_printk("[team-wifi] http route page=events\r\n");
        team_http_send_events_page(fd);
    } else if (strncmp(g_team_http_req_buf, "GET /pairing", 12) == 0) {
        osal_printk("[team-wifi] http route page=pairing\r\n");
        team_http_send_pairing_page(fd);
    } else if (strncmp(g_team_http_req_buf, "GET /favicon.ico", 16) == 0) {
        osal_printk("[team-wifi] http route favicon\r\n");
        team_http_send_response(fd, "204 No Content", "text/plain", "");
    } else {
        osal_printk("[team-wifi] http route page=status\r\n");
        team_http_send_status_page(fd);
    }
}

static void team_http_server_loop(void)
{
    int listen_fd;
    int client_fd;
    int opt = 1;
    struct sockaddr_in addr = {0};
    struct sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) {
            g_team_http_last_errno = errno;
            osal_printk("[team-wifi] http socket failed fd=%d errno=%d, retrying\r\n", listen_fd,
                g_team_http_last_errno);
            osal_msleep(SLE_TEAM_HTTP_RETRY_MS);
            continue;
        }
        g_team_http_listen_fd = listen_fd;

        (void)setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = PP_HTONL(INADDR_ANY);
        addr.sin_port = htons(SLE_TEAM_HTTP_PORT);

        if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            g_team_http_last_errno = errno;
            osal_printk("[team-wifi] http bind failed errno=%d, retrying\r\n", g_team_http_last_errno);
            closesocket(listen_fd);
            g_team_http_listen_fd = -1;
            osal_msleep(SLE_TEAM_HTTP_RETRY_MS);
            continue;
        }
        if (listen(listen_fd, SLE_TEAM_HTTP_BACKLOG) != 0) {
            g_team_http_last_errno = errno;
            osal_printk("[team-wifi] http listen failed errno=%d, retrying\r\n", g_team_http_last_errno);
            closesocket(listen_fd);
            g_team_http_listen_fd = -1;
            osal_msleep(SLE_TEAM_HTTP_RETRY_MS);
            continue;
        }
        break;
    }

    g_team_http_ready = 1;
    osal_printk("[team-wifi] http server ready port=%u\r\n", SLE_TEAM_HTTP_PORT);
    while (1) {
        client_len = sizeof(client_addr);
        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            osal_msleep(100);
            continue;
        }
        g_team_http_accept_count++;
        team_http_set_client_timeouts(client_fd);
        team_http_handle_client(client_fd);
        osal_msleep(20);
        (void)shutdown(client_fd, SHUT_RDWR);
        closesocket(client_fd);
    }
}

static void *team_wifi_ap_task(const char *arg)
{
    errcode_t ret;
    uint32_t wait_ms = 0;

    unused(arg);

    team_wifi_print("task started");
    if (wifi_is_wifi_inited() == 0) {
        ret = wifi_init();
        osal_printk("[team-wifi] wifi_init ret=0x%x\r\n", ret);
    }

    while (wifi_is_wifi_inited() == 0) {
        osal_msleep(100);
        wait_ms += 100;
        if (wait_ms >= SLE_TEAM_WIFI_INIT_WAIT_MAX_MS) {
            team_wifi_print("wifi init timeout");
            return NULL;
        }
    }
    team_wifi_print("wifi init ready");
    team_identity_init_from_wifi_mac();
#if !CONFIG_SLE_TEAM_WIFI_AP_AUTO_START
    team_wifi_print("softap auto-start disabled for SLE coexistence test");
    return NULL;
#endif
    if (team_tcpip_init_wait() != 0) {
        return NULL;
    }
    while (wifi_is_softap_enabled() == 0) {
        if (team_wifi_ap_start() == 0) {
            break;
        }
        team_wifi_print("softap start failed, retrying");
        team_wifi_print_status();
        osal_msleep(SLE_TEAM_WIFI_AP_RETRY_MS);
    }
    osal_msleep(SLE_TEAM_HTTP_START_DELAY_MS);
    team_http_server_loop();
    return NULL;
}

static void team_wifi_ap_entry(void)
{
    osal_task *task = NULL;

    osal_kthread_lock();
    task = osal_kthread_create((osal_kthread_handler)team_wifi_ap_task, NULL, "TeamWifiApTask",
        SLE_TEAM_WIFI_AP_TASK_STACK_SIZE);
    if (task != NULL) {
        osal_kthread_set_priority(task, SLE_TEAM_WIFI_AP_TASK_PRIO);
    }
    osal_kthread_unlock();
}
#endif

static void team_uart_pins_init(void)
{
    uapi_pin_set_mode(CONFIG_SLE_TEAM_UART_TXD_PIN, PIN_MODE_1);
    uapi_pin_set_mode(CONFIG_SLE_TEAM_UART_RXD_PIN, PIN_MODE_1);
}

static void team_uart_init(void)
{
    uart_attr_t attr = {
        .baud_rate = SLE_TEAM_UART_BAUDRATE,
        .data_bits = UART_DATA_BIT_8,
        .stop_bits = UART_STOP_BIT_1,
        .parity = UART_PARITY_NONE,
    };
    uart_pin_config_t pin_config = {
        .tx_pin = CONFIG_SLE_TEAM_UART_TXD_PIN,
        .rx_pin = CONFIG_SLE_TEAM_UART_RXD_PIN,
        .cts_pin = PIN_NONE,
        .rts_pin = PIN_NONE,
    };

    team_uart_pins_init();
    uapi_uart_deinit(CONFIG_SLE_TEAM_UART_BUS);
    uapi_uart_init(CONFIG_SLE_TEAM_UART_BUS, &pin_config, &attr, NULL, &g_uart_buffer_config);
}

static void team_cli_enqueue_line(const char *line)
{
    sle_team_cli_msg_t msg;

    if (g_team_rt.cli_queue_ready == 0U || line == NULL) {
        return;
    }

    (void)memset_s(&msg, sizeof(msg), 0, sizeof(msg));
    (void)snprintf(msg.line, sizeof(msg.line), "%s", line);
    (void)osal_msg_queue_write_copy(g_team_rt.cli_queue_id, &msg, (uint32_t)sizeof(msg), 0);
}

static void team_uart_rx_cb(const void *buffer, uint16_t length, bool error)
{
    const uint8_t *data = (const uint8_t *)buffer;
    uint16_t i;

    if (error || data == NULL) {
        return;
    }

    for (i = 0; i < length; i++) {
        char ch = (char)data[i];
        if (ch == '\r' || ch == '\n') {
            if (g_team_rt.line_len > 0U) {
                g_team_rt.line_buf[g_team_rt.line_len] = '\0';
                team_cli_enqueue_line(g_team_rt.line_buf);
                g_team_rt.line_len = 0U;
            }
            continue;
        }
        if (g_team_rt.line_len + 1U < sizeof(g_team_rt.line_buf)) {
            g_team_rt.line_buf[g_team_rt.line_len++] = ch;
        } else {
            g_team_rt.line_len = 0U;
            team_print("cli line too long, dropped");
        }
    }
}

static void team_uart_cli_start(void)
{
    errcode_t ret;

    if (osal_msg_queue_create("team_cli_q", SLE_TEAM_CLI_QUEUE_LEN, &g_team_rt.cli_queue_id, 0,
        sizeof(sle_team_cli_msg_t)) == OSAL_SUCCESS) {
        g_team_rt.cli_queue_ready = 1U;
    } else {
        team_print("cli queue create failed");
    }

    uapi_uart_unregister_rx_callback(CONFIG_SLE_TEAM_UART_BUS);
    ret = uapi_uart_register_rx_callback(CONFIG_SLE_TEAM_UART_BUS, UART_RX_CONDITION_FULL_OR_IDLE, 1, team_uart_rx_cb);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[team] uart rx callback failed: 0x%x\r\n", ret);
    }
}

static uint8_t team_decode_app_packet_from_buf(const uint8_t *buf, uint16_t len, sle_team_app_packet_t *app_packet)
{
    sle_team_mesh_packet_t mesh_packet;
    const uint8_t *app_payload = NULL;
    uint16_t app_payload_len = 0U;
    uint8_t channel_hash = 0U;
    uint8_t cipher_mac[2];

    if (buf == NULL || app_packet == NULL) {
        return 0U;
    }
    if (sle_team_decode_mesh_packet(&mesh_packet, buf, len) != SLE_TEAM_OK) {
        return 0U;
    }
    if (sle_team_unwrap_mesh_group_data(&mesh_packet, &channel_hash, cipher_mac, &app_payload, &app_payload_len) !=
        SLE_TEAM_OK) {
        return 0U;
    }
    if (channel_hash != g_team_node.cfg.channel_hash) {
        return 0U;
    }
    if (sle_team_decode_app_packet(app_packet, app_payload, app_payload_len) != SLE_TEAM_OK) {
        return 0U;
    }
    return 1U;
}

static uint8_t team_member_find_upstream_parent_conn(uint16_t *conn_id)
{
    uint8_t parent_id;
    team_conn_track_t *track;
    uint8_t target_required;

    if (conn_id == NULL || g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
        return 0U;
    }
    parent_id = g_team_node.upstream_parent_id;
    target_required = team_member_has_reselect_target(parent_id);
    if (parent_id != 0U && parent_id != SLE_TEAM_BROADCAST_ID &&
        sle_uart_server_find_conn_by_member_ex(parent_id, conn_id) != 0U) {
        return 1U;
    }
    track = team_conn_track_find_by_route_id(parent_id);
    if (track != NULL && track->dir == TEAM_CONN_DIR_UPSTREAM &&
        team_route_conn_is_active(TEAM_CONN_DIR_UPSTREAM, track->conn_id) != 0U) {
        *conn_id = track->conn_id;
        return 1U;
    }
    if (target_required != 0U) {
        return 0U;
    }
    if (g_team_node.cfg.leader_id != 0U && g_team_node.cfg.leader_id != SLE_TEAM_BROADCAST_ID &&
        sle_uart_server_find_conn_by_member_ex(g_team_node.cfg.leader_id, conn_id) != 0U) {
        return 1U;
    }
    return 0U;
}

static int team_sle_send(void *user_ctx, sle_team_send_kind_t kind, uint8_t dst_id, const uint8_t *buf, uint16_t len)
{
    uint16_t target_conn_id = 0U;
    uint8_t has_target_conn = 0U;
    sle_team_app_packet_t app_packet;
    uint8_t self_bucket;
    uint8_t src_bucket;
    uint8_t send_downstream = 0U;
    uint8_t send_upstream = 0U;
    uint8_t app_decoded = 0U;
    uint8_t use_relay_transport = 0U;
    errcode_t send_ret;
    unused(user_ctx);
    unused(kind);

    if (buf == NULL || len == 0U) {
        return SLE_TEAM_ERR_ARG;
    }

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        if (sle_uart_client_is_ready() == 0U) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NO_MEMBER\r\n",
                dst_id, SLE_TEAM_ERR_UNSUPPORTED);
            return SLE_TEAM_ERR_UNSUPPORTED;
        }
        if (dst_id != SLE_TEAM_BROADCAST_ID) {
            has_target_conn = team_route_find(dst_id, TEAM_CONN_DIR_DOWNSTREAM, &target_conn_id);
            if (has_target_conn == 0U) {
                has_target_conn = sle_uart_client_find_conn_by_member(dst_id, &target_conn_id);
            }
            if (has_target_conn == 0U) {
                osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NO_ROUTE\r\n",
                    dst_id, SLE_TEAM_ERR_UNSUPPORTED);
                return SLE_TEAM_ERR_UNSUPPORTED;
            }
        }
        send_ret = (dst_id == SLE_TEAM_BROADCAST_ID) ?
            sle_uart_client_send_all(buf, len) :
            sle_uart_client_send_by_conn(target_conn_id, buf, len);
        if (send_ret != ERRCODE_SLE_SUCCESS) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=WRITE_FAIL\r\n",
                dst_id, SLE_TEAM_ERR_FORMAT);
            return SLE_TEAM_ERR_FORMAT;
        }
        team_web_record_packet(SLE_TEAM_WEB_EVENT_TX, buf, len, "leader tx");
        return SLE_TEAM_OK;
    }

    if (team_decode_app_packet_from_buf(buf, len, &app_packet) != 0U) {
        app_decoded = 1U;
    }
    use_relay_transport = team_route_is_relay_enabled();
    if (use_relay_transport == 0U && app_decoded != 0U &&
        team_route_should_bridge_relay_control(&app_packet) != 0U) {
        use_relay_transport = 1U;
        osal_printk("[team] relay control bridge src=%u dst=%u type=%u joined=%u enabled=%u state=%u\r\n",
            app_packet.src_id, app_packet.dst_id, app_packet.app_msg_type, g_team_node.joined,
            g_team_node.cfg.relay_enabled, (uint8_t)g_team_node.state);
    }

    if (use_relay_transport == 0U) {
        if (sle_uart_server_connected_count() == 0U) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NOT_READY\r\n",
                dst_id, SLE_TEAM_ERR_UNSUPPORTED);
            team_member_upstream_recover_after_tx_fail("not-ready", dst_id, SLE_TEAM_ERR_UNSUPPORTED);
            return SLE_TEAM_ERR_UNSUPPORTED;
        }
        if (dst_id != SLE_TEAM_BROADCAST_ID && dst_id != g_team_node.cfg.leader_id) {
            has_target_conn = sle_uart_server_find_conn_by_member_ex(dst_id, &target_conn_id);
            if (has_target_conn == 0U) {
                osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NO_ROUTE\r\n",
                    dst_id, SLE_TEAM_ERR_UNSUPPORTED);
                return SLE_TEAM_ERR_UNSUPPORTED;
            }
        }
        if (dst_id == g_team_node.cfg.leader_id &&
            team_member_find_upstream_parent_conn(&target_conn_id) != 0U) {
            send_ret = sle_uart_server_send_report_by_conn(target_conn_id, buf, len);
        } else if (dst_id == g_team_node.cfg.leader_id &&
            team_member_has_reselect_target(g_team_node.upstream_parent_id) != 0U) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=RESELECT_PARENT\r\n",
                dst_id, SLE_TEAM_ERR_UNSUPPORTED);
            return SLE_TEAM_ERR_UNSUPPORTED;
        } else {
            send_ret = (dst_id == SLE_TEAM_BROADCAST_ID || dst_id == g_team_node.cfg.leader_id) ?
                sle_uart_server_send_report_by_handle(buf, len) :
                sle_uart_server_send_report_by_conn(target_conn_id, buf, len);
        }
        if (send_ret != ERRCODE_SLE_SUCCESS) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=WRITE_FAIL\r\n", dst_id, SLE_TEAM_ERR_FORMAT);
            team_member_upstream_recover_after_tx_fail("write-fail", dst_id, SLE_TEAM_ERR_FORMAT);
            return SLE_TEAM_ERR_FORMAT;
        }
        team_member_upstream_recover_clear("tx");
        team_web_record_packet(SLE_TEAM_WEB_EVENT_TX, buf, len, "member tx");
        return SLE_TEAM_OK;
    }

    if (app_decoded == 0U) {
        osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=FORMAT\r\n", dst_id, SLE_TEAM_ERR_FORMAT);
        return SLE_TEAM_ERR_FORMAT;
    }

    self_bucket = team_route_bucket_for_self();
    src_bucket = team_route_bucket_from_ids(app_packet.src_id, g_team_node.cfg.leader_id);
    if (app_packet.src_id == g_team_node.cfg.self_id) {
        send_upstream = (uint8_t)(dst_id == g_team_node.cfg.leader_id || dst_id == SLE_TEAM_BROADCAST_ID);
        send_downstream = (uint8_t)(send_upstream == 0U);
    } else if (dst_id == g_team_node.cfg.leader_id) {
        /* Leader-bound relayed packets always go upstream before bucket tier routing. */
        send_upstream = 1U;
    } else if (src_bucket < self_bucket) {
        send_downstream = 1U;
    } else if (src_bucket > self_bucket) {
        send_upstream = 1U;
    } else if (dst_id == g_team_node.cfg.leader_id || dst_id == SLE_TEAM_BROADCAST_ID) {
        send_upstream = 1U;
    } else {
        send_downstream = 1U;
    }

    if (send_downstream != 0U) {
        if (g_team_rt.relay_client_started == 0U && app_packet.src_id != g_team_node.cfg.self_id) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NO_ROUTE\r\n",
                dst_id, SLE_TEAM_ERR_UNSUPPORTED);
            return SLE_TEAM_ERR_UNSUPPORTED;
        }
        if (dst_id == SLE_TEAM_BROADCAST_ID) {
            send_ret = sle_uart_client_send_all(buf, len);
            if (send_ret != ERRCODE_SLE_SUCCESS) {
                osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=WRITE_FAIL\r\n",
                    dst_id, SLE_TEAM_ERR_FORMAT);
                return SLE_TEAM_ERR_FORMAT;
            }
            team_web_record_packet(SLE_TEAM_WEB_EVENT_TX, buf, len, "relay down tx");
            return SLE_TEAM_OK;
        }
        has_target_conn = team_route_find(dst_id, TEAM_CONN_DIR_DOWNSTREAM, &target_conn_id);
        if (has_target_conn == 0U) {
            has_target_conn = sle_uart_client_find_conn_by_member(dst_id, &target_conn_id);
        }
        if (has_target_conn == 0U) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NO_ROUTE\r\n",
                dst_id, SLE_TEAM_ERR_UNSUPPORTED);
            return SLE_TEAM_ERR_UNSUPPORTED;
        }
        send_ret = sle_uart_client_send_by_conn(target_conn_id, buf, len);
        if (send_ret != ERRCODE_SLE_SUCCESS) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=WRITE_FAIL\r\n",
                dst_id, SLE_TEAM_ERR_FORMAT);
            return SLE_TEAM_ERR_FORMAT;
        }
        team_web_record_packet(SLE_TEAM_WEB_EVENT_TX, buf, len, "relay down tx");
        return SLE_TEAM_OK;
    }

    if (sle_uart_server_connected_count() == 0U) {
        osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NOT_READY\r\n",
            dst_id, SLE_TEAM_ERR_UNSUPPORTED);
        team_member_upstream_recover_after_tx_fail("not-ready", dst_id, SLE_TEAM_ERR_UNSUPPORTED);
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    if (dst_id != SLE_TEAM_BROADCAST_ID && dst_id != g_team_node.cfg.leader_id) {
        has_target_conn = team_route_find(dst_id, TEAM_CONN_DIR_UPSTREAM, &target_conn_id);
        if (has_target_conn == 0U) {
            has_target_conn = sle_uart_server_find_conn_by_member_ex(dst_id, &target_conn_id);
        }
        if (has_target_conn == 0U) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NO_ROUTE\r\n",
                dst_id, SLE_TEAM_ERR_UNSUPPORTED);
            return SLE_TEAM_ERR_UNSUPPORTED;
        }
    }
    if (dst_id == g_team_node.cfg.leader_id &&
        team_member_find_upstream_parent_conn(&target_conn_id) != 0U) {
        send_ret = sle_uart_server_send_report_by_conn(target_conn_id, buf, len);
    } else if (dst_id == g_team_node.cfg.leader_id &&
        team_member_has_reselect_target(g_team_node.upstream_parent_id) != 0U) {
        osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=RESELECT_PARENT\r\n",
            dst_id, SLE_TEAM_ERR_UNSUPPORTED);
        return SLE_TEAM_ERR_UNSUPPORTED;
    } else {
        send_ret = (dst_id == SLE_TEAM_BROADCAST_ID || dst_id == g_team_node.cfg.leader_id) ?
            sle_uart_server_send_report_by_handle(buf, len) :
            sle_uart_server_send_report_by_conn(target_conn_id, buf, len);
    }
    if (send_ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=WRITE_FAIL\r\n", dst_id, SLE_TEAM_ERR_FORMAT);
        team_member_upstream_recover_after_tx_fail("write-fail", dst_id, SLE_TEAM_ERR_FORMAT);
        return SLE_TEAM_ERR_FORMAT;
    }
    team_member_upstream_recover_clear("tx");
    team_web_record_packet(SLE_TEAM_WEB_EVENT_TX, buf, len, "relay up tx");
    return SLE_TEAM_OK;
}

static void team_bind_packet_source(uint16_t conn_id, const uint8_t *buf, uint16_t len, team_link_dir_t dir)
{
    sle_team_app_packet_t app_packet;
    team_conn_track_t *track;
    team_conn_dir_t conn_dir;
    uint8_t next_hop_id;
    uint8_t physical_peer_id;
    uint8_t route_update_can_override_next_hop;
    sle_team_route_update_body_t route_update;
    uint8_t route_update_ready = 0U;

    if (buf == NULL || len == 0U || team_decode_app_packet_from_buf(buf, len, &app_packet) == 0U) {
        return;
    }
    if (app_packet.team_id != g_team_node.cfg.team_id || app_packet.src_id == 0U ||
        app_packet.src_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    conn_dir = dir == TEAM_LINK_DOWNSTREAM ? TEAM_CONN_DIR_DOWNSTREAM : TEAM_CONN_DIR_UPSTREAM;
    track = team_conn_track_find(conn_id);
    if (g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER) {
        if (team_conn_track_route_id_is_trusted(track) == 0U) {
            team_conn_track_note_packet(conn_id, conn_dir, app_packet.src_id);
            track = team_conn_track_find(conn_id);
        }
    } else if (team_conn_track_route_id_is_valid(track) != 0U) {
        /*
         * On the leader, app_packet.src_id is the logical origin. For relayed
         * packets it is a child, not the physical first hop. Keep the seek/
         * connection owner as the physical peer and only promote provisional
         * ids when the packet source confirms the same peer.
         */
        if (track->route_id_provisional != 0U && track->route_id == app_packet.src_id) {
            team_conn_track_note_packet(conn_id, conn_dir, app_packet.src_id);
            track = team_conn_track_find(conn_id);
        }
    } else {
        team_conn_track_note_packet(conn_id, conn_dir, app_packet.src_id);
        track = team_conn_track_find(conn_id);
    }
    physical_peer_id = team_conn_track_route_id_is_valid(track) != 0U ? track->route_id : app_packet.src_id;
    next_hop_id = physical_peer_id;
    route_update_can_override_next_hop = 1U;
    if (app_packet.app_msg_type == SLE_TEAM_APP_ROUTE_UPDATE &&
        app_packet.body_len >= sizeof(route_update)) {
        (void)memcpy(&route_update, app_packet.body, sizeof(route_update));
        route_update_ready = 1U;
    }
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER &&
        physical_peer_id != 0U && physical_peer_id != SLE_TEAM_BROADCAST_ID &&
        physical_peer_id != app_packet.src_id) {
        route_update_can_override_next_hop = 0U;
    }
    if (route_update_can_override_next_hop != 0U && route_update_ready != 0U) {
        if (route_update.next_hop_id != 0U && route_update.next_hop_id != SLE_TEAM_BROADCAST_ID) {
            next_hop_id = route_update.next_hop_id;
        } else if (route_update.parent_id != 0U && route_update.parent_id != SLE_TEAM_BROADCAST_ID) {
            next_hop_id = route_update.parent_id;
        }
    } else if (route_update_ready != 0U && route_update_can_override_next_hop == 0U) {
        osal_printk("[team] route update keep physical next_hop src=%u physical=%u\r\n",
            app_packet.src_id, physical_peer_id);
    }
    team_route_update_observe(&app_packet);
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER && dir == TEAM_LINK_UPSTREAM &&
        physical_peer_id != 0U && physical_peer_id != SLE_TEAM_BROADCAST_ID) {
        if (physical_peer_id != g_team_node.cfg.leader_id ||
            team_member_has_reselect_target(g_team_node.upstream_parent_id) == 0U) {
            team_upstream_parent_note(physical_peer_id, SLE_TEAM_PARENT_CONNECTED, "packet");
        }
    }
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        if (sle_uart_client_bind_member_conn(physical_peer_id, conn_id) != 0U) {
            team_route_note(app_packet.src_id, conn_id, TEAM_CONN_DIR_DOWNSTREAM, next_hop_id);
        } else {
            osal_printk("[team] route note skip member=%u conn=%u dir=%u reason=bind-failed\r\n",
                app_packet.src_id, conn_id, (uint8_t)TEAM_CONN_DIR_DOWNSTREAM);
        }
    } else if (dir == TEAM_LINK_DOWNSTREAM) {
        if (sle_uart_client_bind_member_conn(app_packet.src_id, conn_id) != 0U) {
            team_route_note(app_packet.src_id, conn_id, TEAM_CONN_DIR_DOWNSTREAM, next_hop_id);
        } else {
            osal_printk("[team] route note skip member=%u conn=%u dir=%u reason=bind-failed\r\n",
                app_packet.src_id, conn_id, (uint8_t)TEAM_CONN_DIR_DOWNSTREAM);
        }
    } else {
        if (sle_uart_server_bind_member_conn(physical_peer_id, conn_id) != 0U) {
            team_route_note(app_packet.src_id, conn_id, TEAM_CONN_DIR_UPSTREAM, next_hop_id);
        } else {
            osal_printk("[team] route note skip member=%u conn=%u dir=%u reason=bind-failed\r\n",
                app_packet.src_id, conn_id, (uint8_t)TEAM_CONN_DIR_UPSTREAM);
        }
    }
}

static void team_node_init(sle_team_node_role_t role, uint8_t leader_id)
{
    sle_team_node_cfg_t cfg;
    sle_team_node_ops_t ops;

    (void)memset_s(&cfg, sizeof(cfg), 0, sizeof(cfg));
    (void)memset_s(&ops, sizeof(ops), 0, sizeof(ops));

    cfg.team_id = CONFIG_SLE_TEAM_TEAM_ID;
    cfg.self_id = g_team_rt.route_id != 0U ? g_team_rt.route_id : CONFIG_SLE_TEAM_SELF_ID;
    cfg.leader_id = leader_id != 0U ? leader_id : CONFIG_SLE_TEAM_LEADER_ID;
    if (g_team_rt.self_mac_ready != 0U) {
        (void)memcpy(cfg.self_mac, g_team_rt.self_mac, sizeof(cfg.self_mac));
        cfg.self_mac_ready = 1U;
    }
    cfg.role = role;
    cfg.channel_hash = CONFIG_SLE_TEAM_CHANNEL_HASH;
    if (role == SLE_TEAM_ROLE_LEADER) {
        cfg.member_filter_enabled = 1U;
    }
    cfg.relay_enabled = 0U;
    cfg.relay_allowed = 0U;
    cfg.relay_tier = 0U;
    cfg.max_downstream = 0U;
    cfg.report_interval_s = CONFIG_SLE_TEAM_REPORT_INTERVAL_S;
    cfg.heartbeat_interval_s = CONFIG_SLE_TEAM_HEARTBEAT_INTERVAL_S;
    cfg.warn_distance_m = CONFIG_SLE_TEAM_WARN_DISTANCE_M;
    cfg.lost_distance_m = CONFIG_SLE_TEAM_LOST_DISTANCE_M;
    cfg.heartbeat_timeout_s = CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S;
    cfg.parent_timeout_s = (uint16_t)(CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S / 2U);
    cfg.default_ttl = 4U;

    ops.send = team_sle_send;
    ops.now_s = team_now_s;
    ops.rssi_dbm = team_rssi_dbm;
    ops.battery_percent = team_battery_percent_cb;
    ops.log = team_log;
    ops.on_joined = team_joined;
    ops.on_position = team_position;
    ops.on_alert = team_alert;
    ops.on_relay_offline = team_on_relay_offline;
    ops.should_defer_member_timeout = team_should_defer_member_timeout;

    (void)sle_team_node_init(&g_team_node, &cfg, &ops);
    sle_uart_server_adv_set_route_id(g_team_node.cfg.self_id);
    g_team_rt.relay_client_started = 0U;
    g_team_rt.member_upstream_recover_last_s = 0U;
    g_team_rt.member_upstream_not_ready_since_s = 0U;
    g_team_rt.member_upstream_not_ready_count = 0U;
    (void)memset_s(g_team_conn_tracks, sizeof(g_team_conn_tracks), 0, sizeof(g_team_conn_tracks));
    (void)memset_s(g_team_pending_conns, sizeof(g_team_pending_conns), 0, sizeof(g_team_pending_conns));
    (void)memset_s(g_team_routes, sizeof(g_team_routes), 0, sizeof(g_team_routes));
    (void)memset_s(g_team_rt.direct_prune_conn_ids, sizeof(g_team_rt.direct_prune_conn_ids), 0,
        sizeof(g_team_rt.direct_prune_conn_ids));
    (void)memset_s(g_team_rt.direct_prune_member_ids, sizeof(g_team_rt.direct_prune_member_ids), 0,
        sizeof(g_team_rt.direct_prune_member_ids));
    sle_team_cli_init(&g_team_cli, &g_team_node, team_cli_print, NULL);
}

static void team_server_read_cb(uint8_t server_id, uint16_t conn_id, ssaps_req_read_cb_t *read_cb_para,
    errcode_t status)
{
    unused(server_id);
    unused(conn_id);
    unused(read_cb_para);
    unused(status);
}

static void team_member_drop_relay_children(const char *reason)
{
    uint16_t conn_ids[SLE_TEAM_MAX_DIRECT_CONNECTIONS];
    uint8_t conn_count;
    uint8_t i;
    uint8_t dropped = 0U;

    if (g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER || g_team_rt.relay_client_started == 0U) {
        return;
    }
    conn_count = sle_uart_client_get_active_conns(conn_ids, (uint8_t)SLE_TEAM_MAX_DIRECT_CONNECTIONS);
    for (i = 0U; i < conn_count; i++) {
        uint8_t member_id = 0U;
        uint8_t ret;

        (void)sle_uart_client_get_conn_member(conn_ids[i], &member_id);
        ret = sle_uart_client_disconnect_conn(conn_ids[i]);
        team_route_clear_by_conn(conn_ids[i]);
        team_conn_track_clear(conn_ids[i]);
        if (ret != 0U) {
            dropped++;
        }
        osal_printk("[team] relay demote drop child conn=%u member=%u reason=%s ret=%u\r\n",
            conn_ids[i], member_id, reason != NULL ? reason : "unknown", ret);
    }
    if (dropped != 0U) {
        g_team_rt.relay_child_rescan_last_s = 0U;
    }
}

static void team_server_write_cb(uint8_t server_id, uint16_t conn_id, ssaps_req_write_cb_t *write_cb_para,
    errcode_t status)
{
    int ret;
    uint8_t relay_allowed_before;
    uint8_t relay_enabled_before;

    unused(server_id);
    unused(conn_id);
    if (status != ERRCODE_SLE_SUCCESS || write_cb_para == NULL || write_cb_para->value == NULL) {
        return;
    }
    team_bind_packet_source(conn_id, write_cb_para->value, write_cb_para->length, TEAM_LINK_UPSTREAM);
    team_web_record_packet(SLE_TEAM_WEB_EVENT_RX, write_cb_para->value, write_cb_para->length,
        g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER ? "leader rx" : "member rx");
    relay_allowed_before = g_team_node.cfg.relay_allowed;
    relay_enabled_before = g_team_node.cfg.relay_enabled;
    ret = sle_team_node_on_packet(&g_team_node, write_cb_para->value, write_cb_para->length);
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER &&
        (relay_allowed_before != 0U || relay_enabled_before != 0U) &&
        g_team_node.cfg.relay_allowed == 0U) {
        team_member_drop_relay_children("config-demote");
    }
    osal_printk("[team] node packet role=%u len=%u ret=%d\r\n", g_team_node.cfg.role,
        write_cb_para->length, ret);
}

static void team_client_rx_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    int ret;

    unused(client_id);
    unused(conn_id);
    if (status != ERRCODE_SLE_SUCCESS || data == NULL || data->data == NULL) {
        return;
    }
    team_bind_packet_source(conn_id, data->data, data->data_len, TEAM_LINK_DOWNSTREAM);
    team_web_record_packet(SLE_TEAM_WEB_EVENT_RX, data->data, data->data_len,
        g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER ? "leader rx" : "member rx");
    ret = sle_team_node_on_packet(&g_team_node, data->data, data->data_len);
    osal_printk("[team] node packet role=%u len=%u ret=%d\r\n", g_team_node.cfg.role,
        data->data_len, ret);
}

static void team_relay_start_client_if_ready(void)
{
    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER || team_route_is_relay_enabled() == 0U ||
        g_team_rt.relay_client_started != 0U) {
        return;
    }
    if (g_team_node.joined == 0U || g_team_node.cfg.relay_allowed == 0U) {
        return;
    }
    if (sle_uart_server_connected_count() == 0U) {
        return;
    }

    sle_uart_client_set_seek_filter(team_client_seek_filter, NULL);
    sle_uart_client_init(team_client_rx_cb, team_client_rx_cb);
    team_register_connection_callbacks();
    g_team_rt.relay_client_started = 1U;
    team_print("relay sle client started");
}

void sle_uart_notification_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    team_client_rx_cb(client_id, conn_id, data, status);
}

void sle_uart_indication_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    team_client_rx_cb(client_id, conn_id, data, status);
}

static int team_sle_start(void)
{
    if (g_team_rt.sle_started != 0U) {
        return SLE_TEAM_OK;
    }
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        sle_uart_client_set_seek_filter(team_client_seek_filter, NULL);
        sle_uart_client_init(team_client_rx_cb, team_client_rx_cb);
        team_register_connection_callbacks();
        g_team_rt.sle_started = 1U;
        team_print("leader sle 1vs8 client started");
        return SLE_TEAM_OK;
    }

    team_sle_prepare_local_addr();
    if (sle_uart_server_init(team_server_read_cb, team_server_write_cb) == ERRCODE_SLE_SUCCESS) {
        team_register_connection_callbacks();
        g_team_rt.sle_started = 1U;
        if (team_route_is_relay_enabled() != 0U) {
            team_print("member relay server ready");
        } else {
            team_print("member sle server ready");
        }
        return SLE_TEAM_OK;
    }
    team_print("member sle server init failed");
    return SLE_TEAM_ERR_UNSUPPORTED;
}

static int team_configure_role(sle_team_node_role_t role, uint8_t leader_id)
{
    int ret;

    if (g_team_rt.role_configured != 0U) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
    if (g_team_rt.self_mac_ready == 0U) {
        team_print("identity mac not ready");
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
#endif
    team_node_init(role, leader_id);
    team_identity_refresh_labels();
    ret = team_sle_start();
    if (ret != SLE_TEAM_OK) {
        (void)memset_s(&g_team_node, sizeof(g_team_node), 0, sizeof(g_team_node));
        (void)memset_s(&g_team_cli, sizeof(g_team_cli), 0, sizeof(g_team_cli));
        team_identity_refresh_labels();
        return ret;
    }
    g_team_rt.role_configured = 1U;
    team_identity_refresh_labels();
    team_display_refresh_status();
    team_ws2812_refresh_network_state();
    sle_team_cli_print_help(&g_team_cli);
    osal_printk("[team] configured self=%u leader=%u role=%u team=%u label=%s\r\n",
        g_team_node.cfg.self_id,
        g_team_node.cfg.leader_id,
        g_team_node.cfg.role,
        g_team_node.cfg.team_id,
        g_team_rt.self_label);
    return SLE_TEAM_OK;
}

static int team_request_role_config(sle_team_node_role_t role, uint8_t leader_id, uint8_t team_id,
    uint8_t channel_hash, uint16_t leader_suffix, uint8_t save_nv)
{
    if (g_team_rt.role_configured != 0U || g_team_rt.role_request_pending != 0U) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
    if (g_team_rt.self_mac_ready == 0U) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
#endif
    g_team_rt.role_request_role = (uint8_t)role;
    g_team_rt.role_request_leader = leader_id;
    g_team_rt.role_request_team = team_id;
    g_team_rt.role_request_channel = channel_hash;
    g_team_rt.role_request_leader_suffix = leader_suffix;
    g_team_rt.role_request_save_nv = save_nv;
    g_team_rt.role_request_last_ret = SLE_TEAM_OK;
    g_team_rt.role_request_pending = 1U;
    osal_printk("[team] role request queued role=%u leader=%u team=%u channel=%u suffix=%04X\r\n",
        (uint8_t)role, leader_id, team_id, channel_hash, leader_suffix);
    return SLE_TEAM_OK;
}

static void team_handle_role_request_once(void)
{
    sle_team_node_role_t role;
    uint8_t leader;
    uint8_t team;
    uint8_t channel;
    uint8_t save_nv;
    uint16_t leader_suffix;
    int ret;

    if (g_team_rt.role_request_pending == 0U) {
        return;
    }

    role = (sle_team_node_role_t)g_team_rt.role_request_role;
    leader = g_team_rt.role_request_leader;
    team = g_team_rt.role_request_team;
    channel = g_team_rt.role_request_channel;
    leader_suffix = g_team_rt.role_request_leader_suffix;
    save_nv = g_team_rt.role_request_save_nv;

    ret = team_configure_role(role, leader);
    if (ret == SLE_TEAM_OK && role == SLE_TEAM_ROLE_LEADER) {
        g_team_node.cfg.team_id = team;
        g_team_node.cfg.channel_hash = channel;
        (void)team_nv_allowed_apply_to_node();
    }
    if (ret == SLE_TEAM_OK && role == SLE_TEAM_ROLE_MEMBER) {
        int hello_ret;

        g_team_node.cfg.team_id = team;
        g_team_node.cfg.channel_hash = channel;
        if (leader_suffix != 0U) {
            (void)snprintf(g_team_rt.leader_label, sizeof(g_team_rt.leader_label), "L%04X", leader_suffix);
        }
        hello_ret = sle_team_node_member_select_leader(&g_team_node, team, leader, channel);
        team_refresh_relay_mode();
        osal_printk("[team] member initial hello ret=%d, retry by tick if not ready\r\n", hello_ret);
    }
    if (ret == SLE_TEAM_OK && (role == SLE_TEAM_ROLE_LEADER || role == SLE_TEAM_ROLE_MEMBER)) {
        team_ws2812_refresh_network_state();
        team_buzzer_set(0U);
    }
    if (ret == SLE_TEAM_OK && save_nv != 0U) {
        if (role == SLE_TEAM_ROLE_LEADER) {
            (void)team_nv_config_save(SLE_TEAM_ROLE_LEADER, g_team_node.cfg.team_id, team_self_mac_suffix(),
                g_team_node.cfg.channel_hash);
        } else {
            (void)team_nv_config_save(SLE_TEAM_ROLE_MEMBER, team, leader_suffix, channel);
        }
    }
    g_team_rt.role_request_last_ret = ret;
    g_team_rt.role_request_pending = 0U;
    osal_printk("[team] role request done role=%u leader=%u team=%u channel=%u suffix=%04X ret=%d\r\n",
        (uint8_t)role, leader, team, channel, leader_suffix, ret);
}

static void team_restore_web_config(void)
{
    sle_team_web_config_nv_t cfg;
    uint8_t leader;
    int ret;

    if (team_nv_config_load(&cfg) != SLE_TEAM_OK) {
        return;
    }
    if (cfg.role == (uint8_t)SLE_TEAM_ROLE_LEADER) {
        ret = team_configure_role(SLE_TEAM_ROLE_LEADER, g_team_rt.route_id);
        if (ret == SLE_TEAM_OK) {
            g_team_node.cfg.team_id = cfg.team_id;
            g_team_node.cfg.channel_hash = cfg.channel_hash;
            (void)team_nv_allowed_apply_to_node();
        }
        osal_printk("[team-nv] restore leader ret=%d\r\n", ret);
        return;
    }

    leader = team_route_id_from_suffix(cfg.leader_suffix);
    ret = team_configure_role(SLE_TEAM_ROLE_MEMBER, leader);
    if (ret == SLE_TEAM_OK) {
        g_team_node.cfg.team_id = cfg.team_id;
        g_team_node.cfg.channel_hash = cfg.channel_hash;
        (void)snprintf(g_team_rt.leader_label, sizeof(g_team_rt.leader_label), "L%04X", cfg.leader_suffix);
        (void)sle_team_node_member_select_leader(&g_team_node, cfg.team_id, leader, cfg.channel_hash);
        team_refresh_relay_mode();
    }
    osal_printk("[team-nv] restore member leader_suffix=%04X leader=%u ret=%d\r\n",
        cfg.leader_suffix, leader, ret);
}

static void team_handle_cli_queue_once(void)
{
    typedef int (*team_cli_handler_t)(const char *line);
    sle_team_cli_msg_t msg;
    uint32_t msg_size = sizeof(msg);
    uint8_t handler_idx;
    static const team_cli_handler_t cli_handlers[] = {
        team_ws2812_cli_handle,
        team_buzzer_cli_handle,
        team_battery_cli_handle,
        team_display_cli_handle,
        team_led_cli_handle,
        team_serial_cfg_cli_handle,
    };

    if (g_team_rt.cli_queue_ready == 0U) {
        osal_msleep(SLE_TEAM_CLI_QUEUE_TIMEOUT_MS);
        return;
    }

    (void)memset_s(&msg, sizeof(msg), 0, sizeof(msg));
    if (osal_msg_queue_read_copy(g_team_rt.cli_queue_id, &msg, &msg_size, SLE_TEAM_CLI_QUEUE_TIMEOUT_MS) ==
        OSAL_SUCCESS) {
#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
        if (strcmp(msg.line, "wifi") == 0) {
            team_wifi_print_status();
            return;
        }
        if (strcmp(msg.line, "http") == 0) {
            team_http_print_status();
            return;
        }
#endif
        if (strcmp(msg.line, "reboot") == 0 || strcmp(msg.line, "reset") == 0) {
            osal_printk("[cli] reboot requested\r\n");
            team_reboot_schedule("cli");
            return;
        }
        if (strcmp(msg.line, "role leader") == 0) {
            uint8_t team = team_cfg_default_team();
            uint8_t channel = team_cfg_default_channel();
            int ret = team_cfg_save_leader(team, channel, 1U);
            osal_printk("[cli] role leader ret=%d saved=1 team=%u channel=%u\r\n", ret, team, channel);
            return;
        }
        if (strncmp(msg.line, "role member ", 12) == 0) {
            unsigned int suffix = 0U;
            int ret;
            uint8_t team = team_cfg_default_team();
            uint8_t channel = team_cfg_default_channel();
            if (sscanf(msg.line + 12, "%x", &suffix) != 1 || suffix > 0xFFFFU) {
                osal_printk("[cli] usage: role member <leader_mac_suffix>\r\n");
                return;
            }
            ret = team_cfg_save_member((uint16_t)suffix, team, channel, 1U);
            osal_printk("[cli] role member leader_suffix=%04X leader=%u ret=%d saved=1 team=%u channel=%u\r\n",
                suffix, team_route_id_from_suffix((uint16_t)suffix), ret, team, channel);
            return;
        }
        for (handler_idx = 0U; handler_idx < (uint8_t)(sizeof(cli_handlers) / sizeof(cli_handlers[0])); handler_idx++) {
            if (cli_handlers[handler_idx](msg.line) != 0) {
                return;
            }
        }
        if (strcmp(msg.line, "leave") == 0) {
            int ret;
            if (g_team_rt.role_configured == 0U || g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER) {
                osal_printk("[cli] leave ignored role_configured=%u role=%u\r\n",
                    g_team_rt.role_configured, g_team_node.cfg.role);
                return;
            }
            ret = sle_team_node_member_leave(&g_team_node);
            if (ret == SLE_TEAM_OK) {
                team_member_reset_after_leave();
            }
            osal_printk("[cli] leave ret=%d\r\n", ret);
            return;
        }
        if (g_team_rt.role_configured != 0U) {
            sle_team_cli_handle_line(&g_team_cli, msg.line);
        } else {
            osal_printk("[cli] configure first: role leader | role member <leader_mac_suffix>\r\n");
        }
    }
}

static void team_network_wait_identity_ready(void)
{
#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
    uint32_t identity_wait_ms = 0U;

    while (g_team_rt.self_mac_ready == 0U && identity_wait_ms < SLE_TEAM_IDENTITY_WAIT_MAX_MS) {
        osal_msleep(100);
        identity_wait_ms += 100U;
    }
#endif
}

static void team_network_task_bootstrap(void)
{
    sle_team_web_event_log_init(&g_team_events);
    team_network_wait_identity_ready();
    team_uart_init();
    team_uart_cli_start();
    osal_printk("[hw] init begin fw=%s\r\n", SLE_TEAM_FW_VERSION);
    team_gps_init();
    team_chrg_init();
    team_adc_init();
    team_ws2812_init();
    team_buzzer_init();
    team_display_start();
    team_display_wait_ready(1200U);
    team_display_refresh_status();
    team_ws2812_refresh_network_state();
    team_hardware_report_print();
    osal_printk("[team] boot unconfigured route=%u label=%s ssid=%s\r\n",
        g_team_rt.route_id,
        g_team_rt.self_label,
        g_team_rt.softap_ssid);
    team_restore_web_config();
}

static void team_network_tick_common(void)
{
    team_buzzer_toggle_tick();
    team_ws2812_refresh_network_state();
    team_handle_cli_queue_once();
    team_handle_role_request_once();
    sle_uart_client_tick();
}

static void team_network_tick_role_configured(void)
{
    uint8_t joined_before = g_team_node.joined;

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        uint8_t leader_has_offline_members;

        if (g_team_node.cfg.pairing_enabled != 0U || sle_uart_client_is_ready() == 0U) {
            team_led_post_seek_throttled();
        }
        leader_has_offline_members = team_leader_has_offline_members();
        if (g_team_node.cfg.pairing_enabled != 0U || sle_uart_client_connected_count() == 0U ||
            leader_has_offline_members != 0U) {
            const char *rescan_reason = "no_member";

            if (g_team_node.cfg.pairing_enabled != 0U) {
                rescan_reason = "pairing_window";
            } else if (leader_has_offline_members != 0U) {
                rescan_reason = "member_offline";
            }
            team_leader_rescan_if_needed(rescan_reason);
        }
        if (g_team_node.cfg.pairing_enabled != 0U) {
            team_leader_pairing_rotate_connections();
        }
    }

    team_relay_start_client_if_ready();
    team_member_parent_reselect_disconnect_tick();
    team_member_upstream_recover_tick();
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER && team_member_relay_can_accept_child() != 0U &&
        g_team_rt.relay_client_started != 0U) {
        uint32_t now_s = team_now_s(NULL);

        if (team_interval_not_reached(now_s, g_team_rt.relay_child_rescan_last_s,
                SLE_TEAM_RELAY_CHILD_RESCAN_INTERVAL_S) == 0U) {
            g_team_rt.relay_child_rescan_last_s = now_s;
            sle_uart_client_force_rescan();
        }
    }
    team_member_autoselect_parent();
    team_request_sle_rssi();
    team_battery_sample_tick(0U);
    sle_team_node_tick(&g_team_node);
    team_display_note_offline_delta();
    team_leader_relay_config_retry_tick();
    team_leader_rebalance_relays(0U);
    team_leader_enforce_direct_capacity(0U);
    team_leader_route_metrics_update();

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER && joined_before != 0U && g_team_node.joined == 0U) {
        team_upstream_parent_reset("heartbeat timeout");
        team_ws2812_refresh_network_state();
        if (g_team_rt.relay_client_started != 0U) {
            sle_uart_client_force_rescan();
        }
    }
}

static void team_network_prestart(void)
{
    team_buzzer_force_pin_off((uint8_t)CONFIG_SLE_TEAM_BUZZER_PIN);
    team_led_start();
#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
    team_identity_set_fallback();
    team_wifi_ap_entry();
#endif
}

static void team_network_spawn_task(void)
{
    osal_task *task = NULL;

    osal_kthread_lock();
    task = osal_kthread_create((osal_kthread_handler)team_network_task, NULL, "TeamNetworkTask",
        SLE_TEAM_APP_TASK_STACK_SIZE);
    if (task != NULL) {
        osal_kthread_set_priority(task, SLE_TEAM_APP_TASK_PRIO);
    }
    osal_kthread_unlock();
}

static void *team_network_task(const char *arg)
{
    unused(arg);
    team_network_task_bootstrap();

    while (1) {
        team_network_tick_common();
        if (g_team_rt.role_configured != 0U) {
            team_network_tick_role_configured();
        }
        osal_msleep(SLE_TEAM_MAIN_LOOP_SLEEP_MS);
    }

    return NULL;
}

static void team_network_entry(void)
{
    team_network_prestart();
    team_network_spawn_task();
}

app_run(team_network_entry);
