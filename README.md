# sle_mesh

`sle_mesh` 是一个面向 `WS63` 星闪设备的轻量组网与队伍协同协议骨架。

当前版本：

- `v1.2.10`

当前重点不是 WebUI 或 GNSS 业务完整实现，而是先把下面这些基础能力打稳：

- 主从节点共用同一套包帧定义
- 串口终端控制组网和消息发送
- WS63 当前使用同一个统一固件包，开机后在内置 WebUI 或串口 CLI 选择 leader/member 角色
- 支持 `HELLO / ACK / CONFIG / HEARTBEAT / POS_REPORT / ALERT`
- 外层采用 MeshCore 风格包帧，内层使用当前项目自定义业务包

## 目录

- [include/](/Users/bh4me_macair/Documents/Codex/sle_intercom/include)：公共头文件
- [src/](/Users/bh4me_macair/Documents/Codex/sle_intercom/src)：协议、组网、串口 CLI 实现
- [examples/](/Users/bh4me_macair/Documents/Codex/sle_intercom/examples)：本地测试和 HiSpark 接入模板
- [docs/](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs)：设计文档
- [versions/](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions)：版本记录和快照

## 当前协议文档

请优先看这些文档：

- [docs/protocol/README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/README.md)
- [docs/protocol/packet-structure.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/packet-structure.md)
- [docs/protocol/messages.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/messages.md)
- [docs/protocol/terminal-cli.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/terminal-cli.md)
- [docs/protocol/versioning.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/versioning.md)

## WebUI

第一版 Web 控制台在 [webui/](/Users/bh4me_macair/Documents/Codex/sle_intercom/webui)。

它参考 Meshtastic Web 和 MeshCore Web App 的使用形态，但按本项目协议实现：

- 节点列表
- 消息流
- 测试发送
- `Mesh Packet -> GROUP_DATA -> App Packet` 十六进制解析器
- 域名上位机使用 Vite/TypeScript，WS63 板端使用 C 端 SSR
- 两端共享 `webui/shared/console-pages.json` 中的名称、入口、标签和配色
- 串口模式可以配置 leader 成员准入白名单，避免同队号同信道时误收无关 member
- WS63 内置 WebUI 支持 leader 开/关配队窗口、批准 pending member，member 可选择 leader 或退出队伍

详细接入计划见 [docs/webui-plan.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/webui-plan.md)。

## WS63 板端 WiFi 控制台

当前 WS63 统一固件已加入板端 SoftAP + HTTP 控制台：

- SSID：`SLE-TEAM-WS63-XXXX`，其中 `XXXX` 是本机 WiFi MAC 后四位
- 密码：`123456789`
- 板端地址：`http://192.168.43.1/`
- API：`/api/status`、`/api/nodes`、`/api/events`、`/api/pending`、`/api/pairing`、`/api/member/select`、`/api/member/leave`、`/api/factory-reset`

手机调试时要注意：WS63 SoftAP 没有外网，iOS/部分浏览器会自动切回蜂窝数据，右上角如果显示 `5G` 而不是 WiFi 图标，页面请求会出现 `Network request failed`。这不是 WS63 HTTP 服务必然崩溃，而是手机没有稳定走 WS63 的 WiFi 路由。

`v1.2.3` 的当前可用基线是 `ssr=v3`：板端直接渲染 `/`、`/nodes`、`/events` 三个 HTML 页面，并使用带 `Content-Length` 的完整 HTTP 响应，避免 iOS/微信内置浏览器对流式响应一闪而过。页面底部会显示 `page=... ssr=v3`，用于现场确认固件版本。

当前现场已经确认 `status`、`nodes`、`events` 页面均可打开。Nodes/Events 显示空数组 `[]` 表示 leader 当前还没有 member 加入，不代表页面失败。JSON API 仍保留给上位机或调试使用。

统一固件开机默认是 `unconfigured`，此时 SLE 还不启动，只启动 SoftAP、HTTP WebUI 和串口 CLI。先打开 `http://192.168.43.1/pairing`：

- 点 `set leader` 后，本板成为 leader，页面自标识显示 `LXXXX`。
- 填 leader 的 MAC 后四位并点 `set member` 后，本板成为 member，页面自标识显示 `MXXXX`。
- `UXXXX / LXXXX / MXXXX` 都来自同一个固件，`XXXX` 是 MAC 后四位；内部路由仍是 1 字节 ID，由 MAC 低字节派生。

`v1.2.5` 增加 `/pairing` 页面，用于现场无电脑、无互联网时靠手机操作：

- leader 页面：`/pairing` 里 `start` 打开配队窗口，附近 member 周期发送 `HELLO` 时会进入 pending 列表；点 `approve` 后 leader 把该 member 加入 RAM 白名单，并向 member 发送 `CONFIG + ACK`，member 随后进入 joined；`cancel` 只关闭配队窗口，不删除已批准 member。
- member 页面：`/pairing` 里可以 `select leader`，填写 `team / leader / channel` 后立即发送 `HELLO`；也可以 `leave` 退出当前队伍，状态回到 discovering。

`v1.2.6` 起，WebUI 设置的角色、队伍号、leader MAC 后四位和 channel 会写入 WS63 NV flash；断电或复位后会自动恢复。member 点 `leave` 或 WebUI 点 `factory reset` 会清除这份 flash 配置。pending 列表、节点在线状态和 events 日志仍是 RAM 运行时状态，重启后重新发现。

`v1.2.7` 起，板端 HTTP 给每个手机连接设置短收发超时，并把连接等待队列加到 4，避免手机浏览器留下半开连接后卡住下一次访问。页面底部版本标识为 `ssr=v5`。

同版还把 WebUI 角色切换改成异步：点 `set leader` / `set member` 后页面先返回，SLE 初始化由主任务继续执行，避免“开启 SLE 后 Web 页面加载不出来”。member 如果第一次 HELLO 因 SLE client 未 ready 发送失败，会继续周期重试。

`v1.2.8` 起，RSSI 改为 WS63 SLE 真实连接 RSSI：固件调用 `sle_read_remote_device_rssi(conn_id)` 并在 `read_rssi_cb` 中缓存 dBm。未知值显示 `NA`，不会再显示模拟 `-50`。member 如果 SLE client 处于 `NOT_READY` 或写失败，会按 5 秒节流强制重新扫描；已 joined 后如果超时未收到 leader 包，会自动回到 joining 并继续发送 `HELLO`。leader SLE 断开后会重新启动 announce。

当前还不是“扫描附近所有 leader 并选择”的完整发现系统；member 选择 leader 仍需要填写 `team / leader / channel`。下一步真正户外配队要补 leader/member 地址绑定或 `cipher_mac` 认证。

为了保护 WS63 RAM 和兼容手机浏览器，板端页面不直接烧录 Vite 产物，也不运行前端 JS。当前做法是同源配置、不同渲染目标：域名页面从 `webui/shared/console-pages.json` 读取配置，板端通过 `tools/gen_ws63_console_header.mjs` 生成 `xc/ws63_team_network/src/ws63_console_pages.h`，再由 C 代码拼出完整 HTML 响应。

如果刷新页面时偶尔失败，优先看两点：

- 手机是否仍连在 `SLE-TEAM-WS63-XXXX`，而不是切到了蜂窝或其他 WiFi。
- 串口如果只出现 `errno=104`，通常是浏览器刷新时取消旧连接，不等于板端 HTTP 服务崩溃。

早期草案保留在 [docs/protocol.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol.md)，但当前实现以 `docs/protocol/` 和 `include/`、`src/` 为准。

## WS63 烧录确认

当前 leader/member 烧录的是同一个统一固件包。脚本里的 `leader/member` 只用于选择默认串口和二次确认，避免把上、下两个雷电口对应的板子烧错：

```sh
scripts/ws63_flash_team.sh leader /dev/tty.usbserial-10
scripts/ws63_flash_team.sh member /dev/tty.usbserial-110
```

脚本会打印角色、串口和固件路径，并要求输入 `flash leader` 或 `flash member` 才会继续。
macOS 烧录优先使用 `/dev/tty.usbserial-*`。

需要重新出包时，优先用局域网 Ubuntu 编译机：

```sh
UBUNTU_HOST=192.168.6.5 \
UBUNTU_USER=owen \
UBUNTU_PASS='67215837' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 \
scripts/ws63_build_team_ubuntu.sh unified
```

输出统一固件包：

```text
/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_unified_runtime_role/ws63-liteos-app_unified_all.fwpkg
```

VM 构建脚本仍保留，网络 Ubuntu 不可用时再用：

```sh
scripts/ws63_build_team_vm.sh unified
```

这些脚本现在会先同步本仓库最新源码到 Ubuntu SDK，再执行构建，避免“本地改了代码但烧进去还是旧版本”的问题。远程 SDK 里需要有 BearPi 工程，脚本会同步到：

```text
third_party/sle_mesh/
application/samples/products/sle_team_network/
```

WS63 样例现在同时编入 SLE UART server/client 代码，并强制打开 `SUPPORT_SLE_PERIPHERAL` 与 `SUPPORT_SLE_CENTRAL`。角色由开机后的 WebUI/CLI 配置决定，不再维护两份业务代码或两种业务固件。

## 队伍匹配和成员准入

当前组包逻辑遵循 `docs/protocol/` 中的结构：

```text
Mesh Packet -> GROUP_DATA(channel_hash, cipher_mac) -> App Packet(team_id, src_id, dst_id, ...)
```

当前真实参与匹配的字段是：

- `channel_hash`：无线信道/队伍口令的轻量散列。
- `team_id`：队伍号。
- `dst_id`：目标节点，或 `255` 广播。
- `leader_id`：member 本地配置的目标 leader ID。
- `src_id`：发送方节点 ID。

如果现场有两个 leader 的 `team_id`、`leader_id`、`channel_hash` 完全一样，member 在发现阶段仍可能连到先发现的那个 leader。为降低这个风险，当前版本增加了 leader 侧成员准入：

```text
allow
allow all
allow only 2
allow add 3
allow del 3
pairing start
pairing pending
pairing approve 2
pairing stop
join 1 1 17
leave
```

leader 收到非白名单 member 的 `HELLO / HEARTBEAT / POS_REPORT` 会直接拒绝，不会 ACK、不会 CONFIG、不会登记到 nodes。member 侧也会拒绝 `src_id` 不是本地 `leader_id` 的包。

leader 配队窗口打开后，未在白名单内的 member `HELLO` 不会立刻入队，而是进入 pending。leader 执行 `pairing approve <id>` 或在内置 WebUI 点 approve 后，该 member 才会收到 `ACK/CONFIG` 并加入。

`v1.2.9` 起，SLE 连接方向按官方 `sle_uart_1_vs_8` 思路调整：leader 作为 central/client 持续扫描并连接多个 member，member 作为 peripheral/server 广播并等待 leader 连接。leader 收到 member 上行包后会绑定 `member_id -> conn_id`，单播下行优先按 conn 精确发送，广播下行才发给所有连接。

`v1.2.10` 修复双 member 现场测试时“先配置谁就只能看到谁”的问题。根因是官方 server/peripheral 样例给所有 member 使用了同一个固定 SLE 本机地址，leader 会把两块板当成同一个远端设备。现在 member 会用本机 WiFi MAC 派生唯一 SLE 地址；2026-05-04 实测 `C7E9` 和 `E7F1` 两块 member 可同时被 leader 扫描、连接，并分别绑定到 `conn_id:0` 和 `conn_id:1`。

注意：当前 `allow/pairing/join` 配置是运行时 RAM 配置，断电重启会恢复默认构建配置。后续如果要做真正配对，需要把白名单、leader 绑定或 pairing key 写入 WS63 持久化配置，并用 `cipher_mac` 做真实认证。

## 快速编译验证

本地协议包测试：

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  src/sle_team_packet.c examples/team_node_common.c \
  -o /tmp/sle_team_packet_test && /tmp/sle_team_packet_test
```

本地组网流程测试：

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  src/sle_team_packet.c src/sle_team_node.c examples/team_network_demo.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test
```

串口 CLI 模拟测试：

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_TERMINAL_TEST \
  src/sle_team_packet.c src/sle_team_node.c src/sle_team_cli.c examples/app_terminal_node.c \
  -o /tmp/sle_team_terminal_test
```

## HiSpark 接入点

在 HiSpark 工程里，主要接这三个入口：

- 串口收到一整行命令后调用 `sle_team_cli_handle_line(&g_cli, line)`
- SLE 收到二进制包后调用 `sle_team_node_on_packet(&g_node, rx_buf, rx_len)`
- 周期定时器里调用 `sle_team_node_tick(&g_node)`

`examples/app_terminal_node.c` 顶部可用宏定义选择角色：

```c
#define SLE_TEAM_NODE_IS_LEADER 1
```

- `1`：leader
- `0`：member

## 安全状态

当前版本没有真正加密。

当前 `GROUP_DATA` 结构中保留了：

- `channel_hash`
- `cipher_mac`

但 `cipher_mac` 目前只是占位字段，`app_packet` 是明文。后续版本如果要加密，应在 `GROUP_DATA` 层把 `app_packet` 替换为 `ciphertext`，并让 `cipher_mac` 成为真实认证标签。

## License

Apache-2.0
