#include "sle_team_node.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t now_s;
    uint8_t last_tx[256];
    uint16_t last_tx_len;
    const char *name;
} demo_runtime_t;

static int demo_send(void *user_ctx, sle_team_send_kind_t kind, uint8_t dst_id, const uint8_t *buf, uint16_t len)
{
    demo_runtime_t *rt = (demo_runtime_t *)user_ctx;

    (void)kind;
    (void)dst_id;
    if (rt == NULL || buf == NULL || len > sizeof(rt->last_tx)) {
        return -1;
    }

    (void)memcpy(rt->last_tx, buf, len);
    rt->last_tx_len = len;
    printf("[%s] tx %u bytes\n", rt->name, len);
    return 0;
}

static uint32_t demo_now(void *user_ctx)
{
    demo_runtime_t *rt = (demo_runtime_t *)user_ctx;
    return rt == NULL ? 0U : rt->now_s;
}

static void demo_log(void *user_ctx, const char *text)
{
    demo_runtime_t *rt = (demo_runtime_t *)user_ctx;
    printf("[%s] %s\n", rt == NULL ? "?" : rt->name, text);
}

static void demo_joined(void *user_ctx, uint8_t member_id)
{
    demo_runtime_t *rt = (demo_runtime_t *)user_ctx;
    printf("[%s] joined event member=%u\n", rt == NULL ? "?" : rt->name, member_id);
}

static void demo_position(void *user_ctx, uint8_t member_id, const sle_team_pos_body_t *pos)
{
    demo_runtime_t *rt = (demo_runtime_t *)user_ctx;
    printf("[%s] pos from=%u lat=%ld lon=%ld battery=%u\n",
        rt == NULL ? "?" : rt->name,
        member_id,
        (long)pos->latitude_e6,
        (long)pos->longitude_e6,
        pos->battery_percent);
}

static void relay_last_packet(demo_runtime_t *from, sle_team_node_t *to)
{
    if (from->last_tx_len > 0U) {
        (void)sle_team_node_on_packet(to, from->last_tx, from->last_tx_len);
        from->last_tx_len = 0U;
    }
}

static uint8_t demo_decode_last_app_packet(const demo_runtime_t *rt, sle_team_app_packet_t *app)
{
    sle_team_mesh_packet_t mesh;
    const uint8_t *app_payload = NULL;
    uint16_t app_payload_len = 0U;
    uint8_t channel_hash = 0U;
    uint8_t cipher_mac[2];

    if (rt == NULL || app == NULL || rt->last_tx_len == 0U) {
        return 0U;
    }
    if (sle_team_decode_mesh_packet(&mesh, rt->last_tx, rt->last_tx_len) != SLE_TEAM_OK) {
        return 0U;
    }
    if (sle_team_unwrap_mesh_group_data(&mesh, &channel_hash, cipher_mac, &app_payload, &app_payload_len) !=
        SLE_TEAM_OK) {
        return 0U;
    }
    return sle_team_decode_app_packet(app, app_payload, app_payload_len) == SLE_TEAM_OK ? 1U : 0U;
}

#ifdef SLE_TEAM_NETWORK_TEST
int main(void)
{
    demo_runtime_t leader_rt = {.name = "leader"};
    demo_runtime_t relay_rt = {.name = "relay"};
    demo_runtime_t member_rt = {.name = "member"};
    sle_team_node_t leader;
    sle_team_node_t relay;
    sle_team_node_t member;
    sle_team_node_cfg_t leader_cfg;
    sle_team_node_cfg_t relay_cfg;
    sle_team_node_cfg_t member_cfg;
    sle_team_node_ops_t leader_ops;
    sle_team_node_ops_t relay_ops;
    sle_team_node_ops_t member_ops;
    sle_team_pos_body_t pos;

    (void)memset(&leader_cfg, 0, sizeof(leader_cfg));
    leader_cfg.team_id = 1U;
    leader_cfg.self_id = 1U;
    leader_cfg.leader_id = 1U;
    leader_cfg.role = SLE_TEAM_ROLE_LEADER;
    leader_cfg.channel_hash = 0x11U;
    leader_cfg.report_interval_s = 5U;
    leader_cfg.heartbeat_interval_s = 3U;
    leader_cfg.warn_distance_m = 50U;
    leader_cfg.lost_distance_m = 80U;
    leader_cfg.heartbeat_timeout_s = 10U;
    leader_cfg.default_ttl = 1U;

    member_cfg = leader_cfg;
    member_cfg.self_id = 2U;
    member_cfg.role = SLE_TEAM_ROLE_MEMBER;

    relay_cfg = leader_cfg;
    relay_cfg.self_id = 3U;
    relay_cfg.role = SLE_TEAM_ROLE_MEMBER;
    relay_cfg.relay_enabled = 1U;
    relay_cfg.default_ttl = 2U;

    (void)memset(&leader_ops, 0, sizeof(leader_ops));
    leader_ops.send = demo_send;
    leader_ops.now_s = demo_now;
    leader_ops.log = demo_log;
    leader_ops.on_joined = demo_joined;
    leader_ops.on_position = demo_position;
    leader_ops.user_ctx = &leader_rt;

    relay_ops = leader_ops;
    relay_ops.user_ctx = &relay_rt;

    member_ops = leader_ops;
    member_ops.user_ctx = &member_rt;

    (void)sle_team_node_init(&leader, &leader_cfg, &leader_ops);
    (void)sle_team_node_init(&relay, &relay_cfg, &relay_ops);
    (void)sle_team_node_init(&member, &member_cfg, &member_ops);
    assert(relay.cfg.relay_enabled == 0U);

    assert(sle_team_node_pairing_approve_with_relay(&leader, 3U, 1U) == SLE_TEAM_OK);
    assert(sle_team_node_send_config(&leader, 3U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &relay);
    assert(relay.cfg.relay_allowed != 0U);
    assert(relay.cfg.relay_enabled == 0U);
    assert(sle_team_node_send_ack(&leader, 3U, 0U, SLE_TEAM_APP_HELLO, 0U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &relay);
    assert(relay.joined != 0U);
    assert(relay.cfg.relay_enabled != 0U);

    assert(SLE_TEAM_MAX_MEMBERS >= 20U);
    for (uint8_t member_id = 2U; member_id <= 21U; member_id++) {
        assert(sle_team_node_add_allowed_member(&leader, member_id) == SLE_TEAM_OK);
    }
    assert(leader.cfg.allowed_member_count == 20U);
    assert(sle_team_node_is_member_allowed(&leader, 21U) != 0U);
    assert(sle_team_node_pairing_approve_with_relay(&leader, 21U, 0U) == SLE_TEAM_OK);
    assert(sle_team_node_find_member(&leader, 21U) != NULL);
    (void)sle_team_node_allow_all_members(&leader);

    {
        uint8_t only_member_3 = 3U;

        assert(sle_team_node_set_allowed_members(&leader, &only_member_3, 1U) == SLE_TEAM_OK);
        member_rt.now_s = 3U;
        sle_team_node_tick(&member);
        relay_last_packet(&member_rt, &leader);
        assert(sle_team_node_find_member(&leader, 2U) == NULL);
        assert(member.joined == 0U);

        assert(sle_team_node_add_allowed_member(&leader, 2U) == SLE_TEAM_OK);
        assert(sle_team_node_send_hello(&member, 1U) == SLE_TEAM_OK);
        relay_last_packet(&member_rt, &leader);
        relay_last_packet(&leader_rt, &member);
        relay_last_packet(&leader_rt, &member);
        assert(sle_team_node_find_member(&leader, 2U) != NULL);
    }

    (void)sle_team_node_allow_all_members(&leader);
    (void)memset(leader.members, 0, sizeof(leader.members));
    (void)memset(leader.pending_members, 0, sizeof(leader.pending_members));
    member.joined = 0U;
    member.state = SLE_TEAM_NET_DISCOVERING;
    member.last_hello_s = 0U;
    leader_rt.last_tx_len = 0U;
    member_rt.last_tx_len = 0U;

    assert(sle_team_node_pairing_start(&leader) == SLE_TEAM_OK);
    member_rt.now_s = 9U;
    leader_rt.now_s = 9U;
    sle_team_node_tick(&member);
    relay_last_packet(&member_rt, &leader);
    assert(sle_team_node_find_member(&leader, 2U) == NULL);
    assert(leader.pending_members[0].active != 0U);
    assert(leader.pending_members[0].member_id == 2U);
    assert(member.joined == 0U);
    relay_last_packet(&member_rt, &leader);
    assert(sle_team_node_find_member(&leader, 2U) == NULL);
    assert(leader.pending_members[0].active != 0U);

    assert(sle_team_node_pairing_approve(&leader, 2U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &member);
    relay_last_packet(&leader_rt, &member);
    assert(member.joined != 0U);
    assert(sle_team_node_send_config(&leader, 2U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &member);
    assert(member.cfg.relay_allowed == 0U);
    assert(member.cfg.relay_enabled == 0U);
    assert(leader.cfg.member_filter_enabled != 0U);
    assert(sle_team_node_is_member_allowed(&leader, 2U) != 0U);
    assert(sle_team_node_pairing_start(&leader) == SLE_TEAM_OK);
    assert(sle_team_node_send_hello(&member, 1U) == SLE_TEAM_OK);
    relay_last_packet(&member_rt, &leader);
    assert(sle_team_node_find_member(&leader, 2U) != NULL);
    assert(leader.pending_members[0].active == 0U);
    assert(sle_team_node_is_member_allowed(&leader, 2U) != 0U);

    assert(sle_team_node_member_leave(&member) == SLE_TEAM_OK);
    assert(member.joined == 0U);
    assert(member.cfg.relay_enabled == 0U);
    assert(member.state == SLE_TEAM_NET_DISCOVERING);
    assert(member.upstream_parent_state == SLE_TEAM_PARENT_DISCOVERING);
    assert(member.upstream_parent_reselect_pending == 0U);
    assert(sle_team_node_member_select_leader(&member, 4U, 1U, 0x22U) == SLE_TEAM_OK);
    assert(member.cfg.team_id == 4U);
    assert(member.cfg.leader_id == 1U);
    assert(member.cfg.channel_hash == 0x22U);
    assert(member.joined == 0U);

    member_rt.now_s = 3U;
    sle_team_node_tick(&member);
    relay_last_packet(&member_rt, &leader);
    relay_last_packet(&leader_rt, &member);
    relay_last_packet(&leader_rt, &member);
    assert(member.cfg.relay_enabled == 0U);

    assert(sle_team_node_member_select_leader(&member, 1U, 1U, 0x11U) == SLE_TEAM_OK);
    member.cfg.default_ttl = 2U;
    {
        sle_team_app_packet_t route_app;
        sle_team_route_update_body_t route_update = {0};

        route_update.parent_id = 3U;
        route_update.next_hop_id = 3U;
        route_update.parent_state = 2U;
        assert(sle_team_node_send_route_update(&member, 1U, route_update.parent_id,
            route_update.parent_state, route_update.next_hop_id) == SLE_TEAM_OK);
        assert(member_rt.last_tx_len != 0U);
        assert(demo_decode_last_app_packet(&member_rt, &route_app) != 0U);
        assert(route_app.app_msg_type == SLE_TEAM_APP_ROUTE_UPDATE);
        assert(route_app.body_len == sizeof(sle_team_route_update_body_t));
        assert(((const sle_team_route_update_body_t *)route_app.body)->parent_id == 3U);
        assert(((const sle_team_route_update_body_t *)route_app.body)->next_hop_id == 3U);
        assert(((const sle_team_route_update_body_t *)route_app.body)->parent_state == 2U);
        member_rt.last_tx_len = 0U;
    }

    leader.cfg.default_ttl = 2U;
    leader_rt.last_tx_len = 0U;
    assert(sle_team_node_send_route_update(&leader, 2U, 3U, 2U, 3U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &member);
    assert(member.upstream_parent_state == SLE_TEAM_PARENT_CONNECTED);
    assert(member.upstream_parent_id == 3U);
    assert(member.upstream_parent_reselect_pending == 0U);

    member.last_leader_seen_s = 1U;
    member.cfg.heartbeat_timeout_s = 2U;
    member.joined = 1U;
    member.state = SLE_TEAM_NET_ONLINE;
    member_rt.now_s = 6U;
    sle_team_node_tick(&member);
    assert(member.joined == 0U);
    assert(member.state == SLE_TEAM_NET_JOINING);
    assert(member.upstream_parent_state == SLE_TEAM_PARENT_RESELECTING);
    assert(member.upstream_parent_reselect_pending != 0U);
    assert(member.upstream_parent_id == 3U);

    (void)memset(&pos, 0, sizeof(pos));
    pos.latitude_e6 = 39908456;
    pos.longitude_e6 = 116397128;
    pos.speed_cms = 100U;
    pos.heading_deg = 90U;
    pos.battery_percent = 88U;
    pos.fix_status = 1U;
    pos.sat_count = 9U;
    (void)sle_team_node_send_position(&member, 1U, &pos);
    relay_last_packet(&member_rt, &relay);
    assert(relay_rt.last_tx_len != 0U);
    relay_last_packet(&relay_rt, &leader);

    leader_cfg.default_ttl = 2U;
    leader.cfg.default_ttl = 2U;
    assert(sle_team_node_send_config(&leader, 2U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &relay);
    relay_last_packet(&relay_rt, &member);
    assert(member.cfg.report_interval_s == leader.cfg.report_interval_s);

    return 0;
}
#endif
