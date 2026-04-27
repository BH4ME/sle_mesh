# 串口终端命令

串口 CLI 代码：

- [sle_team_cli.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/include/sle_team_cli.h)
- [sle_team_cli.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/src/sle_team_cli.c)
- [app_terminal_node.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/examples/app_terminal_node.c)

## 角色选择

在 `app_terminal_node.c` 顶部使用宏定义：

```c
#define SLE_TEAM_NODE_IS_LEADER 1
```

取值：

- `1`：leader
- `0`：member

常用宏：

- `SLE_TEAM_SELF_ID`
- `SLE_TEAM_LEADER_ID`
- `SLE_TEAM_TEAM_ID`
- `SLE_TEAM_CHANNEL_HASH`

## 命令列表

### help

打印命令帮助。

```text
help
```

### state

打印当前节点状态。

```text
state
```

### members

打印 leader 当前记录的成员状态。

```text
members
```

### hello

发送入网问候。

```text
hello [dst]
```

示例：

```text
hello 1
```

### hb

发送心跳。

```text
hb [dst] [battery] [rssi] [fix]
```

示例：

```text
hb 1 88 -50 1
```

### pos

发送位置。

```text
pos [dst] [lat_e6] [lon_e6] [speed] [heading] [battery] [fix] [sat]
```

示例：

```text
pos 1 39908456 116397128 120 90 88 1 9
```

### alert

发送告警。

```text
alert [dst] [lost_id] [reason] [last_lat] [last_lon] [last_ts]
```

示例：

```text
alert 255 2 2 39908456 116397128 12345
```

### config

发送配置。

```text
config [dst]
```

### ack

发送确认。

```text
ack [dst] [ack_seq] [acked_type] [status]
```

## HiSpark 接入

串口收到一整行后调用：

```c
sle_team_cli_handle_line(&g_cli, line);
```

SLE 收包后调用：

```c
sle_team_node_on_packet(&g_node, rx_buf, rx_len);
```

周期定时器调用：

```c
sle_team_node_tick(&g_node);
```
