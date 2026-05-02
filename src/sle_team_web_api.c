#include "sle_team_web_api.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char *buf;
    size_t len;
    size_t used;
    int truncated;
} sle_team_json_writer_t;

static void json_append(sle_team_json_writer_t *writer, const char *fmt, ...)
{
    va_list ap;
    int written;

    if (writer == NULL || writer->buf == NULL || writer->len == 0U || writer->truncated != 0) {
        return;
    }
    if (writer->used >= writer->len) {
        writer->truncated = 1;
        return;
    }

    va_start(ap, fmt);
    written = vsnprintf(&writer->buf[writer->used], writer->len - writer->used, fmt, ap);
    va_end(ap);
    if (written < 0 || (size_t)written >= writer->len - writer->used) {
        writer->used = writer->len - 1U;
        writer->buf[writer->used] = '\0';
        writer->truncated = 1;
        return;
    }
    writer->used += (size_t)written;
}

static void json_append_escaped(sle_team_json_writer_t *writer, const char *text)
{
    const unsigned char *p;

    json_append(writer, "\"");
    if (text != NULL) {
        for (p = (const unsigned char *)text; *p != '\0'; p++) {
            switch (*p) {
                case '\"':
                    json_append(writer, "\\\"");
                    break;
                case '\\':
                    json_append(writer, "\\\\");
                    break;
                case '\n':
                    json_append(writer, "\\n");
                    break;
                case '\r':
                    json_append(writer, "\\r");
                    break;
                case '\t':
                    json_append(writer, "\\t");
                    break;
                default:
                    if (*p < 0x20U) {
                        json_append(writer, "\\u%04x", *p);
                    } else {
                        json_append(writer, "%c", *p);
                    }
                    break;
            }
        }
    }
    json_append(writer, "\"");
}

const char *sle_team_web_role_name(uint8_t role)
{
    return role == (uint8_t)SLE_TEAM_ROLE_LEADER ? "leader" : "member";
}

const char *sle_team_web_state_name(uint8_t state)
{
    switch (state) {
        case SLE_TEAM_NET_DISCOVERING:
            return "discovering";
        case SLE_TEAM_NET_JOINING:
            return "joining";
        case SLE_TEAM_NET_ONLINE:
            return "online";
        case SLE_TEAM_NET_IDLE:
        default:
            return "idle";
    }
}

const char *sle_team_web_msg_type_name(uint8_t app_msg_type)
{
    switch (app_msg_type) {
        case SLE_TEAM_APP_HELLO:
            return "HELLO";
        case SLE_TEAM_APP_HEARTBEAT:
            return "HEARTBEAT";
        case SLE_TEAM_APP_POS_REPORT:
            return "POS_REPORT";
        case SLE_TEAM_APP_ALERT:
            return "ALERT";
        case SLE_TEAM_APP_CONFIG:
            return "CONFIG";
        case SLE_TEAM_APP_ACK:
            return "ACK";
        default:
            return "UNKNOWN";
    }
}

void sle_team_web_event_log_init(sle_team_web_event_log_t *log)
{
    if (log == NULL) {
        return;
    }
    (void)memset(log, 0, sizeof(*log));
    log->next_id = 1U;
}

void sle_team_web_event_push(sle_team_web_event_log_t *log, uint32_t time_s,
    sle_team_web_event_direction_t direction, uint8_t app_msg_type, uint8_t src_id, uint8_t dst_id, uint16_t seq,
    const char *summary)
{
    sle_team_web_event_t *event;

    if (log == NULL) {
        return;
    }
    event = &log->events[log->head];
    (void)memset(event, 0, sizeof(*event));
    event->id = log->next_id++;
    event->time_s = time_s;
    event->direction = direction;
    event->app_msg_type = app_msg_type;
    event->src_id = src_id;
    event->dst_id = dst_id;
    event->seq = seq;
    if (summary != NULL) {
        (void)snprintf(event->summary, sizeof(event->summary), "%s", summary);
    }
    log->head = (uint8_t)((log->head + 1U) % SLE_TEAM_WEB_EVENT_COUNT);
    if (log->count < SLE_TEAM_WEB_EVENT_COUNT) {
        log->count++;
    }
}

int sle_team_web_write_status_json(const sle_team_node_t *node, uint32_t uptime_s, const char *transport,
    char *out, size_t out_len)
{
    sle_team_json_writer_t writer;

    if (node == NULL || out == NULL || out_len == 0U) {
        return SLE_TEAM_ERR_ARG;
    }
    writer.buf = out;
    writer.len = out_len;
    writer.used = 0U;
    writer.truncated = 0;
    out[0] = '\0';

    json_append(&writer,
        "{\"teamId\":%u,\"selfId\":%u,\"leaderId\":%u,\"role\":\"%s\",\"state\":\"%s\","
        "\"joined\":%s,\"nextSeq\":%u,\"uptimeS\":%lu,\"transport\":\"%s\"}",
        node->cfg.team_id, node->cfg.self_id, node->cfg.leader_id, sle_team_web_role_name((uint8_t)node->cfg.role),
        sle_team_web_state_name((uint8_t)node->state), node->joined != 0U ? "true" : "false", node->next_seq,
        (unsigned long)uptime_s, transport != NULL ? transport : "ws63-http");
    return writer.truncated != 0 ? SLE_TEAM_ERR_BUF : (int)writer.used;
}

int sle_team_web_write_nodes_json(const sle_team_node_t *node, char *out, size_t out_len)
{
    sle_team_json_writer_t writer;
    uint8_t i;
    uint8_t wrote = 0U;

    if (node == NULL || out == NULL || out_len == 0U) {
        return SLE_TEAM_ERR_ARG;
    }
    writer.buf = out;
    writer.len = out_len;
    writer.used = 0U;
    writer.truncated = 0;
    out[0] = '\0';

    json_append(&writer, "[");
    for (i = 0U; i < SLE_TEAM_MAX_MEMBERS; i++) {
        const sle_team_member_record_t *member = &node->members[i];
        if (member->online == 0U) {
            continue;
        }
        if (wrote != 0U) {
            json_append(&writer, ",");
        }
        json_append(&writer,
            "{\"id\":%u,\"role\":\"%s\",\"online\":%s,\"batteryPercent\":%u,\"fixStatus\":%u,"
            "\"lastRssiDbm\":%d,\"lastSeq\":%u,\"lastSeenS\":%lu}",
            member->member_id, sle_team_web_role_name(member->role), member->online != 0U ? "true" : "false",
            member->battery_percent, member->fix_status, member->last_rssi_dbm, member->last_seq,
            (unsigned long)member->last_seen_s);
        wrote = 1U;
    }
    json_append(&writer, "]");
    return writer.truncated != 0 ? SLE_TEAM_ERR_BUF : (int)writer.used;
}

int sle_team_web_write_events_json(const sle_team_web_event_log_t *log, char *out, size_t out_len)
{
    static const char *directions[] = {"rx", "tx", "system"};
    sle_team_json_writer_t writer;
    uint8_t i;

    if (log == NULL || out == NULL || out_len == 0U) {
        return SLE_TEAM_ERR_ARG;
    }
    writer.buf = out;
    writer.len = out_len;
    writer.used = 0U;
    writer.truncated = 0;
    out[0] = '\0';

    json_append(&writer, "[");
    for (i = 0U; i < log->count; i++) {
        uint8_t index = (uint8_t)((log->head + SLE_TEAM_WEB_EVENT_COUNT - 1U - i) % SLE_TEAM_WEB_EVENT_COUNT);
        const sle_team_web_event_t *event = &log->events[index];
        const char *direction = "system";
        if ((uint8_t)event->direction < (uint8_t)(sizeof(directions) / sizeof(directions[0]))) {
            direction = directions[event->direction];
        }
        if (i != 0U) {
            json_append(&writer, ",");
        }
        json_append(&writer,
            "{\"id\":\"evt-%lu\",\"time\":\"%lu\",\"direction\":\"%s\",\"type\":\"%s\","
            "\"srcId\":%u,\"dstId\":%u,\"seq\":%u,\"summary\":",
            (unsigned long)event->id, (unsigned long)event->time_s, direction,
            sle_team_web_msg_type_name(event->app_msg_type), event->src_id, event->dst_id, event->seq);
        json_append_escaped(&writer, event->summary);
        json_append(&writer, "}");
    }
    json_append(&writer, "]");
    return writer.truncated != 0 ? SLE_TEAM_ERR_BUF : (int)writer.used;
}
