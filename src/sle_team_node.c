#include "sle_team_node.h"

#include <string.h>

static const uint8_t g_zero_cipher_mac[2] = {0x00, 0x00};
#define SLE_TEAM_CONFIG_BODY_BASE_SIZE offsetof(sle_team_config_body_t, relay_allowed)
#define SLE_TEAM_MAX_RELAY_TIERS 3U

static void sle_team_node_disable_member_relay(sle_team_node_t *node, uint8_t clear_permission);

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

static int8_t sle_team_rssi_dbm(const sle_team_node_t *node)
{
    if (node == NULL || node->ops.rssi_dbm == NULL) {
        return SLE_TEAM_RSSI_UNKNOWN;
    }
    return node->ops.rssi_dbm(node->ops.user_ctx);
}

static uint8_t sle_team_should_defer_member_timeout(const sle_team_node_t *node, uint8_t member_id,
    uint32_t now_s, uint32_t last_seen_s)
{
    if (node == NULL || node->ops.should_defer_member_timeout == NULL) {
        return 0U;
    }
    return node->ops.should_defer_member_timeout(node->ops.user_ctx, member_id, now_s, last_seen_s);
}

static const sle_team_member_record_t *sle_team_find_member_record_const(const sle_team_node_t *node, uint8_t member_id)
{
    uint8_t i;

    if (node == NULL || member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return NULL;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->members[i].member_id == member_id) {
            return &node->members[i];
        }
    }
    return NULL;
}

static sle_team_member_record_t *sle_team_find_member_record(sle_team_node_t *node, uint8_t member_id)
{
    return (sle_team_member_record_t *)sle_team_find_member_record_const(node, member_id);
}

static sle_team_member_record_t *sle_team_get_member_slot(sle_team_node_t *node, uint8_t member_id, uint8_t create)
{
    uint8_t i;
    sle_team_member_record_t *member;
    sle_team_member_record_t *free_slot = NULL;

    if (node == NULL || member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return NULL;
    }
    member = sle_team_find_member_record(node, member_id);
    if (member != NULL) {
        if (create != 0U) {
            member->online = 1U;
        }
        return member;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (free_slot == NULL && node->members[i].member_id == 0U) {
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

static sle_team_pending_member_t *sle_team_get_pending_slot(sle_team_node_t *node, uint8_t member_id, uint8_t create)
{
    uint8_t i;
    sle_team_pending_member_t *free_slot = NULL;

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->pending_members[i].active != 0U && node->pending_members[i].member_id == member_id) {
            return &node->pending_members[i];
        }
        if (free_slot == NULL && node->pending_members[i].active == 0U) {
            free_slot = &node->pending_members[i];
        }
    }

    if (create != 0U && free_slot != NULL) {
        (void)memset(free_slot, 0, sizeof(*free_slot));
        free_slot->member_id = member_id;
        free_slot->active = 1U;
        return free_slot;
    }
    return NULL;
}

static void sle_team_clear_pending_member(sle_team_node_t *node, uint8_t member_id)
{
    uint8_t i;

    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->pending_members[i].active != 0U && node->pending_members[i].member_id == member_id) {
            (void)memset(&node->pending_members[i], 0, sizeof(node->pending_members[i]));
        }
    }
}

static void sle_team_clear_members(sle_team_node_t *node)
{
    if (node != NULL) {
        (void)memset(node->members, 0, sizeof(node->members));
    }
}

static void sle_team_clear_offline_member_records(sle_team_node_t *node)
{
    uint8_t i;

    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_LEADER) {
        return;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        sle_team_member_record_t *member = &node->members[i];

        if (member->member_id != 0U && member->online == 0U) {
            (void)memset(member, 0, sizeof(*member));
        }
    }
}

static uint8_t sle_team_id_in_list(const uint8_t *member_ids, uint8_t count, uint8_t member_id)
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

static sle_team_member_record_t *sle_team_alloc_offline_member_record(sle_team_node_t *node, uint8_t member_id)
{
    uint8_t i;
    sle_team_member_record_t *free_slot = NULL;

    if (node == NULL || member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return NULL;
    }
    if (sle_team_find_member_record(node, member_id) != NULL) {
        return sle_team_find_member_record(node, member_id);
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->members[i].member_id == 0U) {
            free_slot = &node->members[i];
            break;
        }
    }
    if (free_slot == NULL) {
        return NULL;
    }
    (void)memset(free_slot, 0, sizeof(*free_slot));
    free_slot->member_id = member_id;
    free_slot->role = SLE_TEAM_ROLE_MEMBER;
    free_slot->last_rssi_dbm = SLE_TEAM_RSSI_UNKNOWN;
    return free_slot;
}

static int sle_team_sync_allowed_member_records(sle_team_node_t *node, const uint8_t *member_ids, uint8_t count)
{
    uint8_t i;

    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_LEADER) {
        return SLE_TEAM_OK;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        sle_team_member_record_t *member = &node->members[i];

        if (member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        if (sle_team_id_in_list(member_ids, count, member->member_id) == 0U) {
            (void)memset(member, 0, sizeof(*member));
        }
    }
    for (i = 0U; i < count; i++) {
        if (sle_team_alloc_offline_member_record(node, member_ids[i]) == NULL) {
            return SLE_TEAM_ERR_BUF;
        }
    }
    return SLE_TEAM_OK;
}

static void sle_team_note_leader_seen(sle_team_node_t *node)
{
    if (node != NULL && node->cfg.role == SLE_TEAM_ROLE_MEMBER) {
        node->last_leader_seen_s = sle_team_now(node);
    }
}

static void sle_team_set_parent_state(sle_team_node_t *node, uint8_t parent_id, sle_team_parent_state_t state,
    uint8_t reselect_pending)
{
    uint32_t now_s;

    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_MEMBER) {
        return;
    }
    node->upstream_parent_id = parent_id;
    node->upstream_parent_state = state;
    node->upstream_parent_reselect_pending = reselect_pending;
    now_s = sle_team_now(node);
    if (parent_id == 0U || parent_id == SLE_TEAM_BROADCAST_ID) {
        node->last_parent_seen_s = 0U;
    } else if (state == SLE_TEAM_PARENT_CONNECTED && reselect_pending == 0U && now_s != 0U) {
        node->last_parent_seen_s = now_s;
    }
}

static uint8_t sle_team_member_has_reselect_target(const sle_team_node_t *node)
{
    return (uint8_t)(node != NULL &&
        node->cfg.role == SLE_TEAM_ROLE_MEMBER &&
        node->upstream_parent_state == SLE_TEAM_PARENT_RESELECTING &&
        node->upstream_parent_reselect_pending != 0U &&
        node->upstream_parent_id != 0U &&
        node->upstream_parent_id != SLE_TEAM_BROADCAST_ID &&
        node->upstream_parent_id != node->cfg.leader_id);
}

static int sle_team_member_recover_link(sle_team_node_t *node, const char *reason)
{
    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_MEMBER) {
        return SLE_TEAM_ERR_ARG;
    }
    if (node->cfg.leader_id == 0U || node->cfg.leader_id == SLE_TEAM_BROADCAST_ID) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    sle_team_set_parent_state(node, node->upstream_parent_id, SLE_TEAM_PARENT_RESELECTING, 1U);
    node->joined = 0U;
    node->state = SLE_TEAM_NET_DISCOVERING;
    node->last_hello_s = 0U;
    node->last_heartbeat_s = 0U;
    node->last_config_s = 0U;
    node->last_leader_seen_s = 0U;
    node->last_parent_seen_s = 0U;
    sle_team_node_disable_member_relay(node, 0U);
    sle_team_clear_members(node);
    sle_team_log(node, reason != NULL ? reason : "link lost, rejoining");
    return SLE_TEAM_OK;
}

static void sle_team_member_rejoin(sle_team_node_t *node)
{
    (void)sle_team_member_recover_link(node, "leader timeout, rejoining");
}

int sle_team_node_try_parent_switch(sle_team_node_t *node)
{
    uint8_t old_parent_id;
    int ret;

    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_MEMBER || node->joined == 0U) {
        return SLE_TEAM_ERR_ARG;
    }
    /*
     * ERR_UNSUPPORTED means no relay parent is currently set (none/broadcast/leader),
     * so parent-switch is not applicable for this state.
     */
    if (node->upstream_parent_id == 0U || node->upstream_parent_id == SLE_TEAM_BROADCAST_ID ||
        node->upstream_parent_id == node->cfg.leader_id) {
        return SLE_TEAM_ERR_UNSUPPORTED;
    }

    old_parent_id = node->upstream_parent_id;
    sle_team_set_parent_state(node, old_parent_id, SLE_TEAM_PARENT_RESELECTING, 1U);
    sle_team_log(node, "parent timeout, requesting new parent");
    ret = sle_team_node_send_hello(node, node->cfg.leader_id);
    if (ret != SLE_TEAM_OK) {
        return ret;
    }
    node->upstream_parent_id = 0U;
    node->last_parent_seen_s = 0U;
    return SLE_TEAM_OK;
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

static int sle_team_send_encoded_packet(sle_team_node_t *node, uint8_t dst_id, const sle_team_app_packet_t *app_packet)
{
    uint8_t app_buf[SLE_TEAM_MAX_PAYLOAD_SIZE];
    uint16_t app_len = 0U;

    if (node == NULL || app_packet == NULL) {
        return SLE_TEAM_ERR_ARG;
    }
    if (sle_team_encode_app_packet(app_packet, app_buf, sizeof(app_buf), &app_len) != SLE_TEAM_OK) {
        return SLE_TEAM_ERR_BUF;
    }
    return sle_team_send_app(node, dst_id, app_buf, app_len);
}

static int sle_team_build_and_send(sle_team_node_t *node, uint8_t dst_id, uint8_t msg_type,
    const uint8_t *body, uint16_t body_len)
{
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
    app_packet.ttl = node->cfg.default_ttl != 0U ? node->cfg.default_ttl : 1U;
    app_packet.body_len = body_len;
    app_packet.body = body;
    return sle_team_send_encoded_packet(node, dst_id, &app_packet);
}

static int sle_team_forward_packet(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    sle_team_app_packet_t forwarded;

    if (node == NULL || app == NULL) {
        return SLE_TEAM_ERR_ARG;
    }
    if (app->ttl <= 1U) {
        sle_team_log(node, "relay ttl expired");
        return SLE_TEAM_ERR_UNSUPPORTED;
    }

    forwarded = *app;
    forwarded.ttl = (uint8_t)(app->ttl - 1U);
    return sle_team_send_encoded_packet(node, forwarded.dst_id, &forwarded);
}

static uint8_t sle_team_discovery_only_allows(uint8_t app_msg_type)
{
    return (uint8_t)(app_msg_type == SLE_TEAM_APP_HELLO ||
        app_msg_type == SLE_TEAM_APP_ROUTE_UPDATE ||
        app_msg_type == SLE_TEAM_APP_CONFIG ||
        app_msg_type == SLE_TEAM_APP_ACK);
}

static uint8_t sle_team_relay_may_bridge_packet(const sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    if (node == NULL || app == NULL || node->cfg.role != SLE_TEAM_ROLE_MEMBER ||
        node->cfg.relay_allowed == 0U) {
        return 0U;
    }
    if (node->joined != 0U && node->cfg.relay_enabled != 0U) {
        if (node->cfg.relay_discovery_only != 0U &&
            sle_team_discovery_only_allows(app->app_msg_type) == 0U) {
            return 0U;
        }
        return 1U;
    }
    if (sle_team_discovery_only_allows(app->app_msg_type) == 0U) {
        return 0U;
    }
    if (node->state == SLE_TEAM_NET_IDLE) {
        return 0U;
    }
    return (uint8_t)(node->joined == 0U ||
        node->upstream_parent_state == SLE_TEAM_PARENT_RESELECTING ||
        node->upstream_parent_reselect_pending != 0U);
}

static uint8_t sle_team_should_relay_packet(const sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    if (node == NULL || app == NULL) {
        return 0U;
    }
    if (sle_team_relay_may_bridge_packet(node, app) == 0U) {
        return 0U;
    }
    if (app->src_id == node->cfg.self_id || app->src_id == SLE_TEAM_BROADCAST_ID) {
        return 0U;
    }
    if (app->dst_id == SLE_TEAM_BROADCAST_ID) {
        return app->src_id == node->cfg.leader_id ? 1U : 0U;
    }
    if (app->src_id == node->cfg.leader_id) {
        return 1U;
    }
    return app->dst_id == node->cfg.leader_id ? 1U : 0U;
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
            sle_team_alert_body_t alert;
            uint8_t member_id = member->member_id;

            if (sle_team_should_defer_member_timeout(node, member_id, now_s, member->last_seen_s) != 0U) {
                sle_team_log(node, "member timeout deferred after relay loss");
                continue;
            }
            (void)memset(&alert, 0, sizeof(alert));
            alert.lost_member_id = member_id;
            alert.reason = SLE_TEAM_ALERT_TIMEOUT;
            alert.last_latitude_e6 = member->latitude_e6;
            alert.last_longitude_e6 = member->longitude_e6;
            alert.last_report_s = member->last_seen_s;
            member->online = 0U;
            if (member->relay_allowed != 0U && node->ops.on_relay_offline != NULL) {
                node->ops.on_relay_offline(node->ops.user_ctx, member_id);
            }
            (void)sle_team_node_send_alert(node, SLE_TEAM_BROADCAST_ID, &alert);
            sle_team_log(node, "member heartbeat timeout");
        }
    }
}

static void sle_team_leader_mark_member_left(sle_team_node_t *node, uint8_t member_id, uint32_t last_report_s)
{
    sle_team_member_record_t *member;
    uint32_t now_s;
    uint8_t was_relay;

    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_LEADER ||
        member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return;
    }
    member = sle_team_find_member_record(node, member_id);
    if (member == NULL || member->online == 0U) {
        return;
    }
    member->online = 0U;
    was_relay = member->relay_allowed;
    member->relay_allowed = 0U;
    member->relay_tier = 0U;
    member->max_downstream = 0U;
    now_s = sle_team_now(node);
    if (last_report_s != 0U) {
        member->last_seen_s = last_report_s;
    } else if (member->last_seen_s == 0U && now_s != 0U) {
        member->last_seen_s = now_s;
    }
    if (was_relay != 0U && node->ops.on_relay_offline != NULL) {
        node->ops.on_relay_offline(node->ops.user_ctx, member_id);
    }
    sle_team_log(node, "member left leader");
}

static uint8_t sle_team_has_online_member(const sle_team_node_t *node)
{
    uint8_t i;

    if (node == NULL) {
        return 0U;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->members[i].online != 0U) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t sle_team_node_has_member_record(const sle_team_node_t *node, uint8_t member_id)
{
    return sle_team_find_member_record_const(node, member_id) != NULL ? 1U : 0U;
}

static uint8_t sle_team_node_relay_tier_for_member(uint8_t member_id, uint8_t leader_id)
{
    if (member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID || member_id == leader_id) {
        return 0U;
    }
    return (uint8_t)(((uint8_t)(member_id - 1U) % SLE_TEAM_MAX_RELAY_TIERS) + 1U);
}

static int sle_team_leader_refresh_relay_config(sle_team_node_t *node)
{
    uint8_t i;
    int last_err = SLE_TEAM_OK;

    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_LEADER) {
        return SLE_TEAM_ERR_ARG;
    }
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &node->members[i];
        int ret;

        if (member->online == 0U || member->relay_allowed == 0U ||
            member->member_id == 0U || member->member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        ret = sle_team_node_send_config(node, member->member_id);
        if (ret != SLE_TEAM_OK) {
            last_err = ret;
            sle_team_log(node, "relay config refresh failed");
        }
    }
    return last_err;
}

static void sle_team_node_disable_member_relay(sle_team_node_t *node, uint8_t clear_permission)
{
    if (node != NULL && node->cfg.role == SLE_TEAM_ROLE_MEMBER) {
        node->cfg.relay_enabled = 0U;
        if (clear_permission != 0U) {
            node->cfg.relay_allowed = 0U;
            node->cfg.relay_tier = 0U;
            node->cfg.max_downstream = 0U;
            node->cfg.relay_discovery_only = 0U;
        }
    }
}

static uint8_t sle_team_should_stage_pairing_hello(const sle_team_node_t *node, uint8_t member_id)
{
    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_LEADER || node->cfg.pairing_enabled == 0U ||
        node->cfg.member_filter_enabled == 0U || node->cfg.allowed_member_count != 0U) {
        return 0U;
    }
    return sle_team_find_member_record_const(node, member_id) == NULL ? 1U : 0U;
}

uint8_t sle_team_node_is_member_allowed(const sle_team_node_t *node, uint8_t member_id)
{
    uint8_t i;

    if (node == NULL) {
        return 0U;
    }
    if (node->cfg.role != SLE_TEAM_ROLE_LEADER || node->cfg.member_filter_enabled == 0U) {
        return 1U;
    }
    /*
     * Keep empty allowlist non-blocking. Pairing/startup flows can temporarily
     * set filter=only with count=0; treating this as deny-all would deadlock
     * member rejoin after power cycle.
     */
    if (node->cfg.allowed_member_count == 0U) {
        return 1U;
    }
    for (i = 0U; i < node->cfg.allowed_member_count && i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->cfg.allowed_member_ids[i] == member_id) {
            return 1U;
        }
    }
    return 0U;
}

int sle_team_node_allow_all_members(sle_team_node_t *node)
{
    if (node == NULL) {
        return SLE_TEAM_ERR_ARG;
    }
    node->cfg.member_filter_enabled = 0U;
    node->cfg.allowed_member_count = 0U;
    (void)memset(node->cfg.allowed_member_ids, 0, sizeof(node->cfg.allowed_member_ids));
    sle_team_clear_offline_member_records(node);
    return SLE_TEAM_OK;
}

int sle_team_node_set_allowed_members(sle_team_node_t *node, const uint8_t *member_ids, uint8_t count)
{
    uint8_t i;
    uint8_t j;
    uint8_t unique_ids[SLE_TEAM_MAX_MEMBERS];
    uint8_t unique_count = 0U;

    if (node == NULL || (count > 0U && member_ids == NULL) || count > SLE_TEAM_MAX_MEMBERS) {
        return SLE_TEAM_ERR_ARG;
    }
    for (i = 0U; i < count; i++) {
        if (member_ids[i] == 0U || member_ids[i] == SLE_TEAM_BROADCAST_ID) {
            return SLE_TEAM_ERR_ARG;
        }
        for (j = 0U; j < unique_count; j++) {
            if (unique_ids[j] == member_ids[i]) {
                break;
            }
        }
        if (j == unique_count) {
            unique_ids[unique_count++] = member_ids[i];
        }
    }
    if (sle_team_sync_allowed_member_records(node, unique_ids, unique_count) != SLE_TEAM_OK) {
        return SLE_TEAM_ERR_BUF;
    }
    node->cfg.member_filter_enabled = 1U;
    node->cfg.allowed_member_count = unique_count;
    (void)memset(node->cfg.allowed_member_ids, 0, sizeof(node->cfg.allowed_member_ids));
    (void)memcpy(node->cfg.allowed_member_ids, unique_ids, unique_count);
    return SLE_TEAM_OK;
}

int sle_team_node_add_allowed_member(sle_team_node_t *node, uint8_t member_id)
{
    uint8_t i;
    uint8_t was_filter_enabled;

    if (node == NULL || member_id == 0U || member_id == SLE_TEAM_BROADCAST_ID) {
        return SLE_TEAM_ERR_ARG;
    }
    was_filter_enabled = node->cfg.member_filter_enabled;
    if (node->cfg.member_filter_enabled == 0U) {
        if (sle_team_alloc_offline_member_record(node, member_id) == NULL) {
            return SLE_TEAM_ERR_BUF;
        }
        node->cfg.member_filter_enabled = 1U;
    }
    for (i = 0U; i < node->cfg.allowed_member_count; i++) {
        if (node->cfg.allowed_member_ids[i] == member_id) {
            return SLE_TEAM_OK;
        }
    }
    if (node->cfg.allowed_member_count >= SLE_TEAM_MAX_MEMBERS) {
        return SLE_TEAM_ERR_BUF;
    }
    if (sle_team_alloc_offline_member_record(node, member_id) == NULL) {
        if (was_filter_enabled == 0U) {
            node->cfg.member_filter_enabled = 0U;
        }
        return SLE_TEAM_ERR_BUF;
    }
    node->cfg.allowed_member_ids[node->cfg.allowed_member_count++] = member_id;
    return SLE_TEAM_OK;
}

int sle_team_node_remove_allowed_member(sle_team_node_t *node, uint8_t member_id)
{
    uint8_t i;
    uint8_t j;

    if (node == NULL) {
        return SLE_TEAM_ERR_ARG;
    }
    for (i = 0U; i < node->cfg.allowed_member_count; i++) {
        if (node->cfg.allowed_member_ids[i] != member_id) {
            continue;
        }
        for (j = i; j + 1U < node->cfg.allowed_member_count; j++) {
            node->cfg.allowed_member_ids[j] = node->cfg.allowed_member_ids[j + 1U];
        }
        node->cfg.allowed_member_count--;
        node->cfg.allowed_member_ids[node->cfg.allowed_member_count] = 0U;
        for (j = 0U; j < SLE_TEAM_MAX_MEMBERS; j++) {
            if (node->members[j].member_id == member_id) {
                (void)memset(&node->members[j], 0, sizeof(node->members[j]));
                break;
            }
        }
        return SLE_TEAM_OK;
    }
    return SLE_TEAM_OK;
}

int sle_team_node_pairing_start(sle_team_node_t *node)
{
    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_LEADER) {
        return SLE_TEAM_ERR_ARG;
    }
    node->cfg.pairing_enabled = 1U;
    node->cfg.member_filter_enabled = 1U;
    (void)memset(node->pending_members, 0, sizeof(node->pending_members));
    (void)sle_team_leader_refresh_relay_config(node);
    sle_team_log(node, "pairing started");
    return SLE_TEAM_OK;
}

int sle_team_node_pairing_stop(sle_team_node_t *node)
{
    uint8_t known_ids[SLE_TEAM_MAX_MEMBERS];
    uint8_t pending_ids[SLE_TEAM_MAX_MEMBERS];
    uint8_t known_count = 0U;
    uint8_t pending_count = 0U;
    uint8_t approve_failed = 0U;
    uint8_t i;
    int refresh_ret;
    int last_err = SLE_TEAM_OK;

    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_LEADER) {
        return SLE_TEAM_ERR_ARG;
    }

    /*
     * Preserve members that were already connected before the pairing window
     * opened. Starting pairing enables the leader allowlist filter, and new
     * members are staged as pending while the allowlist is empty. Existing
     * online members can keep sending traffic through that temporary allow-all
     * state; when the window closes they must be sealed into the allowlist too.
     */
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->members[i].member_id != 0U && node->members[i].member_id != SLE_TEAM_BROADCAST_ID) {
            known_ids[known_count++] = node->members[i].member_id;
        }
        if (node->pending_members[i].active == 0U || node->pending_members[i].member_id == 0U ||
            node->pending_members[i].member_id == SLE_TEAM_BROADCAST_ID) {
            continue;
        }
        pending_ids[pending_count++] = node->pending_members[i].member_id;
    }
    for (i = 0U; i < known_count; i++) {
        int ret = sle_team_node_add_allowed_member(node, known_ids[i]);
        if (ret != SLE_TEAM_OK) {
            approve_failed = 1U;
            last_err = ret;
        }
    }
    /*
     * Keep CLI/WebUI behavior consistent in V2: when pairing closes, pending
     * members are promoted to approved members by default (without relay grant).
     * WebUI can still pre-approve with explicit relay policy before stop.
     */
    for (i = 0U; i < pending_count; i++) {
        int ret = sle_team_node_pairing_approve_with_relay(node, pending_ids[i], 0U);
        if (ret != SLE_TEAM_OK) {
            approve_failed = 1U;
            last_err = ret;
        }
    }

    node->cfg.pairing_enabled = 0U;
    refresh_ret = sle_team_leader_refresh_relay_config(node);
    if (refresh_ret != SLE_TEAM_OK) {
        approve_failed = 1U;
        last_err = refresh_ret;
        sle_team_log(node, "pairing stop config retry pending");
    }
    if (approve_failed != 0U) {
        sle_team_log(node, "pairing stopped with pending retry");
    }
    sle_team_log(node, "pairing stopped");
    return approve_failed != 0U ? last_err : SLE_TEAM_OK;
}

int sle_team_node_pairing_approve(sle_team_node_t *node, uint8_t member_id)
{
    return sle_team_node_pairing_approve_with_relay(node, member_id, 0U);
}

int sle_team_node_pairing_approve_with_relay(sle_team_node_t *node, uint8_t member_id, uint8_t relay_allowed)
{
    sle_team_pending_member_t *pending;
    sle_team_member_record_t *member;
    uint8_t had_allowed_before = 0U;
    int cfg_ret;
    int ack_ret;
    int ret;
    uint8_t i;

    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_LEADER) {
        return SLE_TEAM_ERR_ARG;
    }
    pending = sle_team_get_pending_slot(node, member_id, 0U);
    for (i = 0U; i < node->cfg.allowed_member_count; i++) {
        if (node->cfg.allowed_member_ids[i] == member_id) {
            had_allowed_before = 1U;
            break;
        }
    }
    ret = sle_team_node_add_allowed_member(node, member_id);
    if (ret != SLE_TEAM_OK) {
        return ret;
    }
    if (pending != NULL) {
        member = sle_team_get_member_slot(node, member_id, 1U);
        if (member != NULL) {
            member->role = pending->role;
            member->battery_percent = pending->battery_percent;
            member->mac_ready = pending->mac_ready;
            (void)memcpy(member->mac, pending->mac, sizeof(member->mac));
            member->last_seen_s = pending->last_seen_s;
        }
    } else {
        member = sle_team_get_member_slot(node, member_id, 1U);
    }
    if (member == NULL) {
        if (had_allowed_before == 0U) {
            (void)sle_team_node_remove_allowed_member(node, member_id);
        }
        return SLE_TEAM_ERR_BUF;
    }
    member->relay_allowed = relay_allowed != 0U ? 1U : 0U;
    member->relay_tier = member->relay_allowed != 0U ?
        sle_team_node_relay_tier_for_member(member_id, node->cfg.leader_id) : 0U;
    member->max_downstream = member->relay_allowed != 0U ? SLE_TEAM_MAX_DIRECT_CONNECTIONS : 0U;
    cfg_ret = sle_team_node_send_config(node, member_id);
    ack_ret = sle_team_node_send_ack(node, member_id, 0U, SLE_TEAM_APP_HELLO, 0U);
    if (cfg_ret == SLE_TEAM_OK && ack_ret == SLE_TEAM_OK) {
        sle_team_clear_pending_member(node, member_id);
        sle_team_log(node, "member approved");
        return SLE_TEAM_OK;
    }
    if (cfg_ret != SLE_TEAM_OK) {
        sle_team_log(node, "member approve config send failed");
    }
    if (ack_ret != SLE_TEAM_OK) {
        sle_team_log(node, "member approve ack send failed");
        return ack_ret;
    }
    ret = cfg_ret;
    return ret;
}

int sle_team_node_member_select_leader(sle_team_node_t *node, uint8_t team_id, uint8_t leader_id,
    uint8_t channel_hash)
{
    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_MEMBER || team_id == 0U ||
        leader_id == 0U || leader_id == SLE_TEAM_BROADCAST_ID) {
        return SLE_TEAM_ERR_ARG;
    }
    node->cfg.team_id = team_id;
    node->cfg.leader_id = leader_id;
    node->cfg.channel_hash = channel_hash;
    node->joined = 0U;
    node->state = SLE_TEAM_NET_DISCOVERING;
    node->last_hello_s = 0U;
    node->last_heartbeat_s = 0U;
    node->last_config_s = 0U;
    node->last_leader_seen_s = 0U;
    node->last_parent_seen_s = 0U;
    sle_team_set_parent_state(node, 0U, SLE_TEAM_PARENT_DISCOVERING, 0U);
    sle_team_node_disable_member_relay(node, 1U);
    sle_team_clear_members(node);
    return sle_team_node_send_hello(node, node->cfg.leader_id);
}

int sle_team_node_member_leave(sle_team_node_t *node)
{
    sle_team_alert_body_t alert;
    int notify_ret = SLE_TEAM_OK;
    uint8_t leader_id;

    if (node == NULL || node->cfg.role != SLE_TEAM_ROLE_MEMBER) {
        return SLE_TEAM_ERR_ARG;
    }
    leader_id = node->cfg.leader_id;
    if (node->joined != 0U && leader_id != 0U && leader_id != SLE_TEAM_BROADCAST_ID) {
        (void)memset(&alert, 0, sizeof(alert));
        alert.lost_member_id = node->cfg.self_id;
        alert.reason = SLE_TEAM_ALERT_LEAVE;
        alert.last_report_s = sle_team_now(node);
        notify_ret = sle_team_node_send_alert(node, leader_id, &alert);
    }
    node->joined = 0U;
    node->state = SLE_TEAM_NET_IDLE;
    node->cfg.leader_id = 0U;
    node->last_hello_s = 0U;
    node->last_heartbeat_s = 0U;
    node->last_config_s = 0U;
    node->last_leader_seen_s = 0U;
    node->last_parent_seen_s = 0U;
    sle_team_set_parent_state(node, 0U, SLE_TEAM_PARENT_IDLE, 0U);
    sle_team_node_disable_member_relay(node, 1U);
    sle_team_clear_members(node);
    if (notify_ret != SLE_TEAM_OK) {
        sle_team_log(node, "member leave notify failed");
    }
    sle_team_log(node, "member left team");
    return SLE_TEAM_OK;
}

int sle_team_node_member_link_lost(sle_team_node_t *node)
{
    return sle_team_member_recover_link(node, "link lost, rejoining");
}

static int sle_team_handle_hello(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    sle_team_hello_body_t hello;
    sle_team_member_record_t *member;
    sle_team_member_record_t member_before;
    sle_team_pending_member_t *pending;
    int ack_ret;
    int cfg_ret;
    uint8_t had_member;
    uint8_t refreshed_online = 0U;
    uint32_t refreshed_last_seen_s = 0U;
    uint16_t refreshed_last_seq = 0U;

    if (node == NULL || app == NULL || app->body_len < sizeof(hello)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    (void)memcpy(&hello, app->body, sizeof(hello));
    if (sle_team_should_stage_pairing_hello(node, app->src_id) != 0U) {
        pending = sle_team_get_pending_slot(node, app->src_id, 1U);
        if (pending != NULL) {
            pending->role = hello.role;
            pending->battery_percent = hello.battery_percent;
            pending->mac_ready = hello.mac_ready;
            (void)memcpy(pending->mac, hello.mac, sizeof(pending->mac));
            pending->last_seen_s = sle_team_now(node);
            sle_team_log(node, "member pending approval");
            return SLE_TEAM_OK;
        }
        return SLE_TEAM_ERR_BUF;
    }
    if (node->cfg.role == SLE_TEAM_ROLE_LEADER && sle_team_node_is_member_allowed(node, app->src_id) == 0U) {
        if (sle_team_node_has_member_record(node, app->src_id) != 0U) {
            sle_team_log(node, "known member hello before allowlist sync");
            return SLE_TEAM_OK;
        }
        if (node->cfg.pairing_enabled != 0U) {
            pending = sle_team_get_pending_slot(node, app->src_id, 1U);
            if (pending != NULL) {
                pending->role = hello.role;
                pending->battery_percent = hello.battery_percent;
                pending->mac_ready = hello.mac_ready;
                (void)memcpy(pending->mac, hello.mac, sizeof(pending->mac));
                pending->last_seen_s = sle_team_now(node);
                sle_team_log(node, "member pending approval");
                return SLE_TEAM_OK;
            }
        }
        sle_team_log(node, "member rejected by allowlist");
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    member = sle_team_get_member_slot(node, app->src_id, 0U);
    had_member = member != NULL ? 1U : 0U;
    if (had_member != 0U) {
        member_before = *member;
    }
    member = sle_team_get_member_slot(node, app->src_id, 1U);
    if (member == NULL) {
        return SLE_TEAM_ERR_BUF;
    }

    member->role = hello.role;
    member->battery_percent = hello.battery_percent;
    member->mac_ready = hello.mac_ready;
    (void)memcpy(member->mac, hello.mac, sizeof(member->mac));
    member->last_seen_s = sle_team_now(node);
    member->last_seq = app->seq;
    refreshed_online = member->online;
    refreshed_last_seen_s = member->last_seen_s;
    refreshed_last_seq = member->last_seq;

    if (node->cfg.role == SLE_TEAM_ROLE_LEADER) {
        cfg_ret = sle_team_node_send_config(node, app->src_id);
        if (cfg_ret != SLE_TEAM_OK) {
            if (had_member != 0U) {
                *member = member_before;
                member->online = refreshed_online;
                member->last_seen_s = refreshed_last_seen_s;
                member->last_seq = refreshed_last_seq;
                member->role = hello.role;
                member->battery_percent = hello.battery_percent;
                member->mac_ready = hello.mac_ready;
                (void)memcpy(member->mac, hello.mac, sizeof(member->mac));
            } else {
                (void)memset(member, 0, sizeof(*member));
            }
            sle_team_log(node, "config send failed on hello; liveness preserved");
            return cfg_ret;
        }
        ack_ret = sle_team_node_send_ack(node, app->src_id, app->seq, SLE_TEAM_APP_HELLO, 0U);
        if (ack_ret == SLE_TEAM_OK) {
            sle_team_clear_pending_member(node, app->src_id);
            sle_team_mark_joined(node, app->src_id);
        } else {
            if (had_member != 0U) {
                *member = member_before;
                member->online = refreshed_online;
                member->last_seen_s = refreshed_last_seen_s;
                member->last_seq = refreshed_last_seq;
                member->role = hello.role;
                member->battery_percent = hello.battery_percent;
                member->mac_ready = hello.mac_ready;
                (void)memcpy(member->mac, hello.mac, sizeof(member->mac));
            } else {
                (void)memset(member, 0, sizeof(*member));
            }
            sle_team_log(node, "hello ack send failed; liveness preserved");
            return ack_ret;
        }
    }

    return SLE_TEAM_OK;
}

static int sle_team_handle_ack(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    sle_team_ack_body_t ack;

    if (node == NULL || app == NULL || app->body_len < sizeof(ack)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    (void)memcpy(&ack, app->body, sizeof(ack));
    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER && ack.acked_msg_type == SLE_TEAM_APP_HELLO) {
        if (ack.status_code != 0U) {
            sle_team_log(node, "hello ack rejected");
            return SLE_TEAM_ERR_UNSUPPORTED;
        }
        if (app->src_id == node->cfg.leader_id && sle_team_member_has_reselect_target(node) != 0U) {
            sle_team_note_leader_seen(node);
            sle_team_log(node, "hello ack deferred until reselect parent");
            return SLE_TEAM_OK;
        }
        if (node->upstream_parent_id == 0U && app->src_id != 0U && app->src_id != SLE_TEAM_BROADCAST_ID) {
            node->upstream_parent_id = app->src_id;
        }
        node->joined = 1U;
        node->state = SLE_TEAM_NET_ONLINE;
        /* CONFIG may arrive before ACK; ACK marks final join and enables relay when permission is already cached. */
        node->cfg.relay_enabled = node->cfg.relay_allowed != 0U ? 1U : 0U;
        sle_team_set_parent_state(node,
            node->upstream_parent_id != 0U ? node->upstream_parent_id : app->src_id,
            SLE_TEAM_PARENT_CONNECTED, 0U);
        sle_team_note_leader_seen(node);
        node->last_parent_seen_s = sle_team_now(node);
        sle_team_mark_joined(node, node->cfg.self_id);
    }
    return SLE_TEAM_OK;
}

static int sle_team_handle_config(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    sle_team_config_body_t cfg_body;
    uint8_t defer_parent_bind;
    uint32_t now_s;

    if (node == NULL || app == NULL || app->body_len < SLE_TEAM_CONFIG_BODY_BASE_SIZE) {
        return SLE_TEAM_ERR_FORMAT;
    }
    if (node->cfg.role != SLE_TEAM_ROLE_MEMBER || app->src_id != node->cfg.leader_id) {
        sle_team_log(node, "config rejected by role/source");
        return SLE_TEAM_ERR_UNSUPPORTED;
    }

    (void)memset(&cfg_body, 0, sizeof(cfg_body));
    (void)memcpy(&cfg_body, app->body, app->body_len < sizeof(cfg_body) ? app->body_len : sizeof(cfg_body));
    defer_parent_bind = (uint8_t)(app->src_id == node->cfg.leader_id &&
        sle_team_member_has_reselect_target(node) != 0U);
    node->cfg.report_interval_s = cfg_body.report_interval_s;
    node->cfg.warn_distance_m = cfg_body.warn_distance_m;
    node->cfg.lost_distance_m = cfg_body.lost_distance_m;
    node->cfg.heartbeat_timeout_s = cfg_body.heartbeat_timeout_s;
    if (app->body_len >= sizeof(cfg_body)) {
        node->cfg.relay_allowed = cfg_body.relay_allowed != 0U ? 1U : 0U;
        node->cfg.relay_tier = node->cfg.relay_allowed != 0U ? cfg_body.relay_tier : 0U;
        node->cfg.max_downstream = node->cfg.relay_allowed != 0U ? cfg_body.max_downstream : 0U;
        node->cfg.relay_discovery_only = (cfg_body.reserved & SLE_TEAM_CONFIG_FLAG_RELAY_DISCOVERY_ONLY) != 0U ?
            1U : 0U;
        node->cfg.relay_enabled = (node->joined != 0U && node->cfg.relay_allowed != 0U) ? 1U : 0U;
    } else {
        sle_team_node_disable_member_relay(node, 1U);
    }
    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER) {
        if (node->upstream_parent_id == 0U && app->src_id != 0U && app->src_id != SLE_TEAM_BROADCAST_ID) {
            node->upstream_parent_id = app->src_id;
        }
        if (defer_parent_bind == 0U) {
            sle_team_set_parent_state(node,
                node->upstream_parent_id != 0U ? node->upstream_parent_id : app->src_id,
                SLE_TEAM_PARENT_CONNECTED, 0U);
        } else {
            sle_team_log(node, "config deferred until reselect parent");
        }
    }
    now_s = sle_team_now(node);
    node->last_config_s = now_s;
    sle_team_note_leader_seen(node);
    if (defer_parent_bind == 0U) {
        node->last_parent_seen_s = now_s;
    }
    return SLE_TEAM_OK;
}

static int sle_team_handle_heartbeat(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    sle_team_heartbeat_body_t hb;
    sle_team_member_record_t *member;

    if (node == NULL || app == NULL || app->body_len < sizeof(hb)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    (void)memcpy(&hb, app->body, sizeof(hb));
    if (node->cfg.role == SLE_TEAM_ROLE_LEADER && sle_team_node_is_member_allowed(node, app->src_id) == 0U) {
        sle_team_log(node, "heartbeat rejected by allowlist");
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER) {
        sle_team_note_leader_seen(node);
        return SLE_TEAM_OK;
    }
    member = sle_team_get_member_slot(node, app->src_id, 1U);
    if (member == NULL) {
        return SLE_TEAM_ERR_BUF;
    }

    member->battery_percent = hb.battery_percent;
    member->fix_status = hb.fix_status;
    member->last_rssi_dbm = hb.rssi_dbm;
    member->last_seen_s = sle_team_now(node);
    member->last_seq = app->seq;
    sle_team_note_leader_seen(node);
    return SLE_TEAM_OK;
}

static int sle_team_handle_position(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    sle_team_pos_body_t pos;
    sle_team_member_record_t *member;

    if (node == NULL || app == NULL || app->body_len < sizeof(pos)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    (void)memcpy(&pos, app->body, sizeof(pos));
    if (node->cfg.role == SLE_TEAM_ROLE_LEADER && sle_team_node_is_member_allowed(node, app->src_id) == 0U) {
        sle_team_log(node, "position rejected by allowlist");
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    member = sle_team_get_member_slot(node, app->src_id, 1U);
    if (member == NULL) {
        return SLE_TEAM_ERR_BUF;
    }

    member->battery_percent = pos.battery_percent;
    member->fix_status = pos.fix_status;
    member->latitude_e6 = pos.latitude_e6;
    member->longitude_e6 = pos.longitude_e6;
    member->speed_cms = pos.speed_cms;
    member->heading_deg = pos.heading_deg;
    member->sat_count = pos.sat_count;
    member->last_seen_s = sle_team_now(node);
    member->last_seq = app->seq;
    sle_team_note_leader_seen(node);

    if (node->ops.on_position != NULL) {
        node->ops.on_position(node->ops.user_ctx, app->src_id, &pos);
    }
    return SLE_TEAM_OK;
}

static int sle_team_handle_alert(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    sle_team_alert_body_t alert;

    if (node == NULL || app == NULL || app->body_len < sizeof(alert)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    (void)memcpy(&alert, app->body, sizeof(alert));
    if (node->ops.on_alert != NULL) {
        node->ops.on_alert(node->ops.user_ctx, alert.lost_member_id, alert.reason);
    }
    if (node->cfg.role == SLE_TEAM_ROLE_LEADER && alert.reason == SLE_TEAM_ALERT_LEAVE &&
        alert.lost_member_id == app->src_id) {
        sle_team_leader_mark_member_left(node, alert.lost_member_id, alert.last_report_s);
    }
    return SLE_TEAM_OK;
}

static int sle_team_handle_route_update(sle_team_node_t *node, const sle_team_app_packet_t *app)
{
    sle_team_route_update_body_t route_update;

    if (node == NULL || app == NULL || app->body_len < sizeof(route_update)) {
        return SLE_TEAM_ERR_FORMAT;
    }

    (void)memcpy(&route_update, app->body, sizeof(route_update));
    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER && app->src_id == node->cfg.leader_id) {
        sle_team_note_leader_seen(node);
        /*
         * Route updates are leader policy hints. The real upstream parent is
         * the physical first-hop connection, which the WS63 adapter records
         * from connection tracking and packet ingress. Blindly copying the
         * hinted next_hop here makes relayed leaves flap back to "leader".
         */
        if (route_update.parent_state == (uint8_t)SLE_TEAM_PARENT_RESELECTING &&
            route_update.parent_id != 0U && route_update.parent_id != SLE_TEAM_BROADCAST_ID &&
            route_update.parent_id != node->cfg.leader_id) {
            sle_team_set_parent_state(node, route_update.parent_id, SLE_TEAM_PARENT_RESELECTING, 1U);
            node->joined = 0U;
            node->state = SLE_TEAM_NET_DISCOVERING;
            node->last_hello_s = 0U;
            node->last_heartbeat_s = 0U;
            node->last_config_s = 0U;
            node->last_parent_seen_s = 0U;
            sle_team_log(node, "route update requests parent reselect");
        } else if (node->upstream_parent_id == app->src_id ||
            (node->upstream_parent_id == 0U && app->src_id != 0U && app->src_id != SLE_TEAM_BROADCAST_ID)) {
            sle_team_set_parent_state(node, app->src_id, SLE_TEAM_PARENT_CONNECTED, 0U);
            node->last_parent_seen_s = sle_team_now(node);
        }
        if ((route_update.reserved & SLE_TEAM_ROUTE_UPDATE_FLAG_RELAY_GRANT) != 0U) {
            node->cfg.relay_enabled = node->cfg.relay_allowed != 0U ? 1U : 0U;
        }
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
    if (node->cfg.default_ttl == 0U) {
        node->cfg.default_ttl = 1U;
    }
    node->ops = *ops;
    node->next_seq = 1U;
    node->state = (cfg->role == SLE_TEAM_ROLE_LEADER) ? SLE_TEAM_NET_ONLINE : SLE_TEAM_NET_DISCOVERING;
    node->joined = (cfg->role == SLE_TEAM_ROLE_LEADER) ? 1U : 0U;
    node->upstream_parent_id = 0U;
    node->upstream_parent_reselect_pending = 0U;
    node->upstream_parent_state = (cfg->role == SLE_TEAM_ROLE_LEADER) ? SLE_TEAM_PARENT_IDLE :
        SLE_TEAM_PARENT_DISCOVERING;
    node->last_parent_seen_s = 0U;
    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER) {
        node->cfg.relay_allowed = 0U;
        node->cfg.relay_enabled = 0U;
        node->cfg.relay_tier = 0U;
        node->cfg.max_downstream = 0U;
        node->cfg.relay_discovery_only = 0U;
    }
    node->last_leader_seen_s = (cfg->role == SLE_TEAM_ROLE_LEADER) ? 0U : sle_team_now(node);
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

    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER && node->joined != 0U && node->cfg.heartbeat_timeout_s != 0U &&
        node->last_leader_seen_s != 0U && (now_s - node->last_leader_seen_s) > node->cfg.heartbeat_timeout_s) {
        sle_team_member_rejoin(node);
    }

    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER && node->joined != 0U && node->cfg.parent_timeout_s != 0U &&
        node->upstream_parent_id != 0U && node->upstream_parent_id != SLE_TEAM_BROADCAST_ID &&
        node->upstream_parent_id != node->cfg.leader_id &&
        node->last_parent_seen_s != 0U && (now_s - node->last_parent_seen_s) > node->cfg.parent_timeout_s) {
        (void)sle_team_node_try_parent_switch(node);
    }

    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER && node->joined == 0U &&
        node->cfg.leader_id != 0U && node->cfg.leader_id != SLE_TEAM_BROADCAST_ID &&
        node->state != SLE_TEAM_NET_IDLE) {
        if ((now_s - node->last_hello_s) >= 3U) {
            (void)sle_team_node_send_hello(node, node->cfg.leader_id);
            node->last_hello_s = now_s;
            node->state = SLE_TEAM_NET_JOINING;
        }
        return;
    }

    if (node->cfg.role == SLE_TEAM_ROLE_LEADER && sle_team_has_online_member(node) == 0U) {
        return;
    }

    if (node->cfg.heartbeat_interval_s != 0U &&
        (now_s - node->last_heartbeat_s) >= node->cfg.heartbeat_interval_s) {
        hb.battery_percent = 100U;
        hb.rssi_dbm = sle_team_rssi_dbm(node);
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
        sle_team_log(node, "packet rejected by team id");
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    if (app_packet.dst_id != node->cfg.self_id) {
        if (sle_team_should_relay_packet(node, &app_packet) != 0U) {
            ret = sle_team_forward_packet(node, &app_packet);
            if (ret == SLE_TEAM_OK) {
                sle_team_log(node, "relay forwarded packet");
                if (app_packet.dst_id != SLE_TEAM_BROADCAST_ID) {
                    return SLE_TEAM_OK;
                }
            } else {
                sle_team_log(node, "relay forward failed");
                if (app_packet.dst_id != SLE_TEAM_BROADCAST_ID) {
                    return ret;
                }
            }
        }
        if (app_packet.dst_id != SLE_TEAM_BROADCAST_ID) {
            sle_team_log(node, "relay rejected unicast packet");
            return SLE_TEAM_ERR_UNSUPPORTED;
        }
    }
    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER && app_packet.src_id != node->cfg.leader_id) {
        sle_team_log(node, "packet rejected by leader id");
        return SLE_TEAM_ERR_UNSUPPORTED;
    }
    if (node->cfg.role == SLE_TEAM_ROLE_MEMBER && node->cfg.relay_discovery_only != 0U &&
        app_packet.dst_id == SLE_TEAM_BROADCAST_ID &&
        sle_team_discovery_only_allows(app_packet.app_msg_type) == 0U) {
        sle_team_log(node, "relay discovery-only ignored local broadcast");
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
        case SLE_TEAM_APP_ROUTE_UPDATE:
            return sle_team_handle_route_update(node, &app_packet);
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
    hello.mac_ready = node->cfg.self_mac_ready;
    (void)memcpy(hello.mac, node->cfg.self_mac, sizeof(hello.mac));
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
    const sle_team_member_record_t *member;

    if (node == NULL) {
        return SLE_TEAM_ERR_ARG;
    }

    cfg_body.report_interval_s = node->cfg.report_interval_s;
    cfg_body.warn_distance_m = node->cfg.warn_distance_m;
    cfg_body.lost_distance_m = node->cfg.lost_distance_m;
    cfg_body.heartbeat_timeout_s = node->cfg.heartbeat_timeout_s;
    cfg_body.relay_allowed = 0U;
    cfg_body.relay_tier = 0U;
    cfg_body.max_downstream = 0U;
    cfg_body.reserved = 0U;
    if (node->cfg.role == SLE_TEAM_ROLE_LEADER) {
        member = sle_team_node_find_member(node, dst_id);
        if (member != NULL && member->relay_allowed != 0U) {
            cfg_body.relay_allowed = 1U;
            cfg_body.relay_tier = member->relay_tier;
            cfg_body.max_downstream = member->max_downstream;
            if (node->cfg.pairing_enabled != 0U) {
                cfg_body.reserved |= SLE_TEAM_CONFIG_FLAG_RELAY_DISCOVERY_ONLY;
            }
        }
    } else {
        cfg_body.relay_allowed = node->cfg.relay_allowed;
        cfg_body.relay_tier = node->cfg.relay_tier;
        cfg_body.max_downstream = node->cfg.max_downstream;
        if (node->cfg.relay_discovery_only != 0U) {
            cfg_body.reserved |= SLE_TEAM_CONFIG_FLAG_RELAY_DISCOVERY_ONLY;
        }
    }
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

int sle_team_node_send_route_update(sle_team_node_t *node, uint8_t dst_id, uint8_t parent_id,
    uint8_t parent_state, uint8_t next_hop_id)
{
    sle_team_route_update_body_t route_update;

    if (node == NULL) {
        return SLE_TEAM_ERR_ARG;
    }

    route_update.parent_id = parent_id;
    route_update.next_hop_id = next_hop_id != 0U ? next_hop_id : parent_id;
    route_update.parent_state = parent_state;
    route_update.reserved = 0U;
    if (node->cfg.role == SLE_TEAM_ROLE_LEADER && parent_state != 0U) {
        route_update.reserved |= SLE_TEAM_ROUTE_UPDATE_FLAG_RELAY_GRANT;
    }
    return sle_team_build_and_send(node, dst_id, SLE_TEAM_APP_ROUTE_UPDATE, (const uint8_t *)&route_update,
        sizeof(route_update));
}

const sle_team_member_record_t *sle_team_node_find_member(const sle_team_node_t *node, uint8_t member_id)
{
    const sle_team_member_record_t *member;

    if (node == NULL) {
        return NULL;
    }
    member = sle_team_find_member_record_const(node, member_id);
    if (member != NULL && member->online != 0U) {
        return member;
    }
    return NULL;
}
