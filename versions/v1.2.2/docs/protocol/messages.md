# 业务消息

业务消息由 `App Packet` 的 `msg_type` 决定。

当前定义在：

- [sle_team_packet.h](<repo-root>/include/sle_team_packet.h)

## 消息类型

| 类型 | 值 | 作用 |
| --- | --- | --- |
| `HELLO` | `0x01` | 入网问候 |
| `HEARTBEAT` | `0x02` | 心跳保活 |
| `POS_REPORT` | `0x03` | 位置上报 |
| `ALERT` | `0x04` | 告警 |
| `CONFIG` | `0x05` | 参数配置 |
| `ACK` | `0x06` | 消息确认 |

## HELLO

结构体：

```c
typedef struct {
    uint8_t device_id;
    uint8_t role;
    uint8_t battery_percent;
    uint8_t reserved;
} sle_team_hello_body_t;
```

含义：

- `device_id`：节点 ID
- `role`：`0` 为 member，`1` 为 leader
- `battery_percent`：电量百分比
- `reserved`：保留

## HEARTBEAT

结构体：

```c
typedef struct {
    uint8_t battery_percent;
    int8_t rssi_dbm;
    uint8_t fix_status;
    uint8_t reserved;
} sle_team_heartbeat_body_t;
```

含义：

- `battery_percent`：电量
- `rssi_dbm`：信号强度
- `fix_status`：定位状态
- `reserved`：保留

## POS_REPORT

结构体：

```c
typedef struct {
    int32_t latitude_e6;
    int32_t longitude_e6;
    uint16_t speed_cms;
    uint16_t heading_deg;
    uint8_t battery_percent;
    uint8_t fix_status;
    uint8_t sat_count;
    uint8_t reserved;
} sle_team_pos_body_t;
```

含义：

- `latitude_e6`：纬度乘 `1e6`
- `longitude_e6`：经度乘 `1e6`
- `speed_cms`：速度，单位厘米每秒
- `heading_deg`：朝向角
- `battery_percent`：电量
- `fix_status`：定位状态
- `sat_count`：卫星数量
- `reserved`：保留

## ALERT

结构体：

```c
typedef struct {
    uint8_t lost_member_id;
    uint8_t reason;
    uint16_t reserved;
    int32_t last_latitude_e6;
    int32_t last_longitude_e6;
    uint32_t last_report_s;
} sle_team_alert_body_t;
```

含义：

- `lost_member_id`：离队或失联成员 ID
- `reason`：原因
- `last_latitude_e6`：最后纬度
- `last_longitude_e6`：最后经度
- `last_report_s`：最后上报时间

当前原因值：

| 原因 | 值 |
| --- | --- |
| 距离超限 | `1` |
| 心跳超时 | `2` |
| 低电掉线 | `3` |

## CONFIG

结构体：

```c
typedef struct {
    uint16_t report_interval_s;
    uint16_t warn_distance_m;
    uint16_t lost_distance_m;
    uint16_t heartbeat_timeout_s;
} sle_team_config_body_t;
```

含义：

- `report_interval_s`：位置上报周期
- `warn_distance_m`：预警距离
- `lost_distance_m`：严重离队距离
- `heartbeat_timeout_s`：心跳超时时间

## ACK

结构体：

```c
typedef struct {
    uint16_t ack_seq;
    uint8_t acked_msg_type;
    uint8_t status_code;
} sle_team_ack_body_t;
```

含义：

- `ack_seq`：确认的消息序号
- `acked_msg_type`：确认的消息类型
- `status_code`：确认结果，`0` 表示成功

## 当前序列化边界

当前版本直接把 body 结构体复制进包体。

因此当前假设：

- WS63 与本地测试环境按小端使用
- 当前结构体布局不跨平台混用
- 后续如要跨端严格互通，应把 body 字段改为显式逐字段小端编码
