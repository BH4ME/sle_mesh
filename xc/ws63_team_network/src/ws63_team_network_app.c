#include "sle_team_cli.h"

#include <stdio.h>
#include <string.h>

#include "app_init.h"
#include "common_def.h"
#include "errcode.h"
#include "pinctrl.h"
#include "securec.h"
#include "soc_osal.h"
#include "tcxo.h"
#include "uart.h"

#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
#include "lwip/netifapi.h"
#include "wifi_hotspot.h"
#include "wifi_hotspot_config.h"
#endif

#include "sle_errcode.h"
#if defined(CONFIG_SLE_TEAM_NODE_IS_LEADER)
#include "sle_device_discovery.h"
#include "sle_ssap_server.h"
#include "sle_uart_server.h"
#else
#include "sle_ssap_client.h"
#include "sle_uart_client.h"
#endif

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

#define SLE_TEAM_WIFI_AP_TASK_STACK_SIZE 0x1000
#define SLE_TEAM_WIFI_AP_TASK_PRIO 13
#define SLE_TEAM_WIFI_IFNAME_MAX_SIZE 16

typedef struct {
    char line[SLE_TEAM_CLI_LINE_SIZE];
} sle_team_cli_msg_t;

typedef struct {
    uint8_t uart_rx_buf[SLE_TEAM_UART_RX_BUF_SIZE];
    char line_buf[SLE_TEAM_CLI_LINE_SIZE];
    uint16_t line_len;
    unsigned long cli_queue_id;
    uint8_t cli_queue_ready;
} sle_team_ws63_runtime_t;

static sle_team_ws63_runtime_t g_team_rt;
static sle_team_node_t g_team_node;
static sle_team_cli_t g_team_cli;

static uart_buffer_config_t g_uart_buffer_config = {
    .rx_buffer = g_team_rt.uart_rx_buf,
    .rx_buffer_size = SLE_TEAM_UART_RX_BUF_SIZE,
};

static void team_print(const char *text)
{
    osal_printk("[team] %s\r\n", text);
}

#if defined(CONFIG_SLE_TEAM_WIFI_AP_ENABLE)
static void team_wifi_print(const char *text)
{
    osal_printk("[team-wifi] %s\r\n", text);
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
    team_print(text);
}

static void team_cli_print(void *user_ctx, const char *text)
{
    unused(user_ctx);
    osal_printk("[cli] %s\r\n", text);
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

    IP4_ADDR(&ipaddr, 192, 168, 43, CONFIG_SLE_TEAM_WIFI_AP_IP_LAST);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 43, CONFIG_SLE_TEAM_WIFI_AP_IP_LAST);

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

    if (wifi_set_softap_config_advance(&advance_config) != 0) {
        team_wifi_print("softap advance config failed");
        return -1;
    }
    if (wifi_softap_enable(&ap_config) != 0) {
        team_wifi_print("softap enable failed");
        return -1;
    }

    netif_p = netif_find(ifname);
    if (netif_p == NULL) {
        team_wifi_print("softap netif ap0 not found");
        (void)wifi_softap_disable();
        return -1;
    }
    if (netifapi_netif_set_addr(netif_p, &ipaddr, &netmask, &gw) != 0) {
        team_wifi_print("softap set ip failed");
        (void)wifi_softap_disable();
        return -1;
    }
    if (netifapi_dhcps_start(netif_p, NULL, 0) != 0) {
        team_wifi_print("softap dhcp start failed");
        (void)wifi_softap_disable();
        return -1;
    }

    osal_printk("[team-wifi] softap started ssid=%s ip=192.168.43.%u channel=%u\r\n",
        CONFIG_SLE_TEAM_WIFI_AP_SSID,
        CONFIG_SLE_TEAM_WIFI_AP_IP_LAST,
        CONFIG_SLE_TEAM_WIFI_AP_CHANNEL);
    return 0;
}

static void *team_wifi_ap_task(const char *arg)
{
    unused(arg);

    while (wifi_is_wifi_inited() == 0) {
        osal_msleep(100);
    }
    team_wifi_print("wifi init ready");
    if (team_wifi_ap_start() != 0) {
        team_wifi_print("softap start failed");
    }
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

#if defined(CONFIG_SLE_TEAM_NODE_IS_LEADER)
    if (sle_uart_client_is_connected() == 0U) {
        team_print("leader has no connected member");
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    if (sle_uart_server_send_report_by_handle(buf, len) != ERRCODE_SLE_SUCCESS) {
        team_print("leader send failed");
        return SLE_TEAM_ERR_FORMAT;
    }
    return SLE_TEAM_OK;
#else
    ssapc_write_param_t *param = get_g_sle_uart_send_param();
    uint16_t conn_id = get_g_sle_uart_conn_id();
    if (param == NULL || param->handle == 0U) {
        team_print("member not ready to send");
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    param->data_len = len;
    param->data = (uint8_t *)buf;
    if (ssapc_write_req(0, conn_id, param) != ERRCODE_SLE_SUCCESS) {
        team_print("member send failed");
        return SLE_TEAM_ERR_FORMAT;
    }
    return SLE_TEAM_OK;
#endif
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

#if defined(CONFIG_SLE_TEAM_NODE_IS_LEADER)
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
    (void)sle_team_node_on_packet(&g_team_node, write_cb_para->value, write_cb_para->length);
}

static void team_sle_start(void)
{
    if (sle_uart_server_init(team_server_read_cb, team_server_write_cb) == ERRCODE_SLE_SUCCESS) {
        team_print("leader sle server ready");
    } else {
        team_print("leader sle server init failed");
    }
}
#else
static void team_client_rx_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    unused(client_id);
    unused(conn_id);
    if (status != ERRCODE_SLE_SUCCESS || data == NULL || data->data == NULL) {
        return;
    }
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
    sle_uart_client_init(team_client_rx_cb, team_client_rx_cb);
    team_print("member sle client started");
}
#endif

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
        sle_team_cli_handle_line(&g_team_cli, msg.line);
    }
}

static void *team_network_task(const char *arg)
{
    unused(arg);

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
