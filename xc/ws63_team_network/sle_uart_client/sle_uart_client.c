/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
 *
 * Description: SLE uart sample of client. \n
 *
 * History: \n
 * 2023-04-03, Create file. \n
 */
#include "common_def.h"
#include "soc_osal.h"
#include "securec.h"
#include "product.h"
#include "bts_le_gap.h"

#include "sle_errcode.h"
#include "sle_device_discovery.h"
#include "sle_connection_manager.h"
#include "sle_team_packet.h"
#include "sle_uart_client.h"
#define SLE_MTU_SIZE_DEFAULT            520
#define SLE_SEEK_INTERVAL_DEFAULT       400
#define SLE_SEEK_WINDOW_DEFAULT         80
#define UUID_16BIT_LEN                  2
#define UUID_128BIT_LEN                 16
#define SLE_UART_TASK_DELAY_MS          1000
#define SLE_UART_WAIT_SLE_CORE_READY_MS 5000
#define SLE_UART_RECV_CNT               1000
#define SLE_UART_LOW_LATENCY_2K         2000
#ifndef SLE_UART_SERVER_NAME
#define SLE_UART_SERVER_NAME            "sle_uart_server"
#endif
#define SLE_UART_CLIENT_LOG             "[sle uart client]"
#define SLE_UART_CLIENT_MAX_CON         8
#define SLE_UART_MEMBER_ID_MAX          254U
#define SLE_UART_BROADCAST_ID           0xFFU

static ssapc_find_service_result_t g_sle_uart_find_service_result = { 0 };
static sle_announce_seek_callbacks_t g_sle_uart_seek_cbk = { 0 };
static ssapc_callbacks_t g_sle_uart_ssapc_cbk = { 0 };
static sle_addr_t g_sle_uart_remote_addr = { 0 };
ssapc_write_param_t g_sle_uart_send_param = { 0 };
static sle_uart_client_seek_filter_cb g_sle_uart_seek_filter = NULL;
static void *g_sle_uart_seek_filter_user_ctx = NULL;

typedef struct {
    uint8_t active;
    uint16_t conn_id;
    uint8_t member_id;
    int8_t rssi;
    sle_addr_t addr;
} sle_uart_client_conn_t;

static sle_uart_client_conn_t g_sle_uart_conns[SLE_UART_CLIENT_MAX_CON];
static uint8_t g_sle_uart_conn_num = 0;
static uint16_t g_sle_uart_last_conn_id = 0;
static uint8_t g_sle_uart_discovery_ready = 0U;

static uint8_t sle_uart_client_buffer_contains(const uint8_t *buf, size_t buf_len,
    const char *needle, size_t needle_len)
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

static uint8_t sle_uart_client_addr_equal(const sle_addr_t *left, const sle_addr_t *right)
{
    if (left == NULL || right == NULL) {
        return 0U;
    }
    return (uint8_t)(left->type == right->type &&
        memcmp(left->addr, right->addr, sizeof(left->addr)) == 0);
}

static sle_uart_client_conn_t *sle_uart_client_find_conn(uint16_t conn_id)
{
    for (uint8_t i = 0; i < SLE_UART_CLIENT_MAX_CON; i++) {
        if (g_sle_uart_conns[i].active != 0U && g_sle_uart_conns[i].conn_id == conn_id) {
            return &g_sle_uart_conns[i];
        }
    }
    return NULL;
}

static sle_uart_client_conn_t *sle_uart_client_find_conn_by_addr(const sle_addr_t *addr)
{
    for (uint8_t i = 0; i < SLE_UART_CLIENT_MAX_CON; i++) {
        if (g_sle_uart_conns[i].active != 0U && sle_uart_client_addr_equal(&g_sle_uart_conns[i].addr, addr) != 0U) {
            return &g_sle_uart_conns[i];
        }
    }
    return NULL;
}

static sle_uart_client_conn_t *sle_uart_client_alloc_conn(uint16_t conn_id)
{
    sle_uart_client_conn_t *conn = sle_uart_client_find_conn(conn_id);
    if (conn != NULL) {
        return conn;
    }
    for (uint8_t i = 0; i < SLE_UART_CLIENT_MAX_CON; i++) {
        if (g_sle_uart_conns[i].active == 0U) {
            (void)memset_s(&g_sle_uart_conns[i], sizeof(g_sle_uart_conns[i]), 0, sizeof(g_sle_uart_conns[i]));
            g_sle_uart_conns[i].active = 1U;
            g_sle_uart_conns[i].conn_id = conn_id;
            g_sle_uart_conns[i].rssi = SLE_TEAM_RSSI_UNKNOWN;
            g_sle_uart_conn_num++;
            return &g_sle_uart_conns[i];
        }
    }
    return NULL;
}

static void sle_uart_client_remove_conn(uint16_t conn_id)
{
    sle_uart_client_conn_t *conn = sle_uart_client_find_conn(conn_id);
    if (conn == NULL) {
        return;
    }
    (void)memset_s(conn, sizeof(*conn), 0, sizeof(*conn));
    conn->rssi = SLE_TEAM_RSSI_UNKNOWN;
    if (g_sle_uart_conn_num > 0U) {
        g_sle_uart_conn_num--;
    }
    if (g_sle_uart_conn_num == 0U) {
        g_sle_uart_discovery_ready = 0U;
        g_sle_uart_send_param.handle = 0U;
    }
}

uint16_t get_g_sle_uart_conn_id(void)
{
    return g_sle_uart_last_conn_id;
}

uint8_t sle_uart_client_has_conn(uint16_t conn_id)
{
    return sle_uart_client_find_conn(conn_id) != NULL ? 1U : 0U;
}

uint8_t sle_uart_client_is_pending_remote_addr(const sle_addr_t *addr)
{
    if (addr == NULL) {
        return 0U;
    }
    return (uint8_t)(memcmp(&g_sle_uart_remote_addr, addr, sizeof(g_sle_uart_remote_addr)) == 0);
}

void sle_uart_client_set_seek_filter(sle_uart_client_seek_filter_cb seek_filter, void *user_ctx)
{
    g_sle_uart_seek_filter = seek_filter;
    g_sle_uart_seek_filter_user_ctx = user_ctx;
}

uint8_t sle_uart_client_is_ready(void)
{
    return (uint8_t)(g_sle_uart_conn_num != 0U && g_sle_uart_discovery_ready != 0U &&
        g_sle_uart_send_param.handle != 0U);
}

uint16_t sle_uart_client_connected_count(void)
{
    return g_sle_uart_conn_num;
}

int8_t sle_uart_client_get_last_rssi(void)
{
    for (uint8_t i = 0; i < SLE_UART_CLIENT_MAX_CON; i++) {
        if (g_sle_uart_conns[i].active != 0U) {
            return g_sle_uart_conns[i].rssi;
        }
    }
    return SLE_TEAM_RSSI_UNKNOWN;
}

errcode_t sle_uart_client_read_remote_rssi(void)
{
    uint8_t requested = 0U;

    for (uint8_t i = 0; i < SLE_UART_CLIENT_MAX_CON; i++) {
        if (g_sle_uart_conns[i].active == 0U) {
            continue;
        }
        if (sle_read_remote_device_rssi(g_sle_uart_conns[i].conn_id) == ERRCODE_SLE_SUCCESS) {
            requested = 1U;
        }
    }
    if (requested == 0U) {
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

void sle_uart_client_force_rescan(void)
{
    if (g_sle_uart_conn_num < SLE_UART_CLIENT_MAX_CON) {
        sle_uart_start_scan();
    }
}

ssapc_write_param_t *get_g_sle_uart_send_param(void)
{
    return &g_sle_uart_send_param;
}

void sle_uart_start_scan(void)
{
    if (g_sle_uart_conn_num >= SLE_UART_CLIENT_MAX_CON) {
        return;
    }
    sle_seek_param_t param = { 0 };
    param.own_addr_type = 0;
    param.filter_duplicates = 0;
    param.seek_filter_policy = 0;
    param.seek_phys = 1;
    param.seek_type[0] = 1;
    param.seek_interval[0] = SLE_SEEK_INTERVAL_DEFAULT;
    param.seek_window[0] = SLE_SEEK_WINDOW_DEFAULT;
    sle_set_seek_param(&param);
    sle_start_seek();
}

static void sle_uart_client_sample_sle_enable_cbk(errcode_t status)
{
    osal_printk("sle enable: %d.\r\n", status);
    sle_uart_client_init(sle_uart_notification_cb, sle_uart_indication_cb);
    sle_uart_start_scan();
}

static void sle_uart_client_sample_seek_enable_cbk(errcode_t status)
{
    if (status != 0) {
        osal_printk("%s sle_uart_client_sample_seek_enable_cbk,status error\r\n", SLE_UART_CLIENT_LOG);
    }
}

static void sle_uart_client_sample_seek_result_info_cbk(sle_seek_result_info_t *seek_result_data)
{
    static const char server_name[] = SLE_UART_SERVER_NAME;
    size_t server_name_len = sizeof(SLE_UART_SERVER_NAME) - 1U;

    if (seek_result_data == NULL) {
        osal_printk("status error\r\n");
    } else if (g_sle_uart_conn_num < SLE_UART_CLIENT_MAX_CON &&
        sle_uart_client_buffer_contains(seek_result_data->data, seek_result_data->data_length,
            server_name, server_name_len) != 0U) {
        if (g_sle_uart_seek_filter != NULL &&
            g_sle_uart_seek_filter(seek_result_data, g_sle_uart_seek_filter_user_ctx) == 0U) {
            return;
        }
        if (sle_uart_client_find_conn_by_addr(&seek_result_data->addr) != NULL) {
            return;
        }
        osal_printk("%s will connect addr:%02x:**:**:**:%02x:%02x count:%u\r\n", SLE_UART_CLIENT_LOG,
            seek_result_data->addr.addr[0], seek_result_data->addr.addr[4],
            seek_result_data->addr.addr[5], g_sle_uart_conn_num);
        memcpy_s(&g_sle_uart_remote_addr, sizeof(sle_addr_t), &seek_result_data->addr, sizeof(sle_addr_t));
        sle_stop_seek();
    }
}

static void sle_uart_client_sample_seek_disable_cbk(errcode_t status)
{
    if (status != 0) {
        osal_printk("%s sle_uart_client_sample_seek_disable_cbk,status error = %x\r\n", SLE_UART_CLIENT_LOG, status);
    } else {
        sle_connect_remote_device(&g_sle_uart_remote_addr);
    }
}

static void sle_uart_client_sample_seek_cbk_register(void)
{
    g_sle_uart_seek_cbk.sle_enable_cb = sle_uart_client_sample_sle_enable_cbk;
    g_sle_uart_seek_cbk.seek_enable_cb = sle_uart_client_sample_seek_enable_cbk;
    g_sle_uart_seek_cbk.seek_result_cb = sle_uart_client_sample_seek_result_info_cbk;
    g_sle_uart_seek_cbk.seek_disable_cb = sle_uart_client_sample_seek_disable_cbk;
    sle_announce_seek_register_callbacks(&g_sle_uart_seek_cbk);
}

void sle_uart_client_handle_connect_state_changed(uint16_t conn_id, const sle_addr_t *addr,
    sle_acb_state_t conn_state, sle_pair_state_t pair_state, sle_disc_reason_t disc_reason)
{
    osal_printk("%s conn state changed disc_reason:0x%x\r\n", SLE_UART_CLIENT_LOG, disc_reason);
    if (conn_state == SLE_ACB_STATE_CONNECTED) {
        sle_uart_client_conn_t *conn = sle_uart_client_alloc_conn(conn_id);
        osal_printk("%s SLE_ACB_STATE_CONNECTED\r\n", SLE_UART_CLIENT_LOG);
        if (conn == NULL) {
            if (addr != NULL) {
                (void)sle_disconnect_remote_device(addr);
            }
            return;
        }
        if (addr != NULL) {
            (void)memcpy_s(&conn->addr, sizeof(conn->addr), addr, sizeof(*addr));
        }
        g_sle_uart_last_conn_id = conn_id;
        (void)sle_read_remote_device_rssi(conn_id);
        if (pair_state == SLE_PAIR_NONE) {
            sle_pair_remote_device(addr != NULL ? addr : &g_sle_uart_remote_addr);
        }
        ssap_exchange_info_t info = {0};
        info.mtu_size = SLE_MTU_SIZE_DEFAULT;
        info.version = 1;
        ssapc_exchange_info_req(0, conn_id, &info);
        sle_uart_start_scan();
    } else if (conn_state == SLE_ACB_STATE_NONE) {
        osal_printk("%s SLE_ACB_STATE_NONE\r\n", SLE_UART_CLIENT_LOG);
    } else if (conn_state == SLE_ACB_STATE_DISCONNECTED) {
        osal_printk("%s SLE_ACB_STATE_DISCONNECTED\r\n", SLE_UART_CLIENT_LOG);
        sle_uart_client_remove_conn(conn_id);
        sle_uart_start_scan();
    } else {
        osal_printk("%s status error\r\n", SLE_UART_CLIENT_LOG);
    }
}

void sle_uart_client_handle_read_rssi(uint16_t conn_id, int8_t rssi, errcode_t status)
{
    sle_uart_client_conn_t *conn = sle_uart_client_find_conn(conn_id);

    if (status == ERRCODE_SLE_SUCCESS && conn != NULL) {
        conn->rssi = rssi;
        osal_printk("%s rssi conn_id:%d rssi:%d\r\n", SLE_UART_CLIENT_LOG, conn_id, rssi);
    }
}

void sle_uart_client_handle_pair_complete(uint16_t conn_id, const sle_addr_t *addr, errcode_t status)
{
    if (addr != NULL) {
        osal_printk("%s pair complete conn_id:%d, addr:%02x***%02x%02x\n", SLE_UART_CLIENT_LOG, conn_id,
                    addr->addr[0], addr->addr[4], addr->addr[5]);
    } else {
        osal_printk("%s pair complete conn_id:%d, addr:null\n", SLE_UART_CLIENT_LOG, conn_id);
    }
    if (status == 0) {
        ssap_exchange_info_t info = {0};
        info.mtu_size = SLE_MTU_SIZE_DEFAULT;
        info.version = 1;
        ssapc_exchange_info_req(0, conn_id, &info);
    }
}

static void sle_uart_client_sample_exchange_info_cbk(uint8_t client_id, uint16_t conn_id, ssap_exchange_info_t *param,
                                                     errcode_t status)
{
    osal_printk("%s exchange_info_cbk,pair complete client id:%d status:%d\r\n",
                SLE_UART_CLIENT_LOG, client_id, status);
    osal_printk("%s exchange mtu, mtu size: %d, version: %d.\r\n", SLE_UART_CLIENT_LOG,
                param->mtu_size, param->version);
    ssapc_find_structure_param_t find_param = { 0 };
    find_param.type = SSAP_FIND_TYPE_PROPERTY;
    find_param.start_hdl = 1;
    find_param.end_hdl = 0xFFFF;
    ssapc_find_structure(0, conn_id, &find_param);
}

static void sle_uart_client_sample_find_structure_cbk(uint8_t client_id, uint16_t conn_id,
                                                      ssapc_find_service_result_t *service,
                                                      errcode_t status)
{
    osal_printk("%s find structure cbk client: %d conn_id:%d status: %d \r\n", SLE_UART_CLIENT_LOG,
                client_id, conn_id, status);
    osal_printk("%s find structure start_hdl:[0x%02x], end_hdl:[0x%02x], uuid len:%d\r\n", SLE_UART_CLIENT_LOG,
                service->start_hdl, service->end_hdl, service->uuid.len);
    g_sle_uart_find_service_result.start_hdl = service->start_hdl;
    g_sle_uart_find_service_result.end_hdl = service->end_hdl;
    memcpy_s(&g_sle_uart_find_service_result.uuid, sizeof(sle_uuid_t), &service->uuid, sizeof(sle_uuid_t));
}

static void sle_uart_client_sample_find_property_cbk(uint8_t client_id, uint16_t conn_id,
                                                     ssapc_find_property_result_t *property, errcode_t status)
{
    osal_printk("%s sle_uart_client_sample_find_property_cbk, client id: %d, conn id: %d, operate ind: %d, "
                "descriptors count: %d status:%d property->handle %d\r\n", SLE_UART_CLIENT_LOG,
                client_id, conn_id, property->operate_indication,
                property->descriptors_count, status, property->handle);
    g_sle_uart_send_param.handle = property->handle;
    g_sle_uart_send_param.type = SSAP_PROPERTY_TYPE_VALUE;
    g_sle_uart_discovery_ready = 1U;
}

static void sle_uart_client_sample_find_structure_cmp_cbk(uint8_t client_id, uint16_t conn_id,
                                                          ssapc_find_structure_result_t *structure_result,
                                                          errcode_t status)
{
    unused(conn_id);
    osal_printk("%s sle_uart_client_sample_find_structure_cmp_cbk,client id:%d status:%d type:%d uuid len:%d \r\n",
                SLE_UART_CLIENT_LOG, client_id, status, structure_result->type, structure_result->uuid.len);
}

static void sle_uart_client_sample_write_cfm_cb(uint8_t client_id, uint16_t conn_id,
                                                ssapc_write_result_t *write_result, errcode_t status)
{
    osal_printk("%s sle_uart_client_sample_write_cfm_cb, conn_id:%d client id:%d status:%d handle:%02x type:%02x\r\n",
                SLE_UART_CLIENT_LOG, conn_id, client_id, status, write_result->handle, write_result->type);
}

static void sle_uart_client_sample_ssapc_cbk_register(ssapc_notification_callback notification_cb,
                                                      ssapc_notification_callback indication_cb)
{
    g_sle_uart_ssapc_cbk.exchange_info_cb = sle_uart_client_sample_exchange_info_cbk;
    g_sle_uart_ssapc_cbk.find_structure_cb = sle_uart_client_sample_find_structure_cbk;
    g_sle_uart_ssapc_cbk.ssapc_find_property_cbk = sle_uart_client_sample_find_property_cbk;
    g_sle_uart_ssapc_cbk.find_structure_cmp_cb = sle_uart_client_sample_find_structure_cmp_cbk;
    g_sle_uart_ssapc_cbk.write_cfm_cb = sle_uart_client_sample_write_cfm_cb;
    g_sle_uart_ssapc_cbk.notification_cb = notification_cb;
    g_sle_uart_ssapc_cbk.indication_cb = indication_cb;
    ssapc_register_callbacks(&g_sle_uart_ssapc_cbk);
}


void sle_uart_client_init(ssapc_notification_callback notification_cb, ssapc_indication_callback indication_cb)
{
    (void)osal_msleep(5000); /* 延时5s，等待SLE初始化完毕 */
    osal_printk("[SLE Client] try enable.\r\n");
    sle_uart_client_sample_seek_cbk_register();
    sle_uart_client_sample_ssapc_cbk_register(notification_cb, indication_cb);
    if (enable_sle() != ERRCODE_SUCC) {
        osal_printk("[SLE Client] sle enbale fail !\r\n");
    }
}

errcode_t sle_uart_client_send_by_conn(uint16_t conn_id, const uint8_t *data, uint16_t len)
{
    ssapc_write_param_t *param = get_g_sle_uart_send_param();

    if (param == NULL || data == NULL || len == 0U || g_sle_uart_discovery_ready == 0U ||
        sle_uart_client_find_conn(conn_id) == NULL) {
        return ERRCODE_SLE_FAIL;
    }
    param->data_len = len;
    param->data = (uint8_t *)data;
    return ssapc_write_req(0, conn_id, param);
}

errcode_t sle_uart_client_send_all(const uint8_t *data, uint16_t len)
{
    uint8_t sent = 0U;
    errcode_t first_fail = ERRCODE_SLE_FAIL;

    for (uint8_t i = 0; i < SLE_UART_CLIENT_MAX_CON; i++) {
        if (g_sle_uart_conns[i].active == 0U) {
            continue;
        }
        errcode_t ret = sle_uart_client_send_by_conn(g_sle_uart_conns[i].conn_id, data, len);
        if (ret == ERRCODE_SLE_SUCCESS) {
            sent = 1U;
        } else if (first_fail == ERRCODE_SLE_FAIL) {
            first_fail = ret;
        }
    }
    return sent != 0U ? ERRCODE_SLE_SUCCESS : first_fail;
}

void sle_uart_client_bind_member_conn(uint8_t member_id, uint16_t conn_id)
{
    sle_uart_client_conn_t *conn;

    if (member_id == 0U || member_id > SLE_UART_MEMBER_ID_MAX) {
        return;
    }
    conn = sle_uart_client_find_conn(conn_id);
    if (conn == NULL) {
        return;
    }
    for (uint8_t i = 0; i < SLE_UART_CLIENT_MAX_CON; i++) {
        if (g_sle_uart_conns[i].active != 0U && g_sle_uart_conns[i].conn_id != conn_id &&
            g_sle_uart_conns[i].member_id == member_id) {
            g_sle_uart_conns[i].member_id = 0U;
        }
    }
    conn->member_id = member_id;
    osal_printk("%s bind member:%u conn_id:%u\r\n", SLE_UART_CLIENT_LOG, member_id, conn_id);
}

uint8_t sle_uart_client_find_conn_by_member(uint8_t member_id, uint16_t *conn_id)
{
    if (member_id == 0U || member_id == SLE_UART_BROADCAST_ID || conn_id == NULL) {
        return 0U;
    }
    for (uint8_t i = 0; i < SLE_UART_CLIENT_MAX_CON; i++) {
        if (g_sle_uart_conns[i].active != 0U && g_sle_uart_conns[i].member_id == member_id) {
            *conn_id = g_sle_uart_conns[i].conn_id;
            return 1U;
        }
    }
    return 0U;
}
