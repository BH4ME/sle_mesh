# 2026-05-04 WS63 现场更新和踩坑记录

这份文档记录 2026-05-04 最近一轮 WS63 SLE Team 联调结果，重点覆盖双 member 同时入队、leader WiFi/SLE 共存、WebUI 字段含义和后续排查方法。

## 当前重要结论

- `v1.2.10` 是一个关键成功基线：两块统一固件 member 已现场确认可同时进入同一个 leader 的 SLE 队伍。
- member：
  - `MC7E9`，内部路由 ID `233`。
  - `ME7F1`，内部路由 ID `241`。
- leader：
  - `L279A`，内部路由 ID `154`。
- leader 同时收到 `HELLO 233->154` 与 `HELLO 241->154`。
- leader 同时维护 `member 233 -> conn_id:0` 和 `member 241 -> conn_id:1`。
- approve 后两块 member 都进入 `HEARTBEAT`，说明不是 WebUI 假显示，而是真实 SLE 业务包双路跑通。

## 最近更新

### v1.2.8

- RSSI 从模拟值改为 WS63 SLE 真实连接 RSSI。
- 使用 `sle_read_remote_device_rssi(conn_id)` 获取 dBm。
- 未知 RSSI 显示 `NA`，不再显示固定模拟值。
- member 掉线或 `NOT_READY` 后增加重扫/重连机制。

### v1.2.9

- SLE 连接方向改为更接近官方 `sle_uart_1_vs_8` 的模型：
  - leader = client / central / seeker
  - member = server / peripheral / advertiser
- leader 侧从单连接变量改为最多 8 个连接的连接表。
- leader 收到 member 上行包后绑定 `member_id -> conn_id`。
- 修复 `conn_id=0` 被误判为“未找到连接”的问题。
- 修复 leader 打开 pairing window 后，已 joined member 被重新打回 pending 的状态机问题。
- 修复 leader approve 后正式 member 记录没有继承 pending MAC，导致 Nodes 显示 `NF1` 这类 fallback 名的问题。

### v1.2.10

- 修复双 member 只能看到一个的核心问题。
- 根因：官方 server/peripheral 样例把所有 member 的 SLE 本机地址固定成 `01:02:03:04:05:06`。
- 修复：member 启动 SLE server 前，用本机 WiFi MAC 派生唯一 SLE 地址。
- 现场验证：
  - `C7E9` 和 `E7F1` 可同时被 leader 扫描。
  - leader 可同时连接两块 member。
  - 两块 member 未 approve 前进入 pending。
  - approve 后两块 member 都进入 `HEARTBEAT`。

### WiFi/SLE 共存调参

这个改动是在 `v1.2.10` 后继续做的现场调参，目标是缓解 leader 发 SLE 或 pairing 扫描时，手机连接 leader WiFi 后页面需要刷新甚至像 WiFi 断开的现象。

- SLE seek 原参数：
  - `seek_interval=100`
  - `seek_window=100`
  - 近似满占空比扫描，容易挤占 WiFi SoftAP/HTTP。
- 调整后：
  - `seek_interval=400`
  - `seek_window=80`
  - 扫描占空比约 20%。
- leader 强制 rescan 间隔：
  - `5s -> 12s`
- HTTP 收发超时：
  - `1200ms -> 3000ms`
- 该版本已经 Ubuntu 编译通过，并已烧录 leader 做现场验证。

## 踩坑记录

### 坑 1：两个 member 先配置谁就只能看到谁

现象：

- 两块 member 都烧了统一固件。
- leader pairing window 打开后，只能看到一块 member。
- 先配置 `C7E9` 就看到 `C7E9`，先配置 `E7F1` 就看到 `E7F1`。

根因：

- 官方 SLE server/peripheral 样例里，广播本机地址是固定值 `01:02:03:04:05:06`。
- 两块 member 使用同一个 SLE 地址时，leader 会在底层把它们当成同一个远端设备。

修复：

- 新增 `sle_uart_server_adv_set_local_addr()`。
- member 启动 announce 前，把 WiFi MAC 派生出的唯一地址写入 SLE announce 参数。
- 派生地址示例：`sle local addr=52:5C:11:7D:E7:F1`。

验证日志：

```text
bind member:233 conn_id:0
HELLO 233->154
bind member:241 conn_id:1
HELLO 241->154
```

### 坑 2：`conn_id=0` 不能当作无效连接

现象：

- 首个 SLE 连接经常是 `conn_id:0`。
- 旧逻辑如果把 `0` 当作“未找到连接”，leader 精确下发会失败或退回广播。

根因：

- SDK 首个有效连接 ID 可以是 `0`。

修复：

- 查询连接时不要用 `0` 表示失败。
- 改成函数返回 `found=true/false`，`conn_id` 单独输出。

### 坑 3：leader 打开 pairing window 后，老 member 被打回 pending

现象：

- 已 joined 的 member，在 leader 再次打开 pairing window 后可能重新显示成 pending。
- 串口出现 `member heartbeat timeout`、`leader timeout, rejoining`、`member pending approval`。

根因：

- leader 打开 pairing window 后启用 member filter。
- 已经 joined 的 member 如果不在当前 allowlist 判断里，`HELLO` 会被当成未批准设备。

修复：

- 已经进入正式 `members[]` 的 member，即使 pairing window 打开，也视为已批准成员。
- approve pending member 时，把 pending 里的 MAC/role/battery/last_seen 迁移到正式 member 记录。

### 坑 4：leader WiFi 页面在 SLE 扫描/发送时卡住或要刷新

现象：

- 手机连接 leader 的 SoftAP。
- 打开 WebUI 后，leader pairing 或 SLE 发包时页面有时卡住。
- 表现像 WiFi 断开，或者必须刷新页面才恢复。

判断：

- 如果手机 WiFi 图标消失，是 SoftAP/射频共存层可能被挤。
- 如果 WiFi 图标还在但页面转圈，是 HTTP 请求被 SLE 扫描/发送挤到超时。

可疑根因：

- leader 原 SLE seek 参数 `100/100` 接近满占空比扫描。
- WS63 同时跑 SoftAP + SLE central seek 时，WiFi 和 SLE 共享射频/调度资源。

当前缓解：

- seek 改为 `400/80`。
- leader rescan 从 `5s` 拉到 `12s`。
- HTTP timeout 从 `1200ms` 拉到 `3000ms`。

后续观察：

- 打开 leader pairing 后，手机 WiFi 图标是否仍稳定。
- 切换 `status/nodes/events/pairing` 是否仍要频繁刷新。
- 串口是否出现大量 `http recv failed errno=11/104/107`。

### 坑 5：WebUI Events 里的数字容易误解

截图中的例子：

```text
746 HEARTBEAT  MC7E9-L279A #289
743 HEARTBEAT  ME7F1-L279A #288
743 HEARTBEAT  L279A-ALL #9
```

含义：

- 左侧 `746` 是事件发生时的运行时间，单位秒。
- `HEARTBEAT` 是业务包类型。
- `MC7E9-L279A` 是发送方到接收方。
- `#289` 是协议包里的 `seq`，也就是发送方自己的包序号。
- Nodes 页面里的 `Seq 290` 是该节点最近一次被 leader 记录到的 `last_seq`。

注意：

- `746` 和 `#289` 不是同一种计数。
- `746` 是时间。
- `#289` 是包序号。
- 不同发送方的 `seq` 独立计数，所以 `MC7E9 #289` 和 `L279A #9` 不需要一致。

## 当前 WebUI 页面含义

### Status

- 看本板角色、状态、self、leader、joined、nextSeq、uptime、SSID。
- `nextSeq` 是本板下一次发包会使用的序号。

### Nodes

- 展示 leader 当前认为在线的正式成员。
- `Node` 是成员标签，例如 `ME7F1`。
- `Battery` 是最近业务包里携带的电量。
- `RSSI` 是最近读取到的 SLE 连接 RSSI。
- `Seq` 是这个成员最近一条上行包的序号。

### Events

- 展示最近几条 TX/RX 业务包事件。
- 左侧数字是运行秒数。
- 右侧 `#数字` 是包序号。

### Pairing

- leader 页面：
  - start/open：打开配队窗口。
  - pending：未 approve 的 member 会进入这里。
  - approve：批准 member，随后下发 `CONFIG + ACK`。
- member 页面：
  - select leader：选择 leader 后开始发 `HELLO`。
  - leave：退出队伍，清除本地 Web 配置。

## 现场验证命令

### 本地协议测试

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test && /tmp/sle_team_packet_test
```

### Ubuntu 编译

```sh
UBUNTU_HOST=192.168.6.5 \
UBUNTU_USER=owen \
UBUNTU_PASS='67215837' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 \
BUILD_JOBS=4 \
scripts/ws63_build_team_ubuntu.sh unified
```

输出：

```text
/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_unified_runtime_role/ws63-liteos-app_unified_all.fwpkg
```

### 本机烧 leader

```sh
printf 'flash leader\n' | scripts/ws63_flash_team.sh leader /dev/tty.usbserial-10
```

### Windows 烧 member

Windows 当前 member 端口曾出现：

- `COM3`
- `COM5`

烧录命令示例：

```sh
py -3.11 -m xf_burn_tools.burn_tools -p COM3 -b 115200 K:\codex\ws63-liteos-app_unified_all.fwpkg
py -3.11 -m xf_burn_tools.burn_tools -p COM5 -b 115200 K:\codex\ws63-liteos-app_unified_all.fwpkg
```

## 后续优先事项

- 继续观察 WiFi/SLE 共存调参后，leader WebUI 是否还会因 SLE 发包而需要刷新。
- 把 per-member RSSI 更准确地暴露到 WebUI。
- 改善 Events 页面排版，把 `time_s`、`type`、`src->dst`、`seq` 分开显示，避免用户误读。
- 增加“等待 approve”状态回复，让 member WebUI 能显示 leader 已看到自己但尚未批准。
- 评估 8 个以上 member 的策略；如果 SDK 实测连接上限不够，再考虑轮询上报模型。

## GitHub 分支整理建议

当前有效主线：

- `codex/webui-board-console-shared`

已经并入当前主线、可作为历史版本记录而不必长期保留的旧实验分支：

- `codex/webui-sleweb-domain`
- `codex/ws63-blinky`
- `codex/ws63-console-ssr-v3-stable`
- `codex/ws63-status-ok-baseline`
- `codex/ws63-webui-snapshot`
- `codex/ws63-wifi-sle-coexist`

整理原则：

- GitHub 上保留 `main` 和当前主线分支。
- 阶段性成功点写进 `versions/` 和 `docs/field-notes-2026-05-04.md`。
- 不再依赖一堆旧 branch 记忆现场状态。
