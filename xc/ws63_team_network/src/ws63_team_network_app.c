#include "sle_team_cli.h"
#include "sle_team_web_api.h"
#include "ws63_console_pages.h"

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "app_init.h"
#include "common_def.h"
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
#define CONFIG_SLE_TEAM_UART_TXD_PIN 17
#endif

#ifndef CONFIG_SLE_TEAM_UART_RXD_PIN
#define CONFIG_SLE_TEAM_UART_RXD_PIN 18
#endif

#ifndef CONFIG_SLE_TEAM_LED_PIN
#define CONFIG_SLE_TEAM_LED_PIN 2
#endif

#ifndef CONFIG_SLE_TEAM_LED_ACTIVE_LOW
#define CONFIG_SLE_TEAM_LED_ACTIVE_LOW 0
#endif

#ifndef CONFIG_SLE_TEAM_HEARTBEAT_INTERVAL_S
#define CONFIG_SLE_TEAM_HEARTBEAT_INTERVAL_S 3
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
#define CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S 10
#endif

#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
#ifndef CONFIG_SLE_TEAM_WIFI_AP_SSID
#define CONFIG_SLE_TEAM_WIFI_AP_SSID "SLE-TEAM-WS63"
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
#define SLE_TEAM_CLI_QUEUE_TIMEOUT_MS 200
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
#define SLE_TEAM_HTTP_RECV_TIMEOUT_MS 3000
#define SLE_TEAM_HTTP_SEND_TIMEOUT_MS 3000
#define SLE_TEAM_FACTORY_RESET_TASK_STACK_SIZE 0x800
#define SLE_TEAM_FACTORY_RESET_TASK_PRIO 12
#define SLE_TEAM_HTTP_REQ_BUF_SIZE 192
#define SLE_TEAM_HTTP_JSON_BUF_SIZE 1024
#define SLE_TEAM_HTTP_HTML_BUF_SIZE 3072
#define SLE_TEAM_HTTP_PATH_BUF_SIZE 160
#define SLE_TEAM_ROUTE_ID_FALLBACK 1U
#define TEAM_CONN_TRACK_MAX 16U
#define TEAM_PENDING_CONN_MAX SLE_TEAM_MAX_DIRECT_CONNECTIONS
#define TEAM_ROUTE_ENTRY_MAX SLE_TEAM_MAX_MEMBERS
#define SLE_TEAM_PAIRING_ROTATE_INTERVAL_S 6U
#define SLE_TEAM_PAIRING_KEEP_CONNECTED 4U
#define SLE_TEAM_AUTO_RELAY_MAX 3U
#define SLE_TEAM_PARENT_SWITCH_COOLDOWN_S 5U
#define SLE_TEAM_PARENT_SWITCH_RSSI_HYST_DBM 8
#define SLE_TEAM_PARENT_CANDIDATE_MIN_RSSI (-95)
#define SLE_TEAM_RELAY_REBALANCE_INTERVAL_S 3U
#define SLE_TEAM_RELAY_REVOKE_STALE_FACTOR 2U
#define SLE_TEAM_RELAY_CANDIDATE_MIN_RSSI (-92)
#define SLE_TEAM_ROUTE_METRICS_INTERVAL_S 4U
#define SLE_TEAM_ROUTE_STALE_FACTOR 3U
#define SLE_TEAM_ROUTE_HINT_COOLDOWN_S 12U
#define SLE_TEAM_NV_KEY_WEB_CONFIG 0x5001
#define SLE_TEAM_NV_CONFIG_MAGIC 0x534C4554U
#define SLE_TEAM_NV_CONFIG_VERSION 1U
#define SLE_TEAM_MEMBER_RESCAN_INTERVAL_S 12U

typedef struct {
    char line[SLE_TEAM_CLI_LINE_SIZE];
} sle_team_cli_msg_t;

typedef enum {
    SLE_TEAM_LED_EVENT_TX = 1,
    SLE_TEAM_LED_EVENT_RX = 2,
    SLE_TEAM_LED_EVENT_SEEK = 3,
} sle_team_led_event_t;

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
    sle_addr_t addr;
} team_conn_track_t;

typedef struct {
    uint8_t active;
    sle_addr_t addr;
    uint8_t route_id;
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

typedef struct {
    uint8_t uart_rx_buf[SLE_TEAM_UART_RX_BUF_SIZE];
    char line_buf[SLE_TEAM_CLI_LINE_SIZE];
#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
    char self_label[8];
    char leader_label[8];
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
    uint8_t relay_online_count;
    uint8_t relay_target_count;
    uint32_t route_metrics_last_s;
    uint8_t route_metrics_active;
    uint8_t route_metrics_direct;
    uint8_t route_metrics_relayed;
    uint8_t route_metrics_unreachable;
    uint8_t route_metrics_stale;
    uint8_t route_metrics_converged;
    uint32_t route_metrics_epoch;
    uint32_t route_metrics_last_change_s;
    uint32_t route_metrics_last_converged_s;
    uint32_t route_hint_sent_total;
    uint32_t route_hint_failed_total;
    uint32_t route_hint_cooldown_skipped_total;
    uint32_t route_update_rx_total;
    uint32_t route_reparent_total;
    uint32_t route_reparent_last_s;
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
static void team_handle_role_request_once(void);
static void team_register_connection_callbacks(void);
static void team_upstream_parent_note(uint8_t parent_id, sle_team_parent_state_t state, const char *reason);
static void team_upstream_parent_reset(const char *reason);
static void team_leader_pairing_rotate_connections(void);
static void team_leader_auto_approve_pending(void);
static void team_member_autoselect_parent(void);
static void team_leader_rebalance_relays(void);
static void team_leader_route_metrics_update(void);
static void team_leader_route_convergence_hint(uint32_t now_s, uint8_t trigger_state_change,
    uint8_t stale_count, uint8_t unreachable_count);

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
        ret = uapi_nv_flush();
    }
    osal_printk("[team-nv] save role=%u team=%u leader_suffix=%04X channel=%u ret=0x%x\r\n",
        cfg.role, cfg.team_id, cfg.leader_suffix, cfg.channel_hash, ret);
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

    (void)memset_s(&blank, sizeof(blank), 0, sizeof(blank));
    ret = uapi_nv_write(SLE_TEAM_NV_KEY_WEB_CONFIG, (const uint8_t *)&blank, (uint16_t)sizeof(blank));
    if (ret == ERRCODE_SUCC) {
        (void)uapi_nv_flush();
    }
    osal_printk("[team-nv] clear web config ret=0x%x\r\n", ret);
    return ret == ERRCODE_SUCC ? SLE_TEAM_OK : SLE_TEAM_ERR_UNSUPPORTED;
}

static void team_print(const char *text)
{
    osal_printk("[state] %s\r\n", text);
}

static void team_led_set(uint8_t on)
{
    if (g_team_rt.led_active_low != 0U) {
        (void)uapi_gpio_set_val(g_team_rt.led_pin, on != 0U ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH);
    } else {
        (void)uapi_gpio_set_val(g_team_rt.led_pin, on != 0U ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
    }
}

static void team_led_configure(uint8_t pin, uint8_t active_low)
{
    g_team_rt.led_pin = pin;
    g_team_rt.led_active_low = active_low != 0U ? 1U : 0U;
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

static int team_led_cli_handle(const char *line)
{
    unsigned int pin;

    if (strcmp(line, "led") == 0 || strcmp(line, "led status") == 0) {
        team_led_cli_status();
        return 1;
    }
    if (strcmp(line, "led on") == 0) {
        team_led_set(1U);
        team_led_cli_status();
        return 1;
    }
    if (strcmp(line, "led off") == 0) {
        team_led_set(0U);
        team_led_cli_status();
        return 1;
    }
    if (strcmp(line, "led tx") == 0) {
        team_led_blink(SLE_TEAM_LED_TX_PULSES, SLE_TEAM_LED_TX_ON_MS, SLE_TEAM_LED_TX_OFF_MS);
        team_led_cli_status();
        return 1;
    }
    if (strcmp(line, "led rx") == 0) {
        team_led_blink(SLE_TEAM_LED_RX_PULSES, SLE_TEAM_LED_RX_ON_MS, SLE_TEAM_LED_RX_OFF_MS);
        team_led_cli_status();
        return 1;
    }
    if (strcmp(line, "led active_low") == 0) {
        team_led_configure(g_team_rt.led_pin, 1U);
        return 1;
    }
    if (strcmp(line, "led active_high") == 0) {
        team_led_configure(g_team_rt.led_pin, 0U);
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

static uint8_t team_route_is_relay_enabled(void)
{
    return (uint8_t)(g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER && g_team_node.joined != 0U &&
        g_team_node.cfg.relay_allowed != 0U && g_team_node.cfg.relay_enabled != 0U);
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

static void team_pending_conn_note(const sle_addr_t *addr, uint8_t route_id)
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
    if (conn_id == 0U) {
        return 0U;
    }
    if (dir == TEAM_CONN_DIR_DOWNSTREAM) {
        return sle_uart_client_has_conn(conn_id);
    }
    if (dir == TEAM_CONN_DIR_UPSTREAM) {
        return sle_uart_server_has_conn(conn_id);
    }
    return 0U;
}

static void team_route_note(uint8_t member_id, uint16_t conn_id, team_conn_dir_t dir, uint8_t next_hop_id)
{
    team_route_entry_t *route;

    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID || dir == TEAM_CONN_DIR_UNKNOWN) {
        return;
    }
    route = team_route_entry_alloc(member_id);
    if (route == NULL) {
        return;
    }
    route->conn_id = conn_id;
    route->dir = dir;
    route->next_hop_id = next_hop_id;
    route->last_seen_s = (uint32_t)(uapi_tcxo_get_ms() / 1000U);
}

static uint8_t team_route_find(uint8_t member_id, team_conn_dir_t dir, uint16_t *conn_id)
{
    team_route_entry_t *route = team_route_entry_find(member_id);

    if (route == NULL || route->dir != dir || conn_id == NULL) {
        return 0U;
    }
    if (route->next_hop_id != 0U && route->next_hop_id != SLE_TEAM_BROADCAST_ID) {
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

    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        if (g_team_routes[i].active != 0U && g_team_routes[i].conn_id == conn_id) {
            (void)memset_s(&g_team_routes[i], sizeof(g_team_routes[i]), 0, sizeof(g_team_routes[i]));
        }
    }
}

static void team_route_clear_by_next_hop(uint8_t next_hop_id)
{
    uint8_t i;

    if (next_hop_id == 0U || next_hop_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        if (g_team_routes[i].active != 0U && g_team_routes[i].next_hop_id == next_hop_id) {
            (void)memset_s(&g_team_routes[i], sizeof(g_team_routes[i]), 0, sizeof(g_team_routes[i]));
        }
    }
}

static uint8_t team_route_entry_is_stale(const team_route_entry_t *entry, uint32_t now_s, uint16_t timeout_s)
{
    if (entry == NULL || entry->active == 0U) {
        return 0U;
    }
    return team_elapsed_exceeds(now_s, entry->last_seen_s, (uint32_t)timeout_s * SLE_TEAM_ROUTE_STALE_FACTOR);
}

static uint8_t team_leader_route_hint_parent_for_member(const sle_team_member_record_t *member, uint8_t *out_parent_id)
{
    const team_route_entry_t *route;
    uint16_t conn_id = 0U;
    uint8_t member_id;

    if (member == NULL || out_parent_id == NULL || member->online == 0U) {
        return 0U;
    }
    member_id = member->member_id;
    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID || member_id == g_team_node.cfg.self_id) {
        return 0U;
    }

    /* Directly connected members should keep leader as parent. */
    if (sle_uart_client_find_conn_by_member(member_id, &conn_id) != 0U &&
        team_route_conn_is_active(TEAM_CONN_DIR_DOWNSTREAM, conn_id) != 0U) {
        *out_parent_id = g_team_node.cfg.leader_id;
        return 1U;
    }

    route = team_route_entry_find(member_id);
    if (route == NULL || route->active == 0U) {
        return 0U;
    }
    if (route->next_hop_id != 0U && route->next_hop_id != SLE_TEAM_BROADCAST_ID && route->next_hop_id != member_id) {
        *out_parent_id = route->next_hop_id;
        return 1U;
    }
    if (route->dir == TEAM_CONN_DIR_DOWNSTREAM && team_route_conn_is_active(route->dir, route->conn_id) != 0U) {
        *out_parent_id = g_team_node.cfg.leader_id;
        return 1U;
    }
    return 0U;
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
    const sle_team_route_update_body_t *route_update;
    uint8_t previous_next_hop;
    uint8_t observed_next_hop;
    uint32_t now_s;

    if (app_packet == NULL || g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER ||
        app_packet->app_msg_type != SLE_TEAM_APP_ROUTE_UPDATE ||
        app_packet->body_len < sizeof(sle_team_route_update_body_t)) {
        return;
    }
    route_update = (const sle_team_route_update_body_t *)app_packet->body;
    previous_next_hop = 0U;
    {
        const team_route_entry_t *existing = team_route_entry_find(app_packet->src_id);
        if (existing != NULL) {
            previous_next_hop = existing->next_hop_id;
        }
    }
    observed_next_hop = route_update->next_hop_id != 0U ? route_update->next_hop_id : route_update->parent_id;
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
        int ret;

        if (team_leader_route_hint_parent_for_member(member, &hint_parent_id) == 0U) {
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
    timeout_s = g_team_node.cfg.heartbeat_timeout_s != 0U ? g_team_node.cfg.heartbeat_timeout_s :
        CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S;

    for (i = 0U; i < TEAM_ROUTE_ENTRY_MAX; i++) {
        const team_route_entry_t *entry = &g_team_routes[i];

        if (entry->active == 0U || entry->member_id == 0U || entry->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        active++;
        if (entry->next_hop_id != 0U && entry->next_hop_id != SLE_TEAM_BROADCAST_ID &&
            entry->next_hop_id != entry->member_id && entry->next_hop_id != g_team_node.cfg.leader_id) {
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
    }

    converged = (uint8_t)((active != 0U || g_team_rt.relay_online_count == 0U) && unreachable == 0U && stale == 0U);
    changed = (uint8_t)(active != g_team_rt.route_metrics_active ||
        direct != g_team_rt.route_metrics_direct ||
        relayed != g_team_rt.route_metrics_relayed ||
        stale != g_team_rt.route_metrics_stale ||
        unreachable != g_team_rt.route_metrics_unreachable ||
        converged != g_team_rt.route_metrics_converged);
    trigger_state_change = (uint8_t)(converged != g_team_rt.route_metrics_converged);
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
    if (changed != 0U) {
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
    if (changed != 0U) {
        osal_printk("[team] route metrics active=%u direct=%u relayed=%u stale=%u unreachable=%u converged=%u epoch=%lu\r\n",
            active, direct, relayed, stale, unreachable, converged, (unsigned long)g_team_rt.route_metrics_epoch);
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
        if (g_team_conn_tracks[i].active != 0U && g_team_conn_tracks[i].route_id == route_id) {
            return &g_team_conn_tracks[i];
        }
    }
    return NULL;
}

static void team_conn_track_update(uint16_t conn_id, team_conn_dir_t dir, const sle_addr_t *addr)
{
    team_conn_track_t *track = team_conn_track_alloc(conn_id);
    team_pending_conn_t *pending;

    if (track == NULL) {
        return;
    }
    track->dir = dir;
    if (addr != NULL) {
        (void)memcpy_s(&track->addr, sizeof(track->addr), addr, sizeof(*addr));
        pending = team_pending_conn_find(addr);
        if (pending != NULL) {
            track->route_id = pending->route_id;
            team_pending_conn_clear(addr);
        } else {
            track->route_id = team_route_id_from_sle_addr(addr->addr);
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
    route_bucket = team_route_bucket_from_ids(route_id, g_team_node.cfg.leader_id);
    track->dir = dir;
    if (dir == TEAM_CONN_DIR_UPSTREAM && g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER &&
        track->route_id != 0U && track->route_id != g_team_node.cfg.leader_id) {
        uint8_t current_bucket = team_route_bucket_from_ids(track->route_id, g_team_node.cfg.leader_id);

        /* Preserve the real first-hop relay when a leader-origin packet is forwarded across the same link. */
        if (route_bucket == 0U && current_bucket > 0U) {
            return;
        }
    }
    track->route_id = route_id;
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

    team_conn_track_update(conn_id, dir, addr);
    if (conn_state == SLE_ACB_STATE_DISCONNECTED) {
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
                    (void)sle_team_node_member_leave(&g_team_node);
                    team_upstream_parent_reset("disconnect");
                    if (g_team_rt.relay_client_started != 0U) {
                        sle_uart_client_force_rescan();
                    }
                }
            }
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
    uint8_t self_bucket;
    uint8_t candidate_bucket;
    uint8_t candidate_id;
    size_t relay_server_name_len = sizeof(relay_server_name) - 1U;

    unused(user_ctx);
    if (seek_result_data == NULL || seek_result_data->data == NULL || seek_result_data->data_length == 0U) {
        return 0U;
    }
    if (seek_result_data->rssi == SLE_TEAM_RSSI_UNKNOWN) {
        return 0U;
    }
    if (team_buffer_contains(seek_result_data->data, seek_result_data->data_length,
            relay_server_name, relay_server_name_len) == 0U) {
        return 0U;
    }

    candidate_id = team_route_id_from_adv_data(seek_result_data->data, seek_result_data->data_length);
    if (candidate_id == 0U) {
        /* Legacy fallback: address-derived route ids are only provisional. */
        candidate_id = team_route_id_from_sle_addr(seek_result_data->addr.addr);
    }
    if (candidate_id == 0U || candidate_id == g_team_node.cfg.self_id || candidate_id == g_team_node.cfg.leader_id) {
        return 0U;
    }

    self_bucket = team_route_bucket_for_self();
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        candidate_bucket = team_route_bucket_from_ids(candidate_id, g_team_node.cfg.leader_id);
        if (candidate_bucket == 1U) {
            team_pending_conn_note(&seek_result_data->addr, candidate_id);
            return 1U;
        }
        return 0U;
    }
    if ((team_route_is_relay_enabled() == 0U && team_member_reselection_active() == 0U) || self_bucket >= 3U) {
        return 0U;
    }
    candidate_bucket = team_route_bucket_from_ids(candidate_id, g_team_node.cfg.leader_id);
    if (candidate_bucket == (uint8_t)(self_bucket + 1U)) {
        team_pending_conn_note(&seek_result_data->addr, candidate_id);
        return 1U;
    }
    return 0U;
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
        if (sle_uart_client_is_ready() == 0U) {
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
            sle_uart_client_is_ready() != 0U) {
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
    for (i = 0U; i < conn_count && (conn_count - keep_count) > SLE_TEAM_PAIRING_KEEP_CONNECTED; i++) {
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
    uint8_t relay_quota = SLE_TEAM_AUTO_RELAY_MAX;

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

static uint8_t team_leader_relay_target_for_online(uint8_t online_count)
{
    if (online_count == 0U || online_count <= SLE_TEAM_MAX_DIRECT_CONNECTIONS) {
        return 0U;
    }
    if (online_count <= (uint8_t)(SLE_TEAM_MAX_DIRECT_CONNECTIONS * 2U)) {
        return 2U;
    }
    return SLE_TEAM_AUTO_RELAY_MAX;
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

static int team_leader_set_member_relay_allowed(sle_team_member_record_t *member, uint8_t relay_allowed, const char *reason)
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
    ret = sle_team_node_send_config(&g_team_node, member->member_id);
    if (ret != SLE_TEAM_OK) {
        member->relay_allowed = old_allowed;
        if (old_allowed != 0U) {
            member->relay_tier = team_route_bucket_from_ids(member->member_id, g_team_node.cfg.leader_id);
            member->max_downstream = SLE_TEAM_MAX_DIRECT_CONNECTIONS;
        } else {
            member->relay_tier = 0U;
            member->max_downstream = 0U;
        }
        osal_printk("[team] relay set member=%u allow=%u reason=%s ret=%d\r\n",
            member->member_id, relay_allowed != 0U ? 1U : 0U, reason != NULL ? reason : "unknown", ret);
        return ret;
    }
    if (member->relay_allowed == 0U) {
        team_route_clear_by_next_hop(member->member_id);
    }
    osal_printk("[team] relay set member=%u allow=%u reason=%s ret=%d\r\n",
        member->member_id, member->relay_allowed, reason != NULL ? reason : "unknown", ret);
    return SLE_TEAM_OK;
}

static void team_leader_rebalance_relays(void)
{
    uint32_t now_s;
    uint16_t timeout_s;
    uint8_t i;
    uint8_t online_count = 0U;
    uint8_t relay_count = 0U;
    uint8_t relay_target;
    uint8_t changed = 0U;

    if (g_team_rt.role_configured == 0U || g_team_rt.sle_started == 0U ||
        g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER || g_team_node.cfg.pairing_enabled != 0U) {
        return;
    }
    now_s = team_now_s(NULL);
    if (team_interval_not_reached(now_s, g_team_rt.relay_rebalance_last_s, SLE_TEAM_RELAY_REBALANCE_INTERVAL_S) !=
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
            member->relay_allowed = 0U;
            member->relay_tier = 0U;
            member->max_downstream = 0U;
            team_route_clear_by_next_hop(member->member_id);
            changed = 1U;
            osal_printk("[team] relay revoke member=%u reason=offline\r\n", member->member_id);
            continue;
        }
        if (team_elapsed_exceeds(now_s, member->last_seen_s,
                (uint32_t)timeout_s * SLE_TEAM_RELAY_REVOKE_STALE_FACTOR) != 0U) {
            if (team_leader_set_member_relay_allowed(member, 0U, "stale") == SLE_TEAM_OK) {
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

    relay_target = team_leader_relay_target_for_online(online_count);
    if (relay_target > SLE_TEAM_AUTO_RELAY_MAX) {
        relay_target = SLE_TEAM_AUTO_RELAY_MAX;
    }

    while (relay_count > relay_target) {
        sle_team_member_record_t *victim = team_leader_pick_worst_active_relay(now_s, timeout_s);

        if (victim == NULL) {
            break;
        }
        if (team_leader_set_member_relay_allowed(victim, 0U, "auto-demote") != SLE_TEAM_OK) {
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
        if (team_leader_set_member_relay_allowed(candidate, 1U, "auto-promote") != SLE_TEAM_OK) {
            break;
        }
        relay_count++;
        changed = 1U;
    }

    g_team_rt.relay_online_count = relay_count;
    g_team_rt.relay_target_count = relay_target;
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
    if (team_route_find(g_team_node.cfg.leader_id, TEAM_CONN_DIR_UPSTREAM, &conn_id) != 0U) {
        track = team_conn_track_find(conn_id);
        if (track != NULL && track->route_id != 0U) {
            return track->route_id;
        }
    }
    conn_id = get_connect_id();
    track = team_conn_track_find(conn_id);
    if (track != NULL && track->route_id != 0U) {
        return track->route_id;
    }
    if (g_team_node.upstream_parent_state == SLE_TEAM_PARENT_CONNECTED &&
        g_team_node.upstream_parent_id != 0U && g_team_node.upstream_parent_id != SLE_TEAM_BROADCAST_ID) {
        return g_team_node.upstream_parent_id;
    }
    return g_team_node.cfg.leader_id;
}

static void team_cli_print(void *user_ctx, const char *text)
{
    unused(user_ctx);
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
    if (g_team_node.cfg.role != SLE_TEAM_ROLE_MEMBER || member_id != g_team_node.cfg.self_id) {
        return;
    }
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
}

static void team_alert(void *user_ctx, uint8_t member_id, uint8_t reason)
{
    unused(user_ctx);
    osal_printk("[team] alert member=%u reason=%u\r\n", member_id, reason);
}

static void team_on_relay_offline(void *user_ctx, uint8_t member_id)
{
    unused(user_ctx);
    osal_printk("[team] relay offline event member=%u trigger immediate rebalance\r\n", member_id);
    team_leader_rebalance_relays();
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
    ap_config.security_type = 3;
    ap_config.channel_num = CONFIG_SLE_TEAM_WIFI_AP_CHANNEL;
    ap_config.wifi_psk_type = 0;

    advance_config.beacon_interval = 100;
    advance_config.dtim_period = 2;
    advance_config.gi = 0;
    advance_config.group_rekey = 86400;
    advance_config.protocol_mode = 4;
    advance_config.hidden_ssid_flag = 1;

    ret = wifi_set_softap_config_advance(&advance_config);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[team-wifi] softap advance config failed ret=0x%x\r\n", ret);
        return -1;
    }
    ret = wifi_softap_enable(&ap_config);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[team-wifi] softap enable failed ret=0x%x\r\n", ret);
        return -1;
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

static int team_http_query_u8(const char *path, const char *key, uint8_t min_value, uint8_t max_value,
    uint8_t *out)
{
    char pattern[24];
    const char *p;
    unsigned long value = 0;
    uint8_t digits = 0U;

    if (path == NULL || key == NULL || out == NULL) {
        return -1;
    }
    (void)snprintf(pattern, sizeof(pattern), "%s=", key);
    p = strstr(path, pattern);
    if (p == NULL) {
        return -1;
    }
    p += strlen(pattern);
    while (*p >= '0' && *p <= '9') {
        value = value * 10UL + (unsigned long)(*p - '0');
        p++;
        digits++;
        if (value > 255UL) {
            return -1;
        }
    }
    if (*p != '\0' && *p != '&') {
        return -1;
    }
    if (digits == 0U || value < min_value || value > max_value) {
        return -1;
    }
    *out = (uint8_t)value;
    return 0;
}

static int team_http_query_u16(const char *path, const char *key, uint16_t min_value, uint16_t max_value,
    uint16_t *out)
{
    char pattern[24];
    const char *p;
    unsigned long value = 0;
    uint8_t digits = 0U;

    if (path == NULL || key == NULL || out == NULL) {
        return -1;
    }
    (void)snprintf(pattern, sizeof(pattern), "%s=", key);
    p = strstr(path, pattern);
    if (p == NULL) {
        return -1;
    }
    p += strlen(pattern);
    while (*p >= '0' && *p <= '9') {
        value = value * 10UL + (unsigned long)(*p - '0');
        p++;
        digits++;
        if (value > 65535UL) {
            return -1;
        }
    }
    if (*p != '\0' && *p != '&') {
        return -1;
    }
    if (digits == 0U || value < min_value || value > max_value) {
        return -1;
    }
    *out = (uint16_t)value;
    return 0;
}

static int team_http_query_i32(const char *path, const char *key, int32_t min_value, int32_t max_value,
    int32_t *out)
{
    char pattern[24];
    const char *p;
    long sign = 1L;
    unsigned long abs_value = 0UL;
    uint8_t digits = 0U;

    if (path == NULL || key == NULL || out == NULL) {
        return -1;
    }
    (void)snprintf(pattern, sizeof(pattern), "%s=", key);
    p = strstr(path, pattern);
    if (p == NULL) {
        return -1;
    }
    p += strlen(pattern);
    if (*p == '-') {
        sign = -1L;
        p++;
    } else if (*p == '+') {
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        abs_value = abs_value * 10UL + (unsigned long)(*p - '0');
        p++;
        digits++;
        if (abs_value > 2147483648UL) {
            return -1;
        }
    }
    if (*p != '\0' && *p != '&') {
        return -1;
    }
    if (digits == 0U) {
        return -1;
    }
    {
        long signed_value = (long)abs_value * sign;
        if (signed_value < (long)min_value || signed_value > (long)max_value) {
            return -1;
        }
        *out = (int32_t)signed_value;
    }
    return 0;
}

static int team_http_query_hex16(const char *path, const char *key, uint16_t *out)
{
    char pattern[24];
    const char *p;
    uint16_t value = 0U;
    uint8_t digits = 0U;

    if (path == NULL || key == NULL || out == NULL) {
        return -1;
    }
    (void)snprintf(pattern, sizeof(pattern), "%s=", key);
    p = strstr(path, pattern);
    if (p == NULL) {
        return -1;
    }
    p += strlen(pattern);
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

static void team_http_send_status_json_response(int fd)
{
    int ret;

    if (g_team_rt.role_configured == 0U) {
        (void)snprintf(g_team_http_json_buf, sizeof(g_team_http_json_buf),
            "{\"configured\":false,\"selfLabel\":\"%s\",\"routeId\":%u,\"macReady\":%s,"
            "\"macSuffix\":\"%02X%02X\",\"ssid\":\"%s\",\"transport\":\"ws63-softap\"}",
            g_team_rt.self_label,
            g_team_rt.route_id,
            g_team_rt.self_mac_ready != 0U ? "true" : "false",
            g_team_rt.self_mac[4],
            g_team_rt.self_mac[5],
            g_team_rt.softap_ssid);
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
    team_http_send_response(fd, ret < 0 ? "500 Internal Server Error" : "200 OK", "application/json",
        ret < 0 ? "{\"error\":\"status\"}" : g_team_http_json_buf);
}

static void team_http_send_location_event(int fd, const char *summary, uint8_t dst_id, int send_ret)
{
    if (summary == NULL) {
        summary = send_ret == SLE_TEAM_OK ? "location sent" : "location failed";
    }
    (void)snprintf(g_team_http_json_buf, sizeof(g_team_http_json_buf),
        "{\"id\":\"ws63-location-%lu\",\"time\":\"%lu\",\"direction\":\"%s\","
        "\"type\":\"POS_REPORT\",\"srcId\":%u,\"dstId\":%u,\"seq\":%u,\"summary\":\"%s\"}",
        (unsigned long)team_now_s(NULL),
        (unsigned long)team_now_s(NULL),
        send_ret == SLE_TEAM_OK ? "tx" : "fail",
        g_team_node.cfg.self_id,
        dst_id,
        g_team_node.next_seq == 0U ? 0U : (uint16_t)(g_team_node.next_seq - 1U),
        summary);
    team_http_send_response(fd, send_ret == SLE_TEAM_OK ? "200 OK" : "500 Internal Server Error",
        "application/json", g_team_http_json_buf);
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
        if (i == 0U && strcmp(active, "pairing") != 0) {
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
            "<div class=\"row\"><span class=\"k\">State</span><span class=\"v %s\">%s</span></div>",
            g_team_rt.role_request_pending != 0U ? "warn" : "ok",
            g_team_rt.role_request_pending != 0U ? "starting SLE" : "ready");
        team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"tag\">Choose this board role after boot.</div>"
            "<div class=\"bar\"><a href=\"/api/role?role=leader\">set leader</a></div>"
            "<form action=\"/api/role\" method=\"get\">"
            "<input name=\"role\" type=\"hidden\" value=\"member\">"
            "<div class=\"tag\">Set member: fill leader MAC suffix, team and channel.</div>"
            "<label>team</label>"
            "<input name=\"team\" type=\"number\" min=\"1\" max=\"254\" value=\"1\">"
            "<label>leader suffix</label>"
            "<input name=\"leader\" type=\"text\" maxlength=\"4\" placeholder=\"279A\" value=\"\">"
            "<label>channel</label>"
            "<input name=\"channel\" type=\"number\" min=\"0\" max=\"255\" value=\"17\">"
            "<button type=\"submit\">set member</button></form>"
            "<div class=\"tag\">Use the leader MAC suffix shown after L, for example A3F7.</div>"
            "<div class=\"bar\"><a href=\"/api/factory-reset\">factory reset</a></div>"
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
    team_http_append_html_end(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used);
    team_http_send_response(fd, "200 OK", "text/html; charset=utf-8", g_team_http_html_buf);
}

static void team_http_handle_client(int fd)
{
    int ret;
    char path[SLE_TEAM_HTTP_PATH_BUF_SIZE];

    ret = recv(fd, g_team_http_req_buf, sizeof(g_team_http_req_buf) - 1U, 0);
    if (ret <= 0) {
        g_team_http_last_errno = errno;
        osal_printk("[team-wifi] http recv failed fd=%d ret=%d errno=%d\r\n", fd, ret, g_team_http_last_errno);
        return;
    }
    g_team_http_req_buf[ret] = '\0';
    team_http_get_path(path, sizeof(path));
    osal_printk("[team-wifi] http recv fd=%d len=%d path=%s\r\n", fd, ret, path);

    if (strncmp(g_team_http_req_buf, "GET /api/status", 15) == 0) {
        osal_printk("[team-wifi] http route api=status\r\n");
        team_http_send_status_json_response(fd);
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
        uint8_t battery = 100U;
        uint8_t fix = 2U;
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
        if (dst == SLE_TEAM_BROADCAST_ID) {
            dst = SLE_TEAM_BROADCAST_ID;
        }

        (void)memset_s(&pos, sizeof(pos), 0, sizeof(pos));
        pos.latitude_e6 = lat;
        pos.longitude_e6 = lon;
        pos.speed_cms = speed;
        pos.heading_deg = heading;
        pos.battery_percent = battery;
        pos.fix_status = fix;
        pos.sat_count = sat;

        send_ret = sle_team_node_send_position(&g_team_node, dst, &pos);
        if (send_ret == SLE_TEAM_OK) {
            char summary[SLE_TEAM_WEB_EVENT_SUMMARY_SIZE];
            (void)snprintf(summary, sizeof(summary), "POS_REPORT %u->%u lat=%ld lon=%ld",
                g_team_node.cfg.self_id, dst, (long)lat, (long)lon);
            sle_team_web_event_push(&g_team_events, team_now_s(NULL), SLE_TEAM_WEB_EVENT_SYSTEM,
                SLE_TEAM_APP_POS_REPORT, g_team_node.cfg.self_id, dst,
                g_team_node.next_seq == 0U ? 0U : (uint16_t)(g_team_node.next_seq - 1U), summary);
            team_http_send_location_event(fd, "location sent", dst, send_ret);
        } else {
            team_http_send_location_event(fd, "location send failed", dst, send_ret);
        }
    } else if (strncmp(g_team_http_req_buf, "GET /api/pending", 16) == 0) {
        osal_printk("[team-wifi] http route api=pending\r\n");
        ret = sle_team_web_write_pending_json(&g_team_node, g_team_http_json_buf, sizeof(g_team_http_json_buf));
        team_http_send_response(fd, ret < 0 ? "500 Internal Server Error" : "200 OK", "application/json",
            ret < 0 ? "{\"error\":\"pending\"}" : g_team_http_json_buf);
    } else if (strncmp(g_team_http_req_buf, "GET /api/role", 13) == 0) {
        uint8_t team = CONFIG_SLE_TEAM_TEAM_ID;
        uint8_t channel = CONFIG_SLE_TEAM_CHANNEL_HASH;
        uint16_t leader_suffix = 0U;
        uint8_t leader = CONFIG_SLE_TEAM_LEADER_ID;
        osal_printk("[team-wifi] http route api=role path=%s\r\n", path);
        if (strstr(path, "role=leader") != NULL) {
            ret = team_request_role_config(SLE_TEAM_ROLE_LEADER, g_team_rt.route_id, g_team_node.cfg.team_id,
                g_team_node.cfg.channel_hash, team_self_mac_suffix(), 1U);
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
            g_team_rt.pairing_rotate_last_s = 0U;
            g_team_rt.pairing_rotate_index = 0U;
            osal_printk("[team-wifi] pairing start ret=%d\r\n", ret);
            team_http_send_redirect(fd, "/pairing");
        } else if (strstr(path, "action=stop") != NULL) {
            team_leader_auto_approve_pending();
            ret = sle_team_node_pairing_stop(&g_team_node);
            g_team_rt.pairing_rotate_last_s = 0U;
            osal_printk("[team-wifi] pairing stop ret=%d\r\n", ret);
            team_http_send_redirect(fd, "/pairing");
        } else if (strstr(path, "action=approve") != NULL &&
            team_http_query_u8(path, "id", 1U, 254U, &id) == 0) {
            if (team_http_query_u8(path, "relay", 0U, 1U, &relay_allowed) != 0) {
                relay_allowed = 0U;
            }
            ret = sle_team_node_pairing_approve_with_relay(&g_team_node, id, relay_allowed);
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
            (void)team_nv_config_clear();
            g_team_rt.role_configured = 0U;
            g_team_rt.role_request_pending = 0U;
            g_team_rt.role_request_last_ret = SLE_TEAM_OK;
            (void)memset_s(&g_team_node, sizeof(g_team_node), 0, sizeof(g_team_node));
            (void)memset_s(&g_team_cli, sizeof(g_team_cli), 0, sizeof(g_team_cli));
            g_team_rt.leader_label[0] = '\0';
            team_identity_refresh_labels();
        }
        osal_printk("[team-wifi] member leave ret=%d\r\n", ret);
        team_http_send_redirect(fd, "/pairing");
    } else if (strncmp(g_team_http_req_buf, "GET /api/factory-reset", 22) == 0) {
        osal_printk("[team-wifi] http route api=factory_reset\r\n");
        ret = team_nv_config_clear();
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

static int team_sle_send(void *user_ctx, sle_team_send_kind_t kind, uint8_t dst_id, const uint8_t *buf, uint16_t len)
{
    uint16_t target_conn_id = 0U;
    uint8_t has_target_conn = 0U;
    sle_team_app_packet_t app_packet;
    uint8_t self_bucket;
    uint8_t src_bucket;
    uint8_t send_downstream = 0U;
    uint8_t send_upstream = 0U;
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
        errcode_t send_ret = (dst_id == SLE_TEAM_BROADCAST_ID) ?
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

    if (team_route_is_relay_enabled() == 0U) {
        if (sle_uart_server_connected_count() == 0U) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NOT_READY\r\n",
                dst_id, SLE_TEAM_ERR_UNSUPPORTED);
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
        errcode_t send_ret = (dst_id == SLE_TEAM_BROADCAST_ID || dst_id == g_team_node.cfg.leader_id) ?
            sle_uart_server_send_report_by_handle(buf, len) :
            sle_uart_server_send_report_by_conn(target_conn_id, buf, len);
        if (send_ret != ERRCODE_SLE_SUCCESS) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=WRITE_FAIL\r\n", dst_id, SLE_TEAM_ERR_FORMAT);
            return SLE_TEAM_ERR_FORMAT;
        }
        team_web_record_packet(SLE_TEAM_WEB_EVENT_TX, buf, len, "member tx");
        return SLE_TEAM_OK;
    }

    if (team_decode_app_packet_from_buf(buf, len, &app_packet) == 0U) {
        osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=FORMAT\r\n", dst_id, SLE_TEAM_ERR_FORMAT);
        return SLE_TEAM_ERR_FORMAT;
    }

    self_bucket = team_route_bucket_for_self();
    src_bucket = team_route_bucket_from_ids(app_packet.src_id, g_team_node.cfg.leader_id);
    if (app_packet.src_id == g_team_node.cfg.self_id) {
        send_upstream = (uint8_t)(dst_id == g_team_node.cfg.leader_id || dst_id == SLE_TEAM_BROADCAST_ID);
        send_downstream = (uint8_t)(send_upstream == 0U);
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
            errcode_t send_ret = sle_uart_client_send_all(buf, len);
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
        errcode_t send_ret = sle_uart_client_send_by_conn(target_conn_id, buf, len);
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
    errcode_t send_ret = (dst_id == SLE_TEAM_BROADCAST_ID || dst_id == g_team_node.cfg.leader_id) ?
        sle_uart_server_send_report_by_handle(buf, len) :
        sle_uart_server_send_report_by_conn(target_conn_id, buf, len);
    if (send_ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=WRITE_FAIL\r\n", dst_id, SLE_TEAM_ERR_FORMAT);
        return SLE_TEAM_ERR_FORMAT;
    }
    team_web_record_packet(SLE_TEAM_WEB_EVENT_TX, buf, len, "relay up tx");
    return SLE_TEAM_OK;
}

static void team_bind_packet_source(uint16_t conn_id, const uint8_t *buf, uint16_t len, team_link_dir_t dir)
{
    sle_team_app_packet_t app_packet;
    team_conn_track_t *track;
    uint8_t next_hop_id;
    const sle_team_route_update_body_t *route_update = NULL;

    if (buf == NULL || len == 0U || team_decode_app_packet_from_buf(buf, len, &app_packet) == 0U) {
        return;
    }
    if (app_packet.team_id != g_team_node.cfg.team_id || app_packet.src_id == 0U ||
        app_packet.src_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    track = team_conn_track_find(conn_id);
    next_hop_id = (track != NULL && track->route_id != 0U) ? track->route_id : app_packet.src_id;
    if (app_packet.app_msg_type == SLE_TEAM_APP_ROUTE_UPDATE &&
        app_packet.body_len >= sizeof(sle_team_route_update_body_t)) {
        route_update = (const sle_team_route_update_body_t *)app_packet.body;
        if (route_update->next_hop_id != 0U && route_update->next_hop_id != SLE_TEAM_BROADCAST_ID) {
            next_hop_id = route_update->next_hop_id;
        } else if (route_update->parent_id != 0U && route_update->parent_id != SLE_TEAM_BROADCAST_ID) {
            next_hop_id = route_update->parent_id;
        }
    }
    team_route_update_observe(&app_packet);
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER && dir == TEAM_LINK_UPSTREAM &&
        next_hop_id != 0U && next_hop_id != SLE_TEAM_BROADCAST_ID) {
        team_upstream_parent_note(next_hop_id, SLE_TEAM_PARENT_CONNECTED, "packet");
    }
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        sle_uart_client_bind_member_conn(app_packet.src_id, conn_id);
        team_conn_track_note_packet(conn_id, TEAM_CONN_DIR_DOWNSTREAM, app_packet.src_id);
        team_route_note(app_packet.src_id, conn_id, TEAM_CONN_DIR_DOWNSTREAM, next_hop_id);
    } else if (dir == TEAM_LINK_DOWNSTREAM) {
        sle_uart_client_bind_member_conn(app_packet.src_id, conn_id);
        team_conn_track_note_packet(conn_id, TEAM_CONN_DIR_DOWNSTREAM, app_packet.src_id);
        team_route_note(app_packet.src_id, conn_id, TEAM_CONN_DIR_DOWNSTREAM, next_hop_id);
    } else {
        sle_uart_server_bind_member_conn(app_packet.src_id, conn_id);
        team_conn_track_note_packet(conn_id, TEAM_CONN_DIR_UPSTREAM, app_packet.src_id);
        team_route_note(app_packet.src_id, conn_id, TEAM_CONN_DIR_UPSTREAM, next_hop_id);
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
    ops.log = team_log;
    ops.on_joined = team_joined;
    ops.on_position = team_position;
    ops.on_alert = team_alert;
    ops.on_relay_offline = team_on_relay_offline;

    (void)sle_team_node_init(&g_team_node, &cfg, &ops);
    sle_uart_server_adv_set_route_id(g_team_node.cfg.self_id);
    g_team_rt.relay_client_started = 0U;
    (void)memset_s(g_team_conn_tracks, sizeof(g_team_conn_tracks), 0, sizeof(g_team_conn_tracks));
    (void)memset_s(g_team_pending_conns, sizeof(g_team_pending_conns), 0, sizeof(g_team_pending_conns));
    (void)memset_s(g_team_routes, sizeof(g_team_routes), 0, sizeof(g_team_routes));
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

static void team_server_write_cb(uint8_t server_id, uint16_t conn_id, ssaps_req_write_cb_t *write_cb_para,
    errcode_t status)
{
    int ret;

    unused(server_id);
    unused(conn_id);
    if (status != ERRCODE_SLE_SUCCESS || write_cb_para == NULL || write_cb_para->value == NULL) {
        return;
    }
    team_bind_packet_source(conn_id, write_cb_para->value, write_cb_para->length, TEAM_LINK_UPSTREAM);
    team_web_record_packet(SLE_TEAM_WEB_EVENT_RX, write_cb_para->value, write_cb_para->length,
        g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER ? "leader rx" : "member rx");
    ret = sle_team_node_on_packet(&g_team_node, write_cb_para->value, write_cb_para->length);
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
    sle_team_cli_msg_t msg;
    uint32_t msg_size = sizeof(msg);

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
            int ret = team_configure_role(SLE_TEAM_ROLE_LEADER, g_team_rt.route_id);
            osal_printk("[cli] role leader ret=%d\r\n", ret);
            return;
        }
        if (strncmp(msg.line, "role member ", 12) == 0) {
            unsigned int suffix = 0U;
            int ret;
            if (sscanf(msg.line + 12, "%x", &suffix) != 1 || suffix > 0xFFFFU) {
                osal_printk("[cli] usage: role member <leader_mac_suffix>\r\n");
                return;
            }
            ret = team_configure_role(SLE_TEAM_ROLE_MEMBER, team_route_id_from_suffix((uint16_t)suffix));
            if (ret == SLE_TEAM_OK) {
                (void)snprintf(g_team_rt.leader_label, sizeof(g_team_rt.leader_label), "L%04X", suffix);
            }
            osal_printk("[cli] role member leader_suffix=%04X ret=%d\r\n", suffix, ret);
            return;
        }
        if (team_led_cli_handle(msg.line) != 0) {
            return;
        }
        if (g_team_rt.role_configured != 0U) {
            sle_team_cli_handle_line(&g_team_cli, msg.line);
        } else {
            osal_printk("[cli] configure first: role leader | role member <leader_mac_suffix>\r\n");
        }
    }
}

static void *team_network_task(const char *arg)
{
#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
    uint32_t identity_wait_ms = 0U;
#endif

    unused(arg);

    sle_team_web_event_log_init(&g_team_events);
#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
    while (g_team_rt.self_mac_ready == 0U && identity_wait_ms < SLE_TEAM_IDENTITY_WAIT_MAX_MS) {
        osal_msleep(100);
        identity_wait_ms += 100U;
    }
#endif
    team_uart_init();
    team_uart_cli_start();

    osal_printk("[team] boot unconfigured route=%u label=%s ssid=%s\r\n",
        g_team_rt.route_id,
        g_team_rt.self_label,
        g_team_rt.softap_ssid);
    team_restore_web_config();

    while (1) {
        team_handle_cli_queue_once();
        team_handle_role_request_once();
        if (g_team_rt.role_configured != 0U) {
            uint8_t joined_before = g_team_node.joined;

            if ((g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER && g_team_node.cfg.pairing_enabled != 0U) ||
                (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER && sle_uart_client_is_ready() == 0U)) {
                team_led_post_seek_throttled();
            }
            if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER &&
                (g_team_node.cfg.pairing_enabled != 0U || sle_uart_client_connected_count() == 0U)) {
                team_leader_rescan_if_needed(g_team_node.cfg.pairing_enabled != 0U ?
                    "pairing_window" : "no_member");
            }
            if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER && g_team_node.cfg.pairing_enabled != 0U) {
                team_leader_pairing_rotate_connections();
            }
            team_relay_start_client_if_ready();
            team_member_autoselect_parent();
            team_request_sle_rssi();
            sle_team_node_tick(&g_team_node);
            team_leader_rebalance_relays();
            team_leader_route_metrics_update();
            if (g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER && joined_before != 0U && g_team_node.joined == 0U) {
                team_upstream_parent_reset("heartbeat timeout");
                if (g_team_rt.relay_client_started != 0U) {
                    sle_uart_client_force_rescan();
                }
            }
        }
    }

    return NULL;
}

static void team_network_entry(void)
{
    osal_task *task = NULL;

    team_led_start();

#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
    team_identity_set_fallback();
    team_wifi_ap_entry();
#endif

    osal_kthread_lock();
    task = osal_kthread_create((osal_kthread_handler)team_network_task, NULL, "TeamNetworkTask",
        SLE_TEAM_APP_TASK_STACK_SIZE);
    if (task != NULL) {
        osal_kthread_set_priority(task, SLE_TEAM_APP_TASK_PRIO);
    }
    osal_kthread_unlock();
}

app_run(team_network_entry);
