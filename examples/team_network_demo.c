#include "sle_team_node.h"
#include "sle_team_web_api.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t now_s;
    uint8_t last_tx[256];
    uint16_t last_tx_len;
    uint8_t relay_offline_count;
    uint8_t relay_offline_last_member;
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

static void demo_relay_offline(void *user_ctx, uint8_t member_id)
{
    demo_runtime_t *rt = (demo_runtime_t *)user_ctx;

    if (rt == NULL) {
        return;
    }
    rt->relay_offline_count++;
    rt->relay_offline_last_member = member_id;
    printf("[%s] relay offline event member=%u\n", rt->name, member_id);
}

static void relay_last_packet(demo_runtime_t *from, sle_team_node_t *to)
{
    if (from->last_tx_len > 0U) {
        (void)sle_team_node_on_packet(to, from->last_tx, from->last_tx_len);
        from->last_tx_len = 0U;
    }
}

static int demo_send_fail_once(void *user_ctx, sle_team_send_kind_t kind, uint8_t dst_id, const uint8_t *buf, uint16_t len)
{
    demo_runtime_t *rt = (demo_runtime_t *)user_ctx;

    if (rt == NULL) {
        return -1;
    }
    if (rt->last_tx_len == 0xFFFFU) {
        rt->last_tx_len = 0U;
        return -1;
    }
    return demo_send(user_ctx, kind, dst_id, buf, len);
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
    leader_ops.on_relay_offline = demo_relay_offline;
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
    relay.last_leader_seen_s = 1U;
    relay.cfg.heartbeat_timeout_s = 2U;
    relay_rt.now_s = 6U;
    sle_team_node_tick(&relay);
    assert(relay.joined == 0U);
    assert(relay.cfg.relay_allowed != 0U);
    assert(relay.cfg.relay_enabled == 0U);
    assert(relay.upstream_parent_state == SLE_TEAM_PARENT_RESELECTING);
    relay_rt.now_s = 9U;
    leader_rt.now_s = 9U;
    sle_team_node_tick(&relay);
    relay_last_packet(&relay_rt, &leader);
    relay_last_packet(&leader_rt, &relay);
    assert(relay.joined != 0U);
    assert(relay.state == SLE_TEAM_NET_ONLINE);
    assert(relay.cfg.relay_allowed != 0U);
    assert(relay.cfg.relay_enabled != 0U);
    assert(relay.upstream_parent_state == SLE_TEAM_PARENT_CONNECTED);

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
    {
        sle_team_node_ops_t fail_once_ops = leader.ops;
        fail_once_ops.send = demo_send_fail_once;
        fail_once_ops.user_ctx = &leader_rt;
        leader.ops = fail_once_ops;
        leader_rt.last_tx_len = 0xFFFFU;
        assert(sle_team_node_pairing_approve(&leader, 2U) != SLE_TEAM_OK);
        assert(leader.pending_members[0].active != 0U);
        assert(leader.pending_members[0].member_id == 2U);
        leader.ops = leader_ops;
    }
    {
        sle_team_node_ops_t fail_once_ops = leader.ops;
        fail_once_ops.send = demo_send_fail_once;
        fail_once_ops.user_ctx = &leader_rt;
        leader.ops = fail_once_ops;
        leader_rt.last_tx_len = 0xFFFFU;
        assert(sle_team_node_pairing_stop(&leader) != SLE_TEAM_OK);
        assert(leader.pending_members[0].active != 0U);
        assert(leader.pending_members[0].member_id == 2U);
        leader.ops = leader_ops;
        assert(sle_team_node_pairing_start(&leader) == SLE_TEAM_OK);
    }
    relay_last_packet(&member_rt, &leader);
    assert(sle_team_node_find_member(&leader, 2U) != NULL);
    assert(leader.pending_members[0].active == 0U);

    assert(sle_team_node_pairing_approve(&leader, 2U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &member);
    relay_last_packet(&leader_rt, &member);
    assert(member.joined != 0U);
    member.joined = 0U;
    member.state = SLE_TEAM_NET_DISCOVERING;
    assert(sle_team_node_send_ack(&leader, 2U, 0U, SLE_TEAM_APP_HELLO, 1U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &member);
    assert(member.joined == 0U);
    assert(member.state == SLE_TEAM_NET_DISCOVERING);
    assert(sle_team_node_send_ack(&leader, 2U, 0U, SLE_TEAM_APP_HELLO, 0U) == SLE_TEAM_OK);
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
    assert(member.cfg.relay_allowed == 0U);

    /* Approve API default is non-relay; relay grant must be explicit and can be toggled later. */
    assert(sle_team_node_pairing_approve(&leader, 2U) == SLE_TEAM_OK);
    assert(sle_team_node_send_config(&leader, 2U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &member);
    assert(member.cfg.relay_allowed == 0U);
    assert(sle_team_node_pairing_approve_with_relay(&leader, 2U, 1U) == SLE_TEAM_OK);
    assert(sle_team_node_send_config(&leader, 2U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &member);
    assert(member.cfg.relay_allowed != 0U);
    assert(sle_team_node_send_ack(&leader, 2U, 0U, SLE_TEAM_APP_HELLO, 0U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &member);

    assert(sle_team_node_pairing_start(&leader) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &member);
    assert(member.cfg.relay_discovery_only != 0U);
    assert(sle_team_node_pairing_stop(&leader) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &member);
    assert(member.cfg.relay_discovery_only == 0U);

    (void)memset(&pos, 0, sizeof(pos));
    pos.latitude_e6 = 39908456;
    pos.longitude_e6 = 116397128;
    pos.speed_cms = 100U;
    pos.heading_deg = 90U;
    pos.battery_percent = 88U;
    pos.fix_status = 1U;
    pos.sat_count = 9U;

    member_rt.last_tx_len = 0U;
    relay_rt.last_tx_len = 0U;
    relay.cfg.relay_enabled = 1U;
    relay.cfg.relay_discovery_only = 1U;
    assert(sle_team_node_send_position(&member, 1U, &pos) == SLE_TEAM_OK);
    relay_last_packet(&member_rt, &relay);
    assert(relay_rt.last_tx_len == 0U);
    relay_rt.last_tx_len = 0U;
    assert(sle_team_node_send_route_update(&member, 1U, 3U, (uint8_t)SLE_TEAM_PARENT_CONNECTED, 3U) == SLE_TEAM_OK);
    relay_last_packet(&member_rt, &relay);
    assert(relay_rt.last_tx_len != 0U);
    relay_rt.last_tx_len = 0U;
    relay.cfg.relay_discovery_only = 0U;

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

    (void)sle_team_node_send_position(&member, 1U, &pos);
    relay_last_packet(&member_rt, &relay);
    assert(relay_rt.last_tx_len != 0U);
    relay_last_packet(&relay_rt, &leader);

    {
        demo_runtime_t cb_leader_rt = {.name = "cb-leader", .now_s = 20U};
        sle_team_node_t cb_leader;
        sle_team_node_cfg_t cb_cfg = leader_cfg;
        sle_team_node_ops_t cb_ops = leader_ops;
        uint8_t i;

        cb_ops.user_ctx = &cb_leader_rt;
        assert(sle_team_node_init(&cb_leader, &cb_cfg, &cb_ops) == SLE_TEAM_OK);
        assert(sle_team_node_pairing_approve_with_relay(&cb_leader, 3U, 1U) == SLE_TEAM_OK);
        for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
            if (cb_leader.members[i].member_id == 3U && cb_leader.members[i].online != 0U) {
                cb_leader.members[i].last_seen_s = 1U;
                break;
            }
        }
        assert(cb_leader_rt.relay_offline_count == 0U);
        sle_team_node_tick(&cb_leader);
        assert(cb_leader_rt.relay_offline_count == 1U);
        assert(cb_leader_rt.relay_offline_last_member == 3U);
    }

    {
        sle_team_app_packet_t parent_switch_app;

        member.joined = 1U;
        member.state = SLE_TEAM_NET_ONLINE;
        member.cfg.relay_allowed = 1U;
        member.cfg.relay_enabled = 1U;
        member.cfg.heartbeat_interval_s = 0U;
        member.cfg.parent_timeout_s = 2U;
        member.cfg.heartbeat_timeout_s = 10U;
        member.upstream_parent_id = 3U;
        member.upstream_parent_state = SLE_TEAM_PARENT_CONNECTED;
        member.upstream_parent_reselect_pending = 0U;
        member.last_leader_seen_s = 4U;
        member.last_parent_seen_s = 1U;
        member.members[0].member_id = 9U;
        member.members[0].online = 1U;
        member_rt.now_s = 6U;
        member_rt.last_tx_len = 0U;

        sle_team_node_tick(&member);
        assert(member.joined != 0U);
        assert(member.state == SLE_TEAM_NET_ONLINE);
        assert(member.cfg.relay_allowed != 0U);
        assert(member.cfg.relay_enabled != 0U);
        assert(member.members[0].online != 0U);
        assert(member.upstream_parent_state == SLE_TEAM_PARENT_RESELECTING);
        assert(member.upstream_parent_reselect_pending != 0U);
        assert(member.last_parent_seen_s == 0U);
        assert(demo_decode_last_app_packet(&member_rt, &parent_switch_app) != 0U);
        assert(parent_switch_app.app_msg_type == SLE_TEAM_APP_HELLO);
        assert(parent_switch_app.dst_id == member.cfg.leader_id);
    }

    {
        sle_team_app_packet_t parent_switch_retry_app;
        sle_team_send_fn original_send = member.ops.send;

        member.joined = 1U;
        member.state = SLE_TEAM_NET_ONLINE;
        member.cfg.relay_allowed = 1U;
        member.cfg.relay_enabled = 1U;
        member.cfg.heartbeat_interval_s = 0U;
        member.cfg.parent_timeout_s = 2U;
        member.cfg.heartbeat_timeout_s = 10U;
        member.upstream_parent_id = 3U;
        member.upstream_parent_state = SLE_TEAM_PARENT_CONNECTED;
        member.upstream_parent_reselect_pending = 0U;
        member.last_leader_seen_s = 4U;
        member.last_parent_seen_s = 1U;
        member_rt.now_s = 6U;
        member_rt.last_tx_len = 0U;

        member.ops.send = demo_send_fail_once;
        member_rt.last_tx_len = 0xFFFFU; /* fail first parent-switch hello once */
        sle_team_node_tick(&member);

        assert(member.joined != 0U);
        assert(member.state == SLE_TEAM_NET_ONLINE);
        assert(member.upstream_parent_state == SLE_TEAM_PARENT_RESELECTING);
        assert(member.upstream_parent_reselect_pending != 0U);
        assert(member.upstream_parent_id == 3U);
        assert(member.last_parent_seen_s == 1U);
        assert(member_rt.last_tx_len == 0U);

        member.ops.send = original_send;
        sle_team_node_tick(&member);
        assert(member.upstream_parent_id == 0U);
        assert(member.last_parent_seen_s == 0U);
        assert(demo_decode_last_app_packet(&member_rt, &parent_switch_retry_app) != 0U);
        assert(parent_switch_retry_app.app_msg_type == SLE_TEAM_APP_HELLO);
        assert(parent_switch_retry_app.dst_id == member.cfg.leader_id);
    }

    leader_cfg.default_ttl = 2U;
    leader.cfg.default_ttl = 2U;
    assert(sle_team_node_send_config(&leader, 2U) == SLE_TEAM_OK);
    relay_last_packet(&leader_rt, &relay);
    relay_last_packet(&relay_rt, &member);
    assert(member.cfg.report_interval_s == leader.cfg.report_interval_s);
    {
        uint16_t prev_report_interval = leader.cfg.report_interval_s;
        sle_team_node_cfg_t fake_cfg = leader_cfg;
        sle_team_node_t fake_member;
        demo_runtime_t fake_rt = {.name = "fake-member"};
        sle_team_node_ops_t fake_ops = leader_ops;

        fake_cfg.role = SLE_TEAM_ROLE_MEMBER;
        fake_cfg.self_id = 99U;
        fake_ops.user_ctx = &fake_rt;
        fake_ops.log = NULL;
        (void)sle_team_node_init(&fake_member, &fake_cfg, &fake_ops);
        assert(sle_team_node_send_config(&fake_member, 1U) == SLE_TEAM_OK);
        relay_last_packet(&fake_rt, &leader);
        assert(leader.cfg.report_interval_s == prev_report_interval);
    }
    {
        sle_team_node_t hb_member;
        sle_team_node_cfg_t hb_cfg = member_cfg;
        demo_runtime_t hb_rt = {.name = "hb-member", .now_s = 100U};
        sle_team_node_ops_t hb_ops = member_ops;

        hb_cfg.heartbeat_interval_s = 0U;
        hb_ops.user_ctx = &hb_rt;
        (void)sle_team_node_init(&hb_member, &hb_cfg, &hb_ops);
        hb_member.joined = 1U;
        hb_member.state = SLE_TEAM_NET_ONLINE;
        hb_rt.last_tx_len = 0U;
        sle_team_node_tick(&hb_member);
        assert(hb_rt.last_tx_len == 0U);
    }

    {
        char status_json[1024];
        sle_team_web_route_metrics_t web_metrics = {0};
        int status_len;

        web_metrics.active_count = 3U;
        web_metrics.direct_count = 1U;
        web_metrics.relayed_count = 2U;
        web_metrics.unreachable_count = 0U;
        web_metrics.stale_count = 0U;
        web_metrics.converged = 1U;
        web_metrics.relay_target_count = 2U;
        web_metrics.relay_online_count = 2U;
        web_metrics.epoch = 9U;
        web_metrics.last_change_s = 120U;
        web_metrics.last_converged_s = 123U;
        web_metrics.hint_sent_total = 7U;
        web_metrics.hint_failed_total = 1U;
        web_metrics.hint_cooldown_skipped_total = 4U;
        web_metrics.route_update_rx_total = 19U;
        web_metrics.route_reparent_total = 3U;
        web_metrics.route_reparent_last_s = 125U;
        status_len = sle_team_web_write_status_json(&leader, 126U, "unit-test", &web_metrics,
            status_json, sizeof(status_json));
        assert(status_len > 0);
        assert(strstr(status_json, "\"relayTarget\":2") != NULL);
        assert(strstr(status_json, "\"relayOnline\":2") != NULL);
        assert(strstr(status_json, "\"epoch\":9") != NULL);
        assert(strstr(status_json, "\"lastChangeS\":120") != NULL);
        assert(strstr(status_json, "\"lastConvergedS\":123") != NULL);
        assert(strstr(status_json, "\"routeHintSentTotal\":7") != NULL);
        assert(strstr(status_json, "\"routeHintFailedTotal\":1") != NULL);
        assert(strstr(status_json, "\"routeHintCooldownSkippedTotal\":4") != NULL);
        assert(strstr(status_json, "\"routeHintLastActivityS\"") == NULL);
        assert(strstr(status_json, "\"routeUpdateRxTotal\":19") != NULL);
        assert(strstr(status_json, "\"routeReparentTotal\":3") != NULL);
        assert(strstr(status_json, "\"routeReparentLastS\":125") != NULL);
    }

    return 0;
}
#endif
