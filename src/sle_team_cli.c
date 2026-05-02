#include "sle_team_cli.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static void sle_team_cli_puts(sle_team_cli_t *cli, const char *text)
{
    if (cli != NULL && cli->print != NULL) {
        cli->print(cli->user_ctx, text);
    }
}

static void sle_team_cli_printf(sle_team_cli_t *cli, const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    if (cli == NULL || cli->print == NULL) {
        return;
    }

    va_start(ap, fmt);
    (void)vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    cli->print(cli->user_ctx, buf);
}

static int parse_u32(const char *s, uint32_t min_value, uint32_t max_value, uint32_t *out)
{
    char *end = NULL;
    unsigned long v;

    if (s == NULL || out == NULL) {
        return -1;
    }

    errno = 0;
    v = strtoul(s, &end, 0);
    if (errno != 0 || end == s || *end != '\0' || v > max_value || v < min_value) {
        return -1;
    }
    *out = (uint32_t)v;
    return 0;
}

static int parse_i32(const char *s, int32_t min_value, int32_t max_value, int32_t *out)
{
    char *end = NULL;
    long v;

    if (s == NULL || out == NULL) {
        return -1;
    }

    errno = 0;
    v = strtol(s, &end, 0);
    if (errno != 0 || end == s || *end != '\0' || v < min_value || v > max_value) {
        return -1;
    }
    *out = (int32_t)v;
    return 0;
}

static const char *sle_team_cli_err_name(int ret)
{
    switch (ret) {
        case SLE_TEAM_OK:
            return "OK";
        case SLE_TEAM_ERR_ARG:
            return "ARG";
        case SLE_TEAM_ERR_BUF:
            return "BUF";
        case SLE_TEAM_ERR_FORMAT:
            return "FORMAT";
        case SLE_TEAM_ERR_UNSUPPORTED:
            return "NOT_READY";
        default:
            return "ERR";
    }
}

static void sle_team_cli_print_send_result(sle_team_cli_t *cli, const char *type, uint8_t dst_id, int ret)
{
    if (ret == SLE_TEAM_OK) {
        sle_team_cli_printf(cli, "sle_tx_ok type=%s dst=%u", type, dst_id);
    } else {
        sle_team_cli_printf(cli, "sle_tx_fail type=%s dst=%u ret=%d reason=%s",
            type, dst_id, ret, sle_team_cli_err_name(ret));
    }
}

void sle_team_cli_init(sle_team_cli_t *cli, sle_team_node_t *node, sle_team_cli_print_fn print, void *user_ctx)
{
    if (cli == NULL) {
        return;
    }
    cli->node = node;
    cli->print = print;
    cli->user_ctx = user_ctx;
}

void sle_team_cli_print_help(sle_team_cli_t *cli)
{
    sle_team_cli_puts(cli, "commands:");
    sle_team_cli_puts(cli, "  help");
    sle_team_cli_puts(cli, "  hello [dst]");
    sle_team_cli_puts(cli, "  hb [dst] [battery] [rssi] [fix]");
    sle_team_cli_puts(cli, "  pos [dst] [lat_e6] [lon_e6] [speed] [heading] [battery] [fix] [sat]");
    sle_team_cli_puts(cli, "  alert [dst] [lost_id] [reason] [last_lat] [last_lon] [last_ts]");
    sle_team_cli_puts(cli, "  config [dst]");
    sle_team_cli_puts(cli, "  ack [dst] [ack_seq] [acked_type] [status]");
    sle_team_cli_puts(cli, "  members");
    sle_team_cli_puts(cli, "  state");
}

void sle_team_cli_handle_line(sle_team_cli_t *cli, const char *line)
{
    char local[192];
    char *argv[12];
    int argc = 0;
    char *tok;
    uint32_t v[8];
    int32_t sv[4];
    int i;
    sle_team_pos_body_t pos;
    sle_team_alert_body_t alert;
    const sle_team_member_record_t *member;

    if (cli == NULL || cli->node == NULL || line == NULL) {
        return;
    }

    (void)snprintf(local, sizeof(local), "%s", line);
    tok = strtok(local, " \r\n\t");
    while (tok != NULL && argc < (int)(sizeof(argv) / sizeof(argv[0]))) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \r\n\t");
    }

    if (argc == 0) {
        return;
    }

    if (strcmp(argv[0], "help") == 0) {
        sle_team_cli_print_help(cli);
        return;
    }

    if (strcmp(argv[0], "hello") == 0) {
        uint8_t dst = (argc >= 2) ? (uint8_t)strtoul(argv[1], NULL, 0) : cli->node->cfg.leader_id;
        sle_team_cli_print_send_result(cli, "HELLO", dst, sle_team_node_send_hello(cli->node, dst));
        return;
    }

    if (strcmp(argv[0], "hb") == 0) {
        if (argc < 5) {
            sle_team_cli_puts(cli, "usage: hb [dst] [battery] [rssi] [fix]");
            return;
        }
        if (parse_u32(argv[1], 0U, 255U, &v[0]) != 0 ||
            parse_u32(argv[2], 0U, 100U, &v[1]) != 0 ||
            parse_i32(argv[3], -128, 127, &sv[0]) != 0 ||
            parse_u32(argv[4], 0U, 255U, &v[2]) != 0) {
            sle_team_cli_puts(cli, "bad number");
            return;
        }
        sle_team_cli_print_send_result(cli, "HEARTBEAT", (uint8_t)v[0],
            sle_team_node_send_heartbeat(cli->node, (uint8_t)v[0], (uint8_t)v[1], (int8_t)sv[0], (uint8_t)v[2]));
        return;
    }

    if (strcmp(argv[0], "pos") == 0) {
        if (argc < 9) {
            sle_team_cli_puts(cli, "usage: pos [dst] [lat_e6] [lon_e6] [speed] [heading] [battery] [fix] [sat]");
            return;
        }
        if (parse_u32(argv[1], 0U, 255U, &v[0]) != 0 ||
            parse_i32(argv[2], INT32_MIN, INT32_MAX, &sv[0]) != 0 ||
            parse_i32(argv[3], INT32_MIN, INT32_MAX, &sv[1]) != 0 ||
            parse_u32(argv[4], 0U, UINT16_MAX, &v[1]) != 0 ||
            parse_u32(argv[5], 0U, UINT16_MAX, &v[2]) != 0 ||
            parse_u32(argv[6], 0U, 100U, &v[3]) != 0 ||
            parse_u32(argv[7], 0U, 255U, &v[4]) != 0 ||
            parse_u32(argv[8], 0U, 255U, &v[5]) != 0) {
            sle_team_cli_puts(cli, "bad number");
            return;
        }
        memset(&pos, 0, sizeof(pos));
        pos.latitude_e6 = sv[0];
        pos.longitude_e6 = sv[1];
        pos.speed_cms = (uint16_t)v[1];
        pos.heading_deg = (uint16_t)v[2];
        pos.battery_percent = (uint8_t)v[3];
        pos.fix_status = (uint8_t)v[4];
        pos.sat_count = (uint8_t)v[5];
        sle_team_cli_print_send_result(cli, "POS_REPORT", (uint8_t)v[0],
            sle_team_node_send_position(cli->node, (uint8_t)v[0], &pos));
        return;
    }

    if (strcmp(argv[0], "alert") == 0) {
        if (argc < 7) {
            sle_team_cli_puts(cli, "usage: alert [dst] [lost_id] [reason] [last_lat] [last_lon] [last_ts]");
            return;
        }
        if (parse_u32(argv[1], 0U, 255U, &v[0]) != 0 ||
            parse_u32(argv[2], 0U, 255U, &v[1]) != 0 ||
            parse_u32(argv[3], 0U, 255U, &v[2]) != 0 ||
            parse_i32(argv[4], INT32_MIN, INT32_MAX, &sv[0]) != 0 ||
            parse_i32(argv[5], INT32_MIN, INT32_MAX, &sv[1]) != 0 ||
            parse_u32(argv[6], 0U, UINT32_MAX, &v[3]) != 0) {
            sle_team_cli_puts(cli, "bad number");
            return;
        }
        memset(&alert, 0, sizeof(alert));
        alert.lost_member_id = (uint8_t)v[1];
        alert.reason = (uint8_t)v[2];
        alert.last_latitude_e6 = sv[0];
        alert.last_longitude_e6 = sv[1];
        alert.last_report_s = v[3];
        sle_team_cli_print_send_result(cli, "ALERT", (uint8_t)v[0],
            sle_team_node_send_alert(cli->node, (uint8_t)v[0], &alert));
        return;
    }

    if (strcmp(argv[0], "config") == 0) {
        uint8_t dst = (argc >= 2) ? (uint8_t)strtoul(argv[1], NULL, 0) : SLE_TEAM_BROADCAST_ID;
        sle_team_cli_print_send_result(cli, "CONFIG", dst, sle_team_node_send_config(cli->node, dst));
        return;
    }

    if (strcmp(argv[0], "ack") == 0) {
        if (argc < 5) {
            sle_team_cli_puts(cli, "usage: ack [dst] [ack_seq] [acked_type] [status]");
            return;
        }
        if (parse_u32(argv[1], 0U, 255U, &v[0]) != 0 ||
            parse_u32(argv[2], 0U, UINT16_MAX, &v[1]) != 0 ||
            parse_u32(argv[3], 0U, 255U, &v[2]) != 0 ||
            parse_u32(argv[4], 0U, 255U, &v[3]) != 0) {
            sle_team_cli_puts(cli, "bad number");
            return;
        }
        sle_team_cli_print_send_result(cli, "ACK", (uint8_t)v[0],
            sle_team_node_send_ack(cli->node, (uint8_t)v[0], (uint16_t)v[1], (uint8_t)v[2], (uint8_t)v[3]));
        return;
    }

    if (strcmp(argv[0], "members") == 0) {
        for (i = 1; i <= (int)SLE_TEAM_MAX_MEMBERS; i++) {
            member = sle_team_node_find_member(cli->node, (uint8_t)i);
            if (member != NULL) {
                sle_team_cli_printf(cli, "member=%u role=%u online=%u battery=%u fix=%u rssi=%d last_seq=%u last_seen=%lu",
                    member->member_id, member->role, member->online, member->battery_percent,
                    member->fix_status, member->last_rssi_dbm, member->last_seq, (unsigned long)member->last_seen_s);
            }
        }
        return;
    }

    if (strcmp(argv[0], "state") == 0) {
        sle_team_cli_printf(cli, "team=%u self=%u leader=%u role=%u state=%u joined=%u seq=%u",
            cli->node->cfg.team_id, cli->node->cfg.self_id, cli->node->cfg.leader_id,
            cli->node->cfg.role, cli->node->state, cli->node->joined, cli->node->next_seq);
        return;
    }

    sle_team_cli_puts(cli, "unknown command, type help");
}
