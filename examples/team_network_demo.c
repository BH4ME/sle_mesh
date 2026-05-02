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

#ifdef SLE_TEAM_NETWORK_TEST
int main(void)
{
    demo_runtime_t leader_rt = {.name = "leader"};
    demo_runtime_t member_rt = {.name = "member"};
    sle_team_node_t leader;
    sle_team_node_t member;
    sle_team_node_cfg_t leader_cfg;
    sle_team_node_cfg_t member_cfg;
    sle_team_node_ops_t leader_ops;
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

    member_cfg = leader_cfg;
    member_cfg.self_id = 2U;
    member_cfg.role = SLE_TEAM_ROLE_MEMBER;

    (void)memset(&leader_ops, 0, sizeof(leader_ops));
    leader_ops.send = demo_send;
    leader_ops.now_s = demo_now;
    leader_ops.log = demo_log;
    leader_ops.on_joined = demo_joined;
    leader_ops.on_position = demo_position;
    leader_ops.user_ctx = &leader_rt;

    member_ops = leader_ops;
    member_ops.user_ctx = &member_rt;

    (void)sle_team_node_init(&leader, &leader_cfg, &leader_ops);
    (void)sle_team_node_init(&member, &member_cfg, &member_ops);

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

    member_rt.now_s = 3U;
    sle_team_node_tick(&member);
    relay_last_packet(&member_rt, &leader);
    relay_last_packet(&leader_rt, &member);
    relay_last_packet(&leader_rt, &member);

    (void)memset(&pos, 0, sizeof(pos));
    pos.latitude_e6 = 39908456;
    pos.longitude_e6 = 116397128;
    pos.speed_cms = 100U;
    pos.heading_deg = 90U;
    pos.battery_percent = 88U;
    pos.fix_status = 1U;
    pos.sat_count = 9U;
    (void)sle_team_node_send_position(&member, 1U, &pos);
    relay_last_packet(&member_rt, &leader);

    return 0;
}
#endif
