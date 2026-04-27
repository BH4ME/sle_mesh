# 组网骨架说明

当前新增的组网骨架文件：

- [sle_team_node.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/include/sle_team_node.h)
- [sle_team_node.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/src/sle_team_node.c)
- [team_network_demo.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/examples/team_network_demo.c)

## 目标

这套代码先解决下面几件事：

1. `leader/member` 共用一套节点上下文
2. 先把 `HELLO -> ACK -> CONFIG` 入网流程跑通
3. 再叠加 `HEARTBEAT / POS_REPORT / ALERT`
4. 不绑死具体 HiSpark SDK 接口

## 当前入网流程

### member

- 初始化后进入 `DISCOVERING`
- 每 3 秒尝试向 `leader_id` 发一次 `HELLO`
- 收到 `ACK(HELLO)` 后转为 `ONLINE`

### leader

- 初始化即 `ONLINE`
- 收到 `HELLO` 后：
  - 记录成员
  - 回 `ACK`
  - 回 `CONFIG`

## 你后面在 HiSpark 工程里要填的底层接口

主要是 `sle_team_node_ops_t` 里的三个回调：

- `send`
- `now_s`
- `log`

### send

把这里接到你真实的 `SLE notify/write/send`。

### now_s

返回当前秒计时。

### log

接串口打印即可。

## 为什么这样写

因为你现在还没把具体 `SLE` 工程目录贴出来，我先把骨架写成“平台无关层”：

- 协议层只负责组包/解包
- 节点层只负责状态机
- 底层收发由你在 HiSpark 工程里绑定

这样后面迁到 `leader.c/member.c` 会最省事。
