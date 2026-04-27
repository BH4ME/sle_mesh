#ifndef SLE_TEAM_NODE_H
#define SLE_TEAM_NODE_H

#include "sle_team_packet.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SLE_TEAM_MAX_MEMBERS 8U
#define SLE_TEAM_NODE_TX_BUF_SIZE 256U
#define SLE_TEAM_BROADCAST_ID 0xFFU

typedef enum {
    SLE_TEAM_ROLE_MEMBER = 0,
    SLE_TEAM_ROLE_LEADER = 1,
} sle_team_node_role_t;

typedef enum {
    SLE_TEAM_NET_IDLE = 0,
    SLE_TEAM_NET_DISCOVERING = 1,
    SLE_TEAM_NET_JOINING = 2,
    SLE_TEAM_NET_ONLINE = 3,
} sle_team_network_state_t;

typedef enum {
    SLE_TEAM_SEND_UNICAST = 0,
    SLE_TEAM_SEND_GROUP = 1,
} sle_team_send_kind_t;

typedef struct {
    uint8_t member_id;
    uint8_t role;
    uint8_t battery_percent;
    uint8_t online;
    uint8_t fix_status;
    int8_t last_rssi_dbm;
    uint16_t last_seq;
    uint32_t last_seen_s;
} sle_team_member_record_t;

typedef struct sle_team_node sle_team_node_t;

typedef int (*sle_team_send_fn)(void *user_ctx, sle_team_send_kind_t kind, uint8_t dst_id,
    const uint8_t *buf, uint16_t len);
typedef uint32_t (*sle_team_now_fn)(void *user_ctx);
typedef void (*sle_team_log_fn)(void *user_ctx, const char *text);

typedef void (*sle_team_joined_cb)(void *user_ctx, uint8_t member_id);
typedef void (*sle_team_position_cb)(void *user_ctx, uint8_t member_id, const sle_team_pos_body_t *pos);
typedef void (*sle_team_alert_cb)(void *user_ctx, uint8_t member_id, uint8_t reason);

typedef struct {
    sle_team_send_fn send;
    sle_team_now_fn now_s;
    sle_team_log_fn log;
    sle_team_joined_cb on_joined;
    sle_team_position_cb on_position;
    sle_team_alert_cb on_alert;
    void *user_ctx;
} sle_team_node_ops_t;

typedef struct {
    uint8_t team_id;
    uint8_t self_id;
    uint8_t leader_id;
    sle_team_node_role_t role;
    uint8_t channel_hash;
    uint16_t report_interval_s;
    uint16_t heartbeat_interval_s;
    uint16_t warn_distance_m;
    uint16_t lost_distance_m;
    uint16_t heartbeat_timeout_s;
} sle_team_node_cfg_t;

struct sle_team_node {
    sle_team_node_cfg_t cfg;
    sle_team_node_ops_t ops;
    sle_team_network_state_t state;
    uint16_t next_seq;
    uint32_t last_hello_s;
    uint32_t last_heartbeat_s;
    uint32_t last_config_s;
    uint8_t joined;
    sle_team_member_record_t members[SLE_TEAM_MAX_MEMBERS];
};

int sle_team_node_init(sle_team_node_t *node, const sle_team_node_cfg_t *cfg, const sle_team_node_ops_t *ops);
void sle_team_node_tick(sle_team_node_t *node);
int sle_team_node_on_packet(sle_team_node_t *node, const uint8_t *buf, size_t buf_len);

int sle_team_node_send_hello(sle_team_node_t *node, uint8_t dst_id);
int sle_team_node_send_heartbeat(sle_team_node_t *node, uint8_t dst_id, uint8_t battery_percent,
    int8_t rssi_dbm, uint8_t fix_status);
int sle_team_node_send_position(sle_team_node_t *node, uint8_t dst_id, const sle_team_pos_body_t *pos);
int sle_team_node_send_alert(sle_team_node_t *node, uint8_t dst_id, const sle_team_alert_body_t *alert);
int sle_team_node_send_config(sle_team_node_t *node, uint8_t dst_id);
int sle_team_node_send_ack(sle_team_node_t *node, uint8_t dst_id, uint16_t ack_seq, uint8_t acked_msg_type,
    uint8_t status_code);

const sle_team_member_record_t *sle_team_node_find_member(const sle_team_node_t *node, uint8_t member_id);

#ifdef __cplusplus
}
#endif

#endif
