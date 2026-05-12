#include "sle_team_node.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t now_s;
    uint8_t last_tx[256];
    uint16_t last_tx_len;
    uint8_t fail_next_tx;
    uint8_t joined_count;
    const char *name;
} test_runtime_t;

static int test_send(void *user_ctx, sle_team_send_kind_t kind, uint8_t dst_id, const uint8_t *buf, uint16_t len)
{
    test_runtime_t *rt = (test_runtime_t *)user_ctx;

    (void)kind;
    (void)dst_id;
    if (rt == NULL || buf == NULL || len > sizeof(rt->last_tx)) {
        return -1;
    }
    if (rt->fail_next_tx != 0U) {
        rt->fail_next_tx = 0U;
        return -1;
    }
    (void)memcpy(rt->last_tx, buf, len);
    rt->last_tx_len = len;
    return SLE_TEAM_OK;
}

static uint32_t test_now(void *user_ctx)
{
    const test_runtime_t *rt = (const test_runtime_t *)user_ctx;
    return rt == NULL ? 0U : rt->now_s;
}

static void test_log(void *user_ctx, const char *text)
{
    const test_runtime_t *rt = (const test_runtime_t *)user_ctx;
    printf("[%s] %s\n", rt == NULL ? "?" : rt->name, text);
}

static void test_joined(void *user_ctx, uint8_t member_id)
{
    test_runtime_t *rt = (test_runtime_t *)user_ctx;

    (void)member_id;
    if (rt != NULL) {
        rt->joined_count++;
    }
}

static void test_init_leader(sle_team_node_t *node, test_runtime_t *rt)
{
    sle_team_node_cfg_t cfg;
    sle_team_node_ops_t ops;

    (void)memset(&cfg, 0, sizeof(cfg));
    cfg.team_id = 1U;
    cfg.self_id = 1U;
    cfg.leader_id = 1U;
    cfg.role = SLE_TEAM_ROLE_LEADER;
    cfg.channel_hash = 0x11U;
    cfg.report_interval_s = 5U;
    cfg.warn_distance_m = 50U;
    cfg.lost_distance_m = 80U;
    cfg.heartbeat_timeout_s = 10U;
    cfg.default_ttl = 2U;

    (void)memset(&ops, 0, sizeof(ops));
    ops.send = test_send;
    ops.now_s = test_now;
    ops.log = test_log;
    ops.on_joined = test_joined;
    ops.user_ctx = rt;

    assert(sle_team_node_init(node, &cfg, &ops) == SLE_TEAM_OK);
}

static void test_init_member(sle_team_node_t *node, test_runtime_t *rt, uint8_t self_id)
{
    sle_team_node_cfg_t cfg;
    sle_team_node_ops_t ops;

    (void)memset(&cfg, 0, sizeof(cfg));
    cfg.team_id = 1U;
    cfg.self_id = self_id;
    cfg.leader_id = 1U;
    cfg.role = SLE_TEAM_ROLE_MEMBER;
    cfg.channel_hash = 0x11U;
    cfg.report_interval_s = 5U;
    cfg.warn_distance_m = 50U;
    cfg.lost_distance_m = 80U;
    cfg.heartbeat_timeout_s = 10U;
    cfg.default_ttl = 2U;

    (void)memset(&ops, 0, sizeof(ops));
    ops.send = test_send;
    ops.now_s = test_now;
    ops.log = test_log;
    ops.on_joined = test_joined;
    ops.user_ctx = rt;

    assert(sle_team_node_init(node, &cfg, &ops) == SLE_TEAM_OK);
}

static void test_deliver_last(test_runtime_t *from, sle_team_node_t *to)
{
    if (from->last_tx_len == 0U) {
        return;
    }
    assert(sle_team_node_on_packet(to, from->last_tx, from->last_tx_len) == SLE_TEAM_OK);
    from->last_tx_len = 0U;
}

static void test_broadcast_relay_failure_keeps_local_processing(void)
{
    test_runtime_t leader_rt = {.name = "leader", .now_s = 20U};
    test_runtime_t relay_rt = {.name = "relay", .now_s = 20U};
    sle_team_node_t leader;
    sle_team_node_t relay;

    test_init_leader(&leader, &leader_rt);
    test_init_member(&relay, &relay_rt, 3U);
    relay.joined = 1U;
    relay.cfg.relay_allowed = 1U;
    relay.cfg.relay_enabled = 1U;
    relay.last_leader_seen_s = 10U;

    assert(sle_team_node_send_heartbeat(&leader, SLE_TEAM_BROADCAST_ID, 91U, -42, 1U) == SLE_TEAM_OK);
    relay_rt.fail_next_tx = 1U;
    assert(sle_team_node_on_packet(&relay, leader_rt.last_tx, leader_rt.last_tx_len) == SLE_TEAM_OK);
    assert(relay.last_leader_seen_s == 20U);
}

static void test_hello_config_failure_does_not_commit_join(void)
{
    test_runtime_t leader_rt = {.name = "leader", .now_s = 30U};
    test_runtime_t member_rt = {.name = "member", .now_s = 30U};
    sle_team_node_t leader;
    sle_team_node_t member;

    test_init_leader(&leader, &leader_rt);
    test_init_member(&member, &member_rt, 2U);

    assert(sle_team_node_send_hello(&member, 1U) == SLE_TEAM_OK);
    leader_rt.fail_next_tx = 1U;
    assert(sle_team_node_on_packet(&leader, member_rt.last_tx, member_rt.last_tx_len) != SLE_TEAM_OK);
    assert(leader_rt.joined_count == 0U);
    assert(sle_team_node_find_member(&leader, 2U) == NULL);
}

static void test_pairing_stop_config_failure_is_retryable(void)
{
    test_runtime_t leader_rt = {.name = "leader", .now_s = 40U};
    test_runtime_t relay_rt = {.name = "relay", .now_s = 40U};
    sle_team_node_t leader;
    sle_team_node_t relay;

    test_init_leader(&leader, &leader_rt);
    test_init_member(&relay, &relay_rt, 3U);
    relay.joined = 1U;

    assert(sle_team_node_pairing_approve_with_relay(&leader, 3U, 1U) == SLE_TEAM_OK);
    leader_rt.last_tx_len = 0U;
    assert(sle_team_node_pairing_start(&leader) == SLE_TEAM_OK);
    test_deliver_last(&leader_rt, &relay);
    assert(relay.cfg.relay_discovery_only != 0U);

    leader_rt.fail_next_tx = 1U;
    assert(sle_team_node_pairing_stop(&leader) != SLE_TEAM_OK);
    assert(relay.cfg.relay_discovery_only != 0U);

    assert(sle_team_node_pairing_stop(&leader) == SLE_TEAM_OK);
    test_deliver_last(&leader_rt, &relay);
    assert(relay.cfg.relay_discovery_only == 0U);
}

static void test_member_heartbeat_does_not_pollute_members_table(void)
{
    test_runtime_t leader_rt = {.name = "leader", .now_s = 50U};
    test_runtime_t member_rt = {.name = "member", .now_s = 50U};
    sle_team_node_t leader;
    sle_team_node_t member;

    test_init_leader(&leader, &leader_rt);
    test_init_member(&member, &member_rt, 2U);
    member.joined = 1U;
    member.last_leader_seen_s = 10U;

    assert(sle_team_node_send_heartbeat(&leader, SLE_TEAM_BROADCAST_ID, 92U, -35, 1U) == SLE_TEAM_OK);
    assert(sle_team_node_on_packet(&member, leader_rt.last_tx, leader_rt.last_tx_len) == SLE_TEAM_OK);
    assert(member.last_leader_seen_s == 50U);
    assert(sle_team_node_find_member(&member, 1U) == NULL);
}

int main(void)
{
    test_broadcast_relay_failure_keeps_local_processing();
    test_hello_config_failure_does_not_commit_join();
    test_pairing_stop_config_failure_is_retryable();
    test_member_heartbeat_does_not_pollute_members_table();
    printf("[team-node-regression] pass\n");
    return 0;
}
