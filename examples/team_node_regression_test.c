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

static void test_join_member(sle_team_node_t *leader, test_runtime_t *leader_rt,
    sle_team_node_t *member, test_runtime_t *member_rt)
{
    assert(sle_team_node_send_hello(member, member->cfg.leader_id) == SLE_TEAM_OK);
    test_deliver_last(member_rt, leader);
    test_deliver_last(leader_rt, member);
    assert(sle_team_node_find_member(leader, member->cfg.self_id) != NULL);
    assert(member->joined != 0U);
}

static uint8_t test_count_member_records(const sle_team_node_t *node, uint8_t member_id)
{
    uint8_t count = 0U;

    if (node == NULL) {
        return 0U;
    }
    for (uint8_t i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->members[i].member_id == member_id) {
            count++;
        }
    }
    return count;
}

static const sle_team_member_record_t *test_find_member_record(const sle_team_node_t *node, uint8_t member_id)
{
    if (node == NULL) {
        return NULL;
    }
    for (uint8_t i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        if (node->members[i].member_id == member_id) {
            return &node->members[i];
        }
    }
    return NULL;
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

static void test_offline_member_rejoin_reuses_logical_slot(void)
{
    test_runtime_t leader_rt = {.name = "leader", .now_s = 60U};
    test_runtime_t member2_rt = {.name = "member2", .now_s = 60U};
    test_runtime_t member3_rt = {.name = "member3", .now_s = 60U};
    sle_team_node_t leader;
    sle_team_node_t member2;
    sle_team_node_t member3;

    test_init_leader(&leader, &leader_rt);
    test_init_member(&member2, &member2_rt, 2U);
    test_init_member(&member3, &member3_rt, 3U);

    assert(sle_team_node_send_hello(&member2, 1U) == SLE_TEAM_OK);
    assert(sle_team_node_on_packet(&leader, member2_rt.last_tx, member2_rt.last_tx_len) == SLE_TEAM_OK);
    assert(sle_team_node_send_hello(&member3, 1U) == SLE_TEAM_OK);
    assert(sle_team_node_on_packet(&leader, member3_rt.last_tx, member3_rt.last_tx_len) == SLE_TEAM_OK);
    assert(test_count_member_records(&leader, 2U) == 1U);
    assert(test_count_member_records(&leader, 3U) == 1U);

    leader.members[0].online = 0U;
    leader.members[1].online = 0U;
    leader_rt.now_s = 61U;
    member3_rt.now_s = 61U;

    assert(sle_team_node_send_hello(&member3, 1U) == SLE_TEAM_OK);
    assert(sle_team_node_on_packet(&leader, member3_rt.last_tx, member3_rt.last_tx_len) == SLE_TEAM_OK);
    assert(test_count_member_records(&leader, 2U) == 1U);
    assert(test_count_member_records(&leader, 3U) == 1U);
    assert(sle_team_node_find_member(&leader, 2U) == NULL);
    assert(sle_team_node_find_member(&leader, 3U) != NULL);
}

static void test_allowed_list_seeds_offline_logical_members(void)
{
    test_runtime_t leader_rt = {.name = "leader", .now_s = 70U};
    test_runtime_t member5_rt = {.name = "member5", .now_s = 70U};
    sle_team_node_t leader;
    sle_team_node_t member5;
    uint8_t allowed[SLE_TEAM_MAX_MEMBERS];

    test_init_leader(&leader, &leader_rt);
    test_init_member(&member5, &member5_rt, 5U);
    for (uint8_t i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        allowed[i] = (uint8_t)(i + 2U);
    }

    assert(sle_team_node_set_allowed_members(&leader, allowed, SLE_TEAM_MAX_MEMBERS) == SLE_TEAM_OK);
    assert(leader.cfg.allowed_member_count == SLE_TEAM_MAX_MEMBERS);
    for (uint8_t i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &leader.members[i];

        assert(member->member_id == allowed[i]);
        assert(member->online == 0U);
    }

    assert(sle_team_node_send_hello(&member5, 1U) == SLE_TEAM_OK);
    assert(sle_team_node_on_packet(&leader, member5_rt.last_tx, member5_rt.last_tx_len) == SLE_TEAM_OK);
    assert(test_count_member_records(&leader, 5U) == 1U);
    assert(sle_team_node_find_member(&leader, 5U) != NULL);
}

static void test_add_allowed_member_failure_keeps_allow_all(void)
{
    test_runtime_t leader_rt = {.name = "leader", .now_s = 80U};
    sle_team_node_t leader;

    test_init_leader(&leader, &leader_rt);
    assert(sle_team_node_allow_all_members(&leader) == SLE_TEAM_OK);
    for (uint8_t i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        leader.members[i].member_id = (uint8_t)(i + 2U);
        leader.members[i].online = 1U;
    }

    assert(sle_team_node_add_allowed_member(&leader, 40U) == SLE_TEAM_ERR_BUF);
    assert(leader.cfg.member_filter_enabled == 0U);
    assert(leader.cfg.allowed_member_count == 0U);
}

static void test_rebooted_member_rejoins_after_timeout(void)
{
    test_runtime_t leader_rt = {.name = "leader", .now_s = 90U};
    test_runtime_t member_rt = {.name = "member", .now_s = 90U};
    test_runtime_t rebooted_rt = {.name = "rebooted", .now_s = 104U};
    sle_team_node_t leader;
    sle_team_node_t member;
    sle_team_node_t rebooted;
    const sle_team_member_record_t *record;

    test_init_leader(&leader, &leader_rt);
    test_init_member(&member, &member_rt, 2U);
    test_join_member(&leader, &leader_rt, &member, &member_rt);

    record = test_find_member_record(&leader, 2U);
    assert(record != NULL && record->online != 0U);

    test_init_member(&rebooted, &rebooted_rt, 2U);
    leader_rt.now_s = 104U;
    sle_team_node_tick(&leader);
    record = test_find_member_record(&leader, 2U);
    assert(record != NULL && record->online == 0U);
    leader_rt.last_tx_len = 0U;

    assert(sle_team_node_send_hello(&rebooted, rebooted.cfg.leader_id) == SLE_TEAM_OK);
    test_deliver_last(&rebooted_rt, &leader);
    record = test_find_member_record(&leader, 2U);
    assert(record != NULL && record->online != 0U);
    test_deliver_last(&leader_rt, &rebooted);
    assert(rebooted.joined != 0U);
}

static void test_member_leave_notifies_leader_and_manual_rejoin(void)
{
    test_runtime_t leader_rt = {.name = "leader", .now_s = 120U};
    test_runtime_t member_rt = {.name = "member", .now_s = 120U};
    sle_team_node_t leader;
    sle_team_node_t member;
    const sle_team_member_record_t *record;

    test_init_leader(&leader, &leader_rt);
    test_init_member(&member, &member_rt, 2U);
    test_join_member(&leader, &leader_rt, &member, &member_rt);

    assert(sle_team_node_member_leave(&member) == SLE_TEAM_OK);
    test_deliver_last(&member_rt, &leader);
    record = test_find_member_record(&leader, 2U);
    assert(record != NULL && record->online == 0U);
    assert(member.joined == 0U);
    assert(member.state == SLE_TEAM_NET_IDLE);
    assert(member.cfg.leader_id == 0U);

    member_rt.now_s = 130U;
    member_rt.last_tx_len = 0U;
    sle_team_node_tick(&member);
    assert(member_rt.last_tx_len == 0U);

    assert(sle_team_node_member_select_leader(&member, 1U, 1U, 0x11U) == SLE_TEAM_OK);
    test_deliver_last(&member_rt, &leader);
    record = test_find_member_record(&leader, 2U);
    assert(record != NULL && record->online != 0U);
    test_deliver_last(&leader_rt, &member);
    assert(member.joined != 0U);
}

static void test_member_link_lost_preserves_leader_and_rejoins(void)
{
    test_runtime_t leader_rt = {.name = "leader", .now_s = 150U};
    test_runtime_t member_rt = {.name = "member", .now_s = 150U};
    sle_team_node_t leader;
    sle_team_node_t member;
    const sle_team_member_record_t *record;

    test_init_leader(&leader, &leader_rt);
    test_init_member(&member, &member_rt, 2U);
    test_join_member(&leader, &leader_rt, &member, &member_rt);
    member.cfg.relay_allowed = 1U;
    member.cfg.relay_enabled = 1U;
    member.cfg.relay_tier = 1U;
    member.cfg.max_downstream = 4U;
    member.upstream_parent_id = member.cfg.leader_id;
    member.upstream_parent_state = SLE_TEAM_PARENT_CONNECTED;
    member.last_parent_seen_s = 150U;

    assert(sle_team_node_member_link_lost(&member) == SLE_TEAM_OK);
    assert(member.joined == 0U);
    assert(member.state == SLE_TEAM_NET_DISCOVERING);
    assert(member.cfg.leader_id == 1U);
    assert(member.cfg.relay_allowed == 1U);
    assert(member.cfg.relay_enabled == 0U);
    assert(member.cfg.relay_tier == 1U);
    assert(member.cfg.max_downstream == 4U);
    assert(member.upstream_parent_state == SLE_TEAM_PARENT_RESELECTING);
    assert(member.upstream_parent_reselect_pending != 0U);

    member_rt.last_tx_len = 0U;
    member_rt.now_s = 153U;
    sle_team_node_tick(&member);
    assert(member_rt.last_tx_len != 0U);
    test_deliver_last(&member_rt, &leader);
    record = test_find_member_record(&leader, 2U);
    assert(record != NULL && record->online != 0U);
    test_deliver_last(&leader_rt, &member);
    assert(member.joined != 0U);
}

int main(void)
{
    test_broadcast_relay_failure_keeps_local_processing();
    test_hello_config_failure_does_not_commit_join();
    test_pairing_stop_config_failure_is_retryable();
    test_member_heartbeat_does_not_pollute_members_table();
    test_offline_member_rejoin_reuses_logical_slot();
    test_allowed_list_seeds_offline_logical_members();
    test_add_allowed_member_failure_keeps_allow_all();
    test_rebooted_member_rejoins_after_timeout();
    test_member_leave_notifies_leader_and_manual_rejoin();
    test_member_link_lost_preserves_leader_and_rejoins();
    printf("[team-node-regression] pass\n");
    return 0;
}
