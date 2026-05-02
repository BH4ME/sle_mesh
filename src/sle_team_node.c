#include "sle_team_node.h"

#include <string.h>

static const uint8_t g_zero_cipher_mac[2] = {0x00, 0x00};

static uint32_t sle_team_now(const sle_team_node_t *node)
{
    if (node->ops.now_s == NULL) {
        return 0U;
    }
    return node->ops.now_s(node->ops.user_ctx);
}

static void sle_team_log(const sle_team_node_t *node, const char *text)
{
    if (node->ops.log != NULL) {
        node->ops.log(node->ops.user_ctx, text);
    }
}

static sle_team_member_record_t *sle_team_get_member_slot(sle_team_node_t *node, uint8_t member_id, uint8_t create)
{
    uint8_t i;
    sle_team_member_record_t *free_slot = NULL;

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->members[i].online != 0U && node->members[i].member_id == member_id) {
            return &node->members[i];
        }
        if (free_slot == NULL && node->members[i].online == 0U) {
            free_slot = &node->members[i];
        }
    }

    if (create != 0U && free_slot != NULL) {
        (void)memset(free_slot, 0, sizeof(*free_slot));
        free_slot->member_id = member_id;
        free_slot->online = 1U;
        return free_slot;
    }
    return NULL;
}

static int sle_team_send_app(sle_team_node_t *node, uint8_t dst_id, const uint8_t *app_buf, uint16_t app_len)
{
    sle_team_mesh_packet_t mesh_packet;
    uint8_t mesh_buf[SLE_TEAM_NODE_TX_BUF_SIZE];
    size_t mesh_len = 0U;

    if (node == NULL || node->ops.send == NULL || app_buf == NULL) {
        return SLE_TEAM_ERR_ARG;
    }

    if (sle_team_wrap_mesh_group_data(node->cfg.channel_hash, g_zero_cipher_mac, app_buf, app_len,
        dst_id == SLE_TEAM_BROADCAST_ID ? SLE_TEAM_ROUTE_FLOOD : SLE_TEAM_ROUTE_DIRECT, &mesh_packet) != SLE_TEAM_OK) {
        return SLE_TEAM_ERR_FORMAT;
    }

    if (sle_team_encode_mesh_packet(&mesh_packet, mesh_buf, sizeof(mesh_buf), &mesh_len) != SLE_TEAM_OK) {
        return SLE_TEAM_ERR_BUF;
    }

    return node->ops.send(node->ops.user_ctx,
        dst_id == SLE_TEAM_BROADCAST_ID ? SLE_TEAM_SEND_GROUP : SLE_TEAM_SEND_UNICAST,
        dst_id, mesh_buf, (uint16_t)mesh_len);
}

static int sle_team_build_and_send(sle_team_node_t *node, uint8_t dst_id, uint8_t msg_type,
    const uint8_t *body, uint16_t body_len)
{
    uint8_t app_buf[SLE_TEAM_MAX_PAYLOAD_SIZE];
    uint16_t app_len = 0U;
    sle_team_app_packet_t app_packet;

    if (node == NULL) {
        return SLE_TEAM_ERR_ARG;
    }

    app_packet.app_msg_type = msg_type;
    app_packet.flags = 0U;
    app_packet.seq = node->next_seq++;
    app_packet.team_id = node->cfg.team_id;
    app_packet.src_id = node->cfg.self_id;
    app_packet.dst_id = dst_id;
    app_packet.ttl = 1U;
    app_packet.body_len = body_len;
    app_packet.body = body;

    if (sle_team_encode_app_packet(&app_packet, app_buf, sizeof(app_buf), &app_len) != SLE_TEAM_OK) {
        return SLE_TEAM_ERR_BUF;
    }
    return sle_team_send_app(node, dst_id, app_buf, app_len);
}

static void sle_team_mark_joined(sle_team_node_t *node, uint8_t member_id)
{
    if (node->ops.on_joined != NULL) {
        node->ops.on_joined(node->ops.user_ctx, member_id);
    }
}

static void sle_team_prune_stale_members(sle_team_node_t *node, uint32_t now_s)
{
    uint8_t i;
    uint16_t timeout_s;

    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_LEADER) {
        return;
    }

    timeout_s = node->cfg.heartbeat_timeout_s;
    if (timeout_s == 0U) {
        return;
    }

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        sle_team_member_record_t *member = &node->members[i];
        if (member->online == 0U) {
            continue;
        }
        if ((now_s - member->last_seen_s) > timeout_s) {
            member->online = 0U;
            sle_team_log(node, "member heartbeat timeout");
        }
    }
}

static int sle_team_handle_hello(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    const sle_team_hello_body_t *hello;
    sle_team_member_record_t *member;

    if (node == NULL || app == NULL || app->body_len < sizeof(*hello)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    hello = (const sle_team_hello_body_t *)app->body;
    member = sle_team_get_member_slot(node, app->src_id, 1U);
    if (member == NULL) {
        return SLE_TEAM_ERR_BUF;
    }

    member->role = hello->role;
    member->battery_percent = hello->battery_percent;
    member->last_seen_s = sle_team_now(node);
    member->last_seq = app->seq;

    if (node->cfg.role == SLE_TEAM_ROLE_LEADER) {
        (void)sle_team_node_send_ack(node, app->src_id, app->seq, SLE_TEAM_APP_HELLO, 0U);
        (void)sle_team_node_send_config(node, app->src_id);
        sle_team_mark_joined(node, app->src_id);
    }

    return SLE_TEAM_OK;
}

static int sle_team_handle_ack(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    const sle_team_ack_body_t *ack;

    if (node == NULL || app == NULL || app->body_len < sizeof(*ack)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    ack = (const sle_team_ack_body_t *)app->body;
    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER && ack->acked_msg_type == SLE_TEAM_APP_HELLO) {
        node->joined = 1U;
        node->state = SLE_TEAM_NET_ONLINE;
        sle_team_mark_joined(node, node->cfg.self_id);
    }
    return SLE_TEAM_OK;
}

static int sle_team_handle_config(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    const sle_team_config_body_t *cfg_body;

    if (node == NULL || app == NULL || app->body_len < sizeof(*cfg_body)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    cfg_body = (const sle_team_config_body_t *)app->body;
    node->cfg.report_interval_s = cfg_body->report_interval_s;
    node->cfg.warn_distance_m = cfg_body->warn_distance_m;
    node->cfg.lost_distance_m = cfg_body->lost_distance_m;
    node->cfg.heartbeat_timeout_s = cfg_body->heartbeat_timeout_s;
    node->last_config_s = sle_team_now(node);
    return SLE_TEAM_OK;
}

static int sle_team_handle_heartbeat(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    const sle_team_heartbeat_body_t *hb;
    sle_team_member_record_t *member;

    if (node == NULL || app == NULL || app->body_len < sizeof(*hb)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    hb = (const sle_team_heartbeat_body_t *)app->body;
    member = sle_team_get_member_slot(node, app->src_id, 1U);
    if (member == NULL) {
        return SLE_TEAM_ERR_BUF;
    }

    member->battery_percent = hb->battery_percent;
    member->fix_status = hb->fix_status;
    member->last_rssi_dbm = hb->rssi_dbm;
    member->last_seen_s = sle_team_now(node);
    member->last_seq = app->seq;
    return SLE_TEAM_OK;
}

static int sle_team_handle_position(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    const sle_team_pos_body_t *pos;
    sle_team_member_record_t *member;

    if (node == NULL || app == NULL || app->body_len < sizeof(*pos)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    pos = (const sle_team_pos_body_t *)app->body;
    member = sle_team_get_member_slot(node, app->src_id, 1U);
    if (member == NULL) {
        return SLE_TEAM_ERR_BUF;
    }

    member->battery_percent = pos->battery_percent;
    member->fix_status = pos->fix_status;
    member->last_seen_s = sle_team_now(node);
    member->last_seq = app->seq;

    if (node->ops.on_position != NULL) {
        node->ops.on_position(node->ops.user_ctx, app->src_id, pos);
    }
    return SLE_TEAM_OK;
}

static int sle_team_handle_alert(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    const sle_team_alert_body_t *alert;

    if (node == NULL || app == NULL || app->body_len < sizeof(*alert)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    alert = (const sle_team_alert_body_t *)app->body;
    if (node->ops.on_alert != NULL) {
        node->ops.on_alert(node->ops.user_ctx, alert->lost_member_id, alert->reason);
    }
    return SLE_TEAM_OK;
}

int sle_team_node_init(sle_team_node_t *node, const sle_team_node_cfg_t *cfg, const sle_team_node_ops_t *ops)
{
    if (node == NULL || cfg == NULL || ops == NULL || ops->send == NULL) {
        return SLE_TEAM_ERR_ARG;
    }

    (void)memset(node, 0, sizeof(*node));
    node->cfg = *cfg;
    node->ops = *ops;
    node->next_seq = 1U;
    node->state = (cfg->role == SLE_TEAM_ROLE_LEADER) ? SLE_TEAM_NET_ONLINE : SLE_TEAM_NET_DISCOVERING;
    node->joined = (cfg->role == SLE_TEAM_ROLE_LEADER) ? 1U : 0U;
    return SLE_TEAM_OK;
}

void sle_team_node_tick(sle_team_node_t *node)
{
    uint32_t now_s;
    sle_team_heartbeat_body_t hb;

    if (node == NULL) {
        return;
    }

    now_s = sle_team_now(node);
    sle_team_prune_stale_members(node, now_s);

    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER && node->joined == 0U) {
        if ((now_s - node->last_hello_s) >= 3U) {
            (void)sle_team_node_send_hello(node, node->cfg.leader_id);
            node->last_hello_s = now_s;
            node->state = SLE_TEAM_NET_JOINING;
        }
        return;
    }

    if ((now_s - node->last_heartbeat_s) >= node->cfg.heartbeat_interval_s) {
        hb.battery_percent = 100U;
        hb.rssi_dbm = -50;
        hb.fix_status = 1U;
        hb.reserved = 0U;
        (void)sle_team_node_send_heartbeat(node,
            node->cfg.role == SLE_TEAM_ROLE_LEADER ? SLE_TEAM_BROADCAST_ID : node->cfg.leader_id,
            hb.battery_percent, hb.rssi_dbm, hb.fix_status);
        node->last_heartbeat_s = now_s;
    }
}

int sle_team_node_on_packet(sle_team_node_t *node, const uint8_t *buf, size_t buf_len)
{
    sle_team_mesh_packet_t mesh_packet;
    sle_team_app_packet_t app_packet;
    const uint8_t *app_payload = NULL;
    uint16_t app_payload_len = 0U;
    uint8_t channel_hash = 0U;
    uint8_t cipher_mac[2];
    int ret;

    if (node == NULL || buf == NULL) {
        return SLE_TEAM_ERR_ARG;
    }

    ret = sle_team_decode_mesh_packet(&mesh_packet, buf, buf_len);
    if (ret != SLE_TEAM_OK) {
        return ret;
    }

    ret = sle_team_unwrap_mesh_group_data(&mesh_packet, &channel_hash, cipher_mac, &app_payload, &app_payload_len);
    if (ret != SLE_TEAM_OK) {
        return ret;
    }
    if (channel_hash != node->cfg.channel_hash) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }

    ret = sle_team_decode_app_packet(&app_packet, app_payload, app_payload_len);
    if (ret != SLE_TEAM_OK) {
        return ret;
    }
    if (app_packet.team_id != node->cfg.team_id) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    if (app_packet.dst_id != node->cfg.self_id && app_packet.dst_id != SLE_TEAM_BROADCAST_ID) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }

    switch (app_packet.app_msg_type) {
        case SLE_TEAM_APP_HELLO:
            return sle_team_handle_hello(node, &app_packet);
        case SLE_TEAM_APP_ACK:
            return sle_team_handle_ack(node, &app_packet);
        case SLE_TEAM_APP_CONFIG:
            return sle_team_handle_config(node, &app_packet);
        case SLE_TEAM_APP_HEARTBEAT:
            return sle_team_handle_heartbeat(node, &app_packet);
        case SLE_TEAM_APP_POS_REPORT:
            return sle_team_handle_position(node, &app_packet);
        case SLE_TEAM_APP_ALERT:
            return sle_team_handle_alert(node, &app_packet);
        default:
            sle_team_log(node, "unsupported app msg");
            return SLE_TEAM_ERR_UNSUPPORTED;
    }
}

int sle_team_node_send_hello(sle_team_node_t *node, uint8_t dst_id)
{
    sle_team_hello_body_t hello;

    if (node == NULL) {
        return SLE_TEAM_ERR_ARG;
    }

    hello.device_id = node->cfg.self_id;
    hello.role = (uint8_t)node->cfg.role;
    hello.battery_percent = 100U;
    hello.reserved = 0U;
    return sle_team_build_and_send(node, dst_id, SLE_TEAM_APP_HELLO, (const uint8_t *)&hello, sizeof(hello));
}

int sle_team_node_send_heartbeat(sle_team_node_t *node, uint8_t dst_id, uint8_t battery_percent,
    int8_t rssi_dbm, uint8_t fix_status)
{
    sle_team_heartbeat_body_t hb;

    if (node == NULL) {
        return SLE_TEAM_ERR_ARG;
    }

    hb.battery_percent = battery_percent;
    hb.rssi_dbm = rssi_dbm;
    hb.fix_status = fix_status;
    hb.reserved = 0U;
    return sle_team_build_and_send(node, dst_id, SLE_TEAM_APP_HEARTBEAT, (const uint8_t *)&hb, sizeof(hb));
}

int sle_team_node_send_position(sle_team_node_t *node, uint8_t dst_id, const sle_team_pos_body_t *pos)
{
    if (node == NULL || pos == NULL) {
        return SLE_TEAM_ERR_ARG;
    }
    return sle_team_build_and_send(node, dst_id, SLE_TEAM_APP_POS_REPORT, (const uint8_t *)pos, sizeof(*pos));
}

int sle_team_node_send_alert(sle_team_node_t *node, uint8_t dst_id, const sle_team_alert_body_t *alert)
{
    if (node == NULL || alert == NULL) {
        return SLE_TEAM_ERR_ARG;
    }
    return sle_team_build_and_send(node, dst_id, SLE_TEAM_APP_ALERT, (const uint8_t *)alert, sizeof(*alert));
}

int sle_team_node_send_config(sle_team_node_t *node, uint8_t dst_id)
{
    sle_team_config_body_t cfg_body;

    if (node == NULL) {
        return SLE_TEAM_ERR_ARG;
    }

    cfg_body.report_interval_s = node->cfg.report_interval_s;
    cfg_body.warn_distance_m = node->cfg.warn_distance_m;
    cfg_body.lost_distance_m = node->cfg.lost_distance_m;
    cfg_body.heartbeat_timeout_s = node->cfg.heartbeat_timeout_s;
    return sle_team_build_and_send(node, dst_id, SLE_TEAM_APP_CONFIG, (const uint8_t *)&cfg_body, sizeof(cfg_body));
}

int sle_team_node_send_ack(sle_team_node_t *node, uint8_t dst_id, uint16_t ack_seq, uint8_t acked_msg_type,
    uint8_t status_code)
{
    sle_team_ack_body_t ack;

    if (node == NULL) {
        return SLE_TEAM_ERR_ARG;
    }

    ack.ack_seq = ack_seq;
    ack.acked_msg_type = acked_msg_type;
    ack.status_code = status_code;
    return sle_team_build_and_send(node, dst_id, SLE_TEAM_APP_ACK, (const uint8_t *)&ack, sizeof(ack));
}

const sle_team_member_record_t *sle_team_node_find_member(const sle_team_node_t *node, uint8_t member_id)
{
    uint8_t i;

    if (node == NULL) {
        return NULL;
    }

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->members[i].online != 0U && node->members[i].member_id == member_id) {
            return &node->members[i];
        }
    }
    return NULL;
}
