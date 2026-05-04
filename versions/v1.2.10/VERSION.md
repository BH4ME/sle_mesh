# Version v1.2.10

当前版本：

- `v1.2.10`

说明：

- 统一固件继续由内置 WebUI/CLI 在运行时选择 leader/member。
- LED 行为细化为三类：
  - 空闲灭灯。
  - leader pairing window open 时，每秒极短闪一下，表示正在开放发现/配队。
  - leader pairing window open 或尚未连上 member 时，每秒极短闪一下，表示正在扫描 member。
  - `[sle-tx-ok]` 业务包发送成功时快闪两下。
  - `[sle-rx]` 真实收到业务包时慢闪一下。
  - `[sle-tx-fail]` / `NOT_READY` 不闪，表示没有真正发出去。
- SLE 连接方向改为官方 `sle_uart_1_vs_8` 模型：leader 是 client/central/seeker，member 是 server/peripheral/advertiser。
- leader 端 SLE UART client 从单连接变量改为最多 8 个连接的连接表。
- leader pairing window open 或未连满时持续扫描，让第二个 member 有机会同时发现/连接。
- leader 收到 member 上行包时记录 `member_id -> conn_id` 映射；向具体 member 下发 `ACK / CONFIG` 时优先按 conn 精确发送，找不到映射时才 fallback 到全连接发送。广播 HEARTBEAT 仍发到所有已连接链路。
- 修复现场确认到的 `conn_id=0` 问题：SDK 首个连接可以是 0，不能把 0 当作“未找到连接”。
- 修复已 joined member 在 leader 打开 pairing window 后被重新放回 pending 的状态机问题。
- 修复 leader approve 后正式 member 记录没有继承 pending MAC 的问题，避免 Nodes 中显示 `NF1` 这类 fallback 名。
- 修复官方 server/peripheral 样例固定 SLE 本机地址导致的双 member 冲突：member 现在用本机 WiFi MAC 派生唯一 SLE 地址。
- HTTP 响应增加 no-cache 头，并扩大响应头缓冲区，减少手机浏览器缓存/重载造成的页面异常。

2026-05-04 现场测试问题记录：

- leader：`/dev/tty.usbserial-10`，`self=154`，标签约为 `L279A`。
- member：`/dev/tty.usbserial-110`，`self=241`，标签约为 `ME7F1`。
- 关键日志：
- 首个连接实测为 `conn_id:0`：
  - `connect state changed callback conn_id:0x00`
  - `bind member:241 conn_id:0`
- member 初次入队链路正常：
  - `HELLO 241->154`
  - `ACK 154->241`
  - `CONFIG 154->241`
  - `HEARTBEAT 241->154`
- 打开 leader pairing window 后，旧逻辑出现状态回退：
  - `member heartbeat timeout`
  - `leader timeout, rejoining`
  - `HELLO 241->154`
  - `member pending approval`
- 后续双 member 复测发现：先配置谁就只能看到谁，根因是官方样例把所有 member SLE 广播地址固定为 `01:02:03:04:05:06`。
- 修复后 member 启动日志会打印唯一 SLE 地址，例如 `sle local addr=52:5C:11:7D:E7:F1`。
- 修复后 leader 同时收到两路 HELLO：
  - `bind member:233 conn_id:0`
  - `HELLO 233->154`
  - `bind member:241 conn_id:1`
  - `HELLO 241->154`
- approve 后两块 member 都进入心跳：
  - `HEARTBEAT 233->154`
  - `HEARTBEAT 241->154`

配队语义：

- pairing window open 期间，未 approve 的 member 可以建立 SLE 链路并发送 `HELLO`。
- leader 对未 approve member 不应发送 joined 语义的 `ACK/CONFIG`，只应进入 pending。
- 当前协议还没有独立的 `WAIT_APPROVAL` 包；后续若要让 member WebUI 显示“leader 已看到，等待批准”，应增加轻量 pending 状态回复。
- approve 后 leader 才发送 `CONFIG + ACK`，member 才进入 joined，之后才开始正常 HEARTBEAT/业务包。

已知限制：

- 双 member 同时 pending/approved 已通过现场串口验证。
- 目前已经有 `member_id -> conn_id` 绑定和按 member 精确下发；per-member RSSI 还没有暴露到 WebUI，leader 状态页仍显示 server 侧最近/首个连接 RSSI。
- 若 SDK 实测不支持一个 server 同时保持多个 client 连接，就需要退到轮询接入模型：member 分时连接、上报、断开，leader WebUI 维护白名单和最近上报状态。

解决方向：

- 下一步做 per-member RSSI 展示和更清楚的 WebUI 连接状态。
- 后续如果要扩到 20 个 member，再评估 SDK 实际连接上限；当前仍按 8 个连接表实现。
