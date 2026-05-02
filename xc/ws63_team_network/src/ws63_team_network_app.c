#include "sle_team_cli.h"
#include "sle_team_web_api.h"
#include "ws63_console_pages.h"

#include <errno.h>
#include <stdarg.h>
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
#include "sle_ssap_server.h"
#include "sle_uart_server.h"
#include "sle_ssap_client.h"
#include "sle_uart_client.h"

#ifndef CONFIG_SLE_TEAM_SELF_ID
#if defined(CONFIG_SLE_TEAM_NODE_IS_LEADER)
#define CONFIG_SLE_TEAM_SELF_ID 1
#else
#define CONFIG_SLE_TEAM_SELF_ID 2
#endif
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
#define CONFIG_SLE_TEAM_LED_ACTIVE_LOW 1
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

#define SLE_TEAM_WIFI_AP_TASK_STACK_SIZE 0x1000
#define SLE_TEAM_WIFI_AP_TASK_PRIO 13
#define SLE_TEAM_WIFI_IFNAME_MAX_SIZE 16
#define SLE_TEAM_WIFI_INIT_WAIT_MAX_MS 10000
#define SLE_TEAM_TCPIP_INIT_WAIT_MAX_MS 10000
#define SLE_TEAM_WIFI_AP_RETRY_MS 5000
#define SLE_TEAM_HTTP_START_DELAY_MS 5000
#define SLE_TEAM_HTTP_RETRY_MS 5000
#define SLE_TEAM_HTTP_PORT 80
#define SLE_TEAM_HTTP_BACKLOG 2
#define SLE_TEAM_HTTP_REQ_BUF_SIZE 192
#define SLE_TEAM_HTTP_JSON_BUF_SIZE 1024
#define SLE_TEAM_HTTP_HTML_BUF_SIZE 3072

typedef struct {
    char line[SLE_TEAM_CLI_LINE_SIZE];
} sle_team_cli_msg_t;

typedef enum {
    SLE_TEAM_LED_EVENT_TX = 1,
    SLE_TEAM_LED_EVENT_RX = 2,
} sle_team_led_event_t;

typedef struct {
    uint8_t uart_rx_buf[SLE_TEAM_UART_RX_BUF_SIZE];
    char line_buf[SLE_TEAM_CLI_LINE_SIZE];
    uint16_t line_len;
    unsigned long cli_queue_id;
    unsigned long led_queue_id;
    uint8_t cli_queue_ready;
    uint8_t led_queue_ready;
} sle_team_ws63_runtime_t;

static sle_team_ws63_runtime_t g_team_rt;
static sle_team_node_t g_team_node;
static sle_team_cli_t g_team_cli;
static sle_team_web_event_log_t g_team_events;

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

static void team_print(const char *text)
{
    osal_printk("[state] %s\r\n", text);
}

static void team_led_set(uint8_t on)
{
    if (CONFIG_SLE_TEAM_LED_ACTIVE_LOW) {
        (void)uapi_gpio_set_val(CONFIG_SLE_TEAM_LED_PIN, on != 0U ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH);
    } else {
        (void)uapi_gpio_set_val(CONFIG_SLE_TEAM_LED_PIN, on != 0U ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
    }
}

static void team_led_post(sle_team_led_event_t event)
{
    uint8_t msg = (uint8_t)event;

    if (g_team_rt.led_queue_ready == 0U) {
        return;
    }
    (void)osal_msg_queue_write_copy(g_team_rt.led_queue_id, &msg, (uint32_t)sizeof(msg), 0);
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

    (void)uapi_pin_set_mode(CONFIG_SLE_TEAM_LED_PIN, HAL_PIO_FUNC_GPIO);
    (void)uapi_gpio_set_dir(CONFIG_SLE_TEAM_LED_PIN, GPIO_DIRECTION_OUTPUT);
    team_led_set(0U);
    osal_printk("[state] led pin=%u active_low=%u\r\n",
        CONFIG_SLE_TEAM_LED_PIN, CONFIG_SLE_TEAM_LED_ACTIVE_LOW ? 1U : 0U);

    while (1) {
        msg_size = sizeof(event);
        if (osal_msg_queue_read_copy(g_team_rt.led_queue_id, &event, &msg_size, SLE_TEAM_LED_QUEUE_TIMEOUT_MS) !=
            OSAL_SUCCESS) {
            continue;
        }
        if (event == (uint8_t)SLE_TEAM_LED_EVENT_TX) {
            team_led_blink(SLE_TEAM_LED_TX_PULSES, SLE_TEAM_LED_TX_ON_MS, SLE_TEAM_LED_TX_OFF_MS);
        } else if (event == (uint8_t)SLE_TEAM_LED_EVENT_RX) {
            team_led_blink(SLE_TEAM_LED_RX_PULSES, SLE_TEAM_LED_RX_ON_MS, SLE_TEAM_LED_RX_OFF_MS);
        }
    }
    return NULL;
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

static void team_wifi_print_status(void)
{
    osal_printk("[team-wifi] status wifi_inited=%ld softap_enabled=%ld ssid=%s ip=192.168.43.%u\r\n",
        (long)wifi_is_wifi_inited(),
        (long)wifi_is_softap_enabled(),
        CONFIG_SLE_TEAM_WIFI_AP_SSID,
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
    osal_printk("[team-wifi] http ready=%u fd=%d errno=%d accepts=%lu tcpip=%ld\r\n",
        g_team_http_ready,
        g_team_http_listen_fd,
        g_team_http_last_errno,
        (unsigned long)g_team_http_accept_count,
        (long)tcpip_init_finish);
}
#endif

static uint32_t team_now_s(void *user_ctx)
{
    unused(user_ctx);
    return (uint32_t)(uapi_tcxo_get_ms() / 1000U);
}

static void team_log(void *user_ctx, const char *text)
{
    unused(user_ctx);
    osal_printk("[state] %s\r\n", text);
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
    unused(user_ctx);
    osal_printk("[team] joined member=%u\r\n", member_id);
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
    IP4_ADDR(&gw, 192, 168, 43, 2);

    (void)snprintf((char *)ap_config.ssid, sizeof(ap_config.ssid), "%s", CONFIG_SLE_TEAM_WIFI_AP_SSID);
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
        CONFIG_SLE_TEAM_WIFI_AP_SSID,
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

static void team_http_send_response(int fd, const char *status, const char *content_type, const char *body)
{
    char header[192];
    size_t body_len = body != NULL ? strlen(body) : 0U;
    int header_len;

    header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %u\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-store\r\n"
        "\r\n",
        status, content_type, (unsigned int)body_len);
    if (header_len > 0) {
        int send_ret = team_http_send_all(fd, header, (size_t)header_len);
        osal_printk("[team-wifi] http header sent fd=%d len=%d ret=%d\r\n", fd, header_len, send_ret);
    }
    if (body_len > 0U) {
        int send_ret = team_http_send_all(fd, body, body_len);
        osal_printk("[team-wifi] http body sent fd=%d len=%u ret=%d\r\n", fd, (unsigned int)body_len, send_ret);
    }
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
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
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
        "a{font:inherit;border:1px solid #c9d0da;border-radius:6px;background:white;color:#182230;padding:8px 10px;text-decoration:none}"
        "a.on{background:#182230;color:#fff;border-color:#182230}",
        "</style></head><body><header><h1>" WS63_CONSOLE_BOARD_TITLE "</h1>"
        "<div class=\"sub\">" WS63_CONSOLE_BOARD_SUBTITLE "</div></header><main>"
    };
    size_t i;

    buf[0] = '\0';
    *used = 0U;
    for (i = 0; i < sizeof(chunks) / sizeof(chunks[0]); i++) {
        team_http_append_str(buf, buf_size, used, chunks[i]);
    }
    team_http_append_fmt(buf, buf_size, used,
        "<div class=\"bar\"><a class=\"%s\" href=\"" WS63_CONSOLE_TAB_STATUS_HREF "\">"
        WS63_CONSOLE_TAB_STATUS_LABEL "</a><a class=\"%s\" href=\"" WS63_CONSOLE_TAB_NODES_HREF "\">"
        WS63_CONSOLE_TAB_NODES_LABEL "</a><a class=\"%s\" href=\"" WS63_CONSOLE_TAB_EVENTS_HREF "\">"
        WS63_CONSOLE_TAB_EVENTS_LABEL "</a><a href=\"" WS63_CONSOLE_TAB_JSON_HREF "\">"
        WS63_CONSOLE_TAB_JSON_LABEL "</a></div>",
        strcmp(active, "status") == 0 ? "on" : "",
        strcmp(active, "nodes") == 0 ? "on" : "",
        strcmp(active, "events") == 0 ? "on" : "");
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
        "</span><span class=\"v\">%u</span></div>",
        g_team_node.cfg.self_id);
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_LEADER_LABEL
        "</span><span class=\"v\">%u</span></div>",
        g_team_node.cfg.leader_id);
    team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_STATUS_JOINED_LABEL
        "</span><span class=\"v\">%s</span></div>",
        g_team_node.joined != 0U ? "true" : "false");
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

    team_http_append_html_shell_start(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used, "nodes");
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
        "<section class=\"card\"><h2 style=\"font-size:16px;margin:0 0 10px\">Nodes</h2>");
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &g_team_node.members[i];
        if (member->online == 0U) {
            continue;
        }
        wrote = 1U;
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_NODE_NODE_LABEL
            "</span><span class=\"v\">%u</span></div>",
            member->member_id);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_NODE_BATTERY_LABEL
            "</span><span class=\"v\">%u%%</span></div>",
            member->battery_percent);
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">" WS63_CONSOLE_NODE_RSSI_LABEL
            "</span><span class=\"v\">%d</span></div>",
            member->last_rssi_dbm);
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
        team_http_append_fmt(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used,
            "<div class=\"row\"><span class=\"k\">%lu %s</span><span class=\"v\">%u-%u #%u</span></div>",
            (unsigned long)event->time_s, sle_team_web_msg_type_name(event->app_msg_type),
            event->src_id, event->dst_id, event->seq);
    }
    team_http_append_str(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used, "</section>");
    team_http_append_html_end(g_team_http_html_buf, sizeof(g_team_http_html_buf), &used);
    team_http_send_response(fd, "200 OK", "text/html; charset=utf-8", g_team_http_html_buf);
}

static void team_http_handle_client(int fd)
{
    int ret;

    ret = recv(fd, g_team_http_req_buf, sizeof(g_team_http_req_buf) - 1U, 0);
    if (ret <= 0) {
        g_team_http_last_errno = errno;
        osal_printk("[team-wifi] http recv failed fd=%d ret=%d errno=%d\r\n", fd, ret, g_team_http_last_errno);
        return;
    }
    g_team_http_req_buf[ret] = '\0';
    osal_printk("[team-wifi] http recv fd=%d len=%d line=%s\r\n", fd, ret, g_team_http_req_buf);

    if (strncmp(g_team_http_req_buf, "GET /api/status", 15) == 0) {
        osal_printk("[team-wifi] http route api=status\r\n");
        ret = sle_team_web_write_status_json(&g_team_node, team_now_s(NULL), "ws63-softap", g_team_http_json_buf,
            sizeof(g_team_http_json_buf));
        team_http_send_response(fd, ret < 0 ? "500 Internal Server Error" : "200 OK", "application/json",
            ret < 0 ? "{\"error\":\"status\"}" : g_team_http_json_buf);
    } else if (strncmp(g_team_http_req_buf, "GET /api/nodes", 14) == 0) {
        osal_printk("[team-wifi] http route api=nodes\r\n");
        ret = sle_team_web_write_nodes_json(&g_team_node, g_team_http_json_buf, sizeof(g_team_http_json_buf));
        team_http_send_response(fd, ret < 0 ? "500 Internal Server Error" : "200 OK", "application/json",
            ret < 0 ? "{\"error\":\"nodes\"}" : g_team_http_json_buf);
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

static int team_sle_send(void *user_ctx, sle_team_send_kind_t kind, uint8_t dst_id, const uint8_t *buf, uint16_t len)
{
    unused(user_ctx);
    unused(kind);
    unused(dst_id);

    if (buf == NULL || len == 0U) {
        return SLE_TEAM_ERR_ARG;
    }

    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        if (sle_uart_client_is_connected() == 0U) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NO_MEMBER\r\n",
                dst_id, SLE_TEAM_ERR_UNSUPPORTED);
            return SLE_TEAM_ERR_UNSUPPORTED;
        }
        if (sle_uart_server_send_report_by_handle(buf, len) != ERRCODE_SLE_SUCCESS) {
            osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=WRITE_FAIL\r\n",
                dst_id, SLE_TEAM_ERR_FORMAT);
            return SLE_TEAM_ERR_FORMAT;
        }
        team_web_record_packet(SLE_TEAM_WEB_EVENT_TX, buf, len, "leader tx");
        return SLE_TEAM_OK;
    }

    ssapc_write_param_t *param = get_g_sle_uart_send_param();
    uint16_t conn_id = get_g_sle_uart_conn_id();
    if (param == NULL || param->handle == 0U) {
        osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=NOT_READY\r\n",
            dst_id, SLE_TEAM_ERR_UNSUPPORTED);
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    param->data_len = len;
    param->data = (uint8_t *)buf;
    if (ssapc_write_req(0, conn_id, param) != ERRCODE_SLE_SUCCESS) {
        osal_printk("[sle-tx-fail] type=PACKET dst=%u ret=%d reason=WRITE_FAIL\r\n", dst_id, SLE_TEAM_ERR_FORMAT);
        return SLE_TEAM_ERR_FORMAT;
    }
    team_web_record_packet(SLE_TEAM_WEB_EVENT_TX, buf, len, "member tx");
    return SLE_TEAM_OK;
}

static void team_node_init(void)
{
    sle_team_node_cfg_t cfg;
    sle_team_node_ops_t ops;

    (void)memset_s(&cfg, sizeof(cfg), 0, sizeof(cfg));
    (void)memset_s(&ops, sizeof(ops), 0, sizeof(ops));

    cfg.team_id = CONFIG_SLE_TEAM_TEAM_ID;
    cfg.self_id = CONFIG_SLE_TEAM_SELF_ID;
    cfg.leader_id = CONFIG_SLE_TEAM_LEADER_ID;
#if defined(CONFIG_SLE_TEAM_NODE_IS_LEADER)
    cfg.role = SLE_TEAM_ROLE_LEADER;
#else
    cfg.role = SLE_TEAM_ROLE_MEMBER;
#endif
    cfg.channel_hash = CONFIG_SLE_TEAM_CHANNEL_HASH;
    cfg.report_interval_s = CONFIG_SLE_TEAM_REPORT_INTERVAL_S;
    cfg.heartbeat_interval_s = CONFIG_SLE_TEAM_HEARTBEAT_INTERVAL_S;
    cfg.warn_distance_m = CONFIG_SLE_TEAM_WARN_DISTANCE_M;
    cfg.lost_distance_m = CONFIG_SLE_TEAM_LOST_DISTANCE_M;
    cfg.heartbeat_timeout_s = CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S;

    ops.send = team_sle_send;
    ops.now_s = team_now_s;
    ops.log = team_log;
    ops.on_joined = team_joined;
    ops.on_position = team_position;
    ops.on_alert = team_alert;

    (void)sle_team_node_init(&g_team_node, &cfg, &ops);
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
    unused(server_id);
    unused(conn_id);
    if (status != ERRCODE_SLE_SUCCESS || write_cb_para == NULL || write_cb_para->value == NULL) {
        return;
    }
    team_web_record_packet(SLE_TEAM_WEB_EVENT_RX, write_cb_para->value, write_cb_para->length, "leader rx");
    (void)sle_team_node_on_packet(&g_team_node, write_cb_para->value, write_cb_para->length);
}

static void team_client_rx_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    unused(client_id);
    unused(conn_id);
    if (status != ERRCODE_SLE_SUCCESS || data == NULL || data->data == NULL) {
        return;
    }
    team_web_record_packet(SLE_TEAM_WEB_EVENT_RX, data->data, data->data_len, "member rx");
    (void)sle_team_node_on_packet(&g_team_node, data->data, data->data_len);
}

void sle_uart_notification_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    team_client_rx_cb(client_id, conn_id, data, status);
}

void sle_uart_indication_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    team_client_rx_cb(client_id, conn_id, data, status);
}

static void team_sle_start(void)
{
    if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER) {
        if (sle_uart_server_init(team_server_read_cb, team_server_write_cb) == ERRCODE_SLE_SUCCESS) {
            team_print("leader sle server ready");
        } else {
            team_print("leader sle server init failed");
        }
        return;
    }

    sle_uart_client_init(team_client_rx_cb, team_client_rx_cb);
    team_print("member sle client started");
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
        sle_team_cli_handle_line(&g_team_cli, msg.line);
    }
}

static void *team_network_task(const char *arg)
{
    unused(arg);

    sle_team_web_event_log_init(&g_team_events);
    team_node_init();
    team_uart_init();
    team_uart_cli_start();
    team_sle_start();

    sle_team_cli_print_help(&g_team_cli);
    osal_printk("[team] boot self=%u leader=%u role=%u team=%u\r\n",
        g_team_node.cfg.self_id,
        g_team_node.cfg.leader_id,
        g_team_node.cfg.role,
        g_team_node.cfg.team_id);

    while (1) {
        team_handle_cli_queue_once();
        sle_team_node_tick(&g_team_node);
    }

    return NULL;
}

static void team_network_entry(void)
{
    osal_task *task = NULL;

    team_led_start();

#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
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
