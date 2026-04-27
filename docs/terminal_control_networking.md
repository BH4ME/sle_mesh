# 串口终端控制组网

当前新增文件：

- [sle_team_cli.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/include/sle_team_cli.h)
- [sle_team_cli.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/src/sle_team_cli.c)
- [app_terminal_node.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/examples/app_terminal_node.c)

## 目标

把当前组网骨架改成下面这种使用方式：

- 主从角色通过文件开头宏定义切换
- 通过串口终端输入命令
- 串口打印当前状态和发包内容

## 宏定义切换

在 [app_terminal_node.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/examples/app_terminal_node.c) 顶部：

```c
#define SLE_TEAM_NODE_IS_LEADER 1
```

可改为：

- `1`：leader
- `0`：member

其他相关宏：

- `SLE_TEAM_SELF_ID`
- `SLE_TEAM_LEADER_ID`
- `SLE_TEAM_TEAM_ID`
- `SLE_TEAM_CHANNEL_HASH`

## 当前支持的串口命令

- `help`
- `hello [dst]`
- `hb [dst] [battery] [rssi] [fix]`
- `pos [dst] [lat_e6] [lon_e6] [speed] [heading] [battery] [fix] [sat]`
- `alert [dst] [lost_id] [reason] [last_lat] [last_lon] [last_ts]`
- `config [dst]`
- `ack [dst] [ack_seq] [acked_type] [status]`
- `members`
- `state`

在本地测试版本里还支持：

- `tick [now_s]`

它用于模拟时间推进，触发 `tick` 状态机。

## 在 HiSpark 工程中的接法

### 串口接 CLI

串口收到一整行文本后：

```c
sle_team_cli_handle_line(&g_cli, line);
```

### SLE 收包接节点层

收到 SLE 原始二进制包后：

```c
sle_team_node_on_packet(&g_node, rx_buf, rx_len);
```

### 定时器接 tick

周期调用：

```c
sle_team_node_tick(&g_node);
```

## 当前输出方式

发送时：

- 串口打印 `tx` 十六进制数据

事件时：

- 打印 `joined`
- 打印 `pos`
- 打印 `alert`

状态查询时：

- 打印 `state`
- 打印 `members`
