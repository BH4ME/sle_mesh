# sle_mesh

`sle_mesh` 是一个面向 `WS63` 星闪设备的轻量组网与队伍协同协议骨架。

当前版本：

- `v1.2.3`

当前重点不是 WebUI 或 GNSS 业务完整实现，而是先把下面这些基础能力打稳：

- 主从节点共用同一套包帧定义
- 串口终端控制组网和消息发送
- 通过宏定义选择 `leader/member` 角色
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
- 同一份 `dist` 可放进 WS63 板端 HTTP，也可部署到域名作为上位机

详细接入计划见 [docs/webui-plan.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/webui-plan.md)。

## WS63 板端 WiFi 控制台

当前 `v1.2.3` 已在 leader 固件里加入板端 SoftAP + HTTP 控制台：

- SSID：`SLE-TEAM-WS63-L1`
- 密码：`123456789`
- 板端地址：`http://192.168.43.1/`
- API：`/api/status`、`/api/nodes`、`/api/events`

手机调试时要注意：WS63 SoftAP 没有外网，iOS/部分浏览器会自动切回蜂窝数据，右上角如果显示 `5G` 而不是 WiFi 图标，页面请求会出现 `Network request failed`。这不是 WS63 HTTP 服务必然崩溃，而是手机没有稳定走 WS63 的 WiFi 路由。

`v1.2.3` 的当前可用基线是 `ssr=v3`：板端直接渲染 `/`、`/nodes`、`/events` 三个 HTML 页面，并使用带 `Content-Length` 的完整 HTTP 响应，避免 iOS/微信内置浏览器对流式响应一闪而过。页面底部会显示 `page=... ssr=v3`，用于现场确认固件版本。

当前现场已经确认 `status`、`nodes`、`events` 页面均可打开。Nodes/Events 显示空数组 `[]` 表示 leader 当前还没有 member 加入，不代表页面失败。JSON API 仍保留给上位机或调试使用。

如果刷新页面时偶尔失败，优先看两点：

- 手机是否仍连在 `SLE-TEAM-WS63-L1`，而不是切到了蜂窝或其他 WiFi。
- 串口如果只出现 `errno=104`，通常是浏览器刷新时取消旧连接，不等于板端 HTTP 服务崩溃。

早期草案保留在 [docs/protocol.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol.md)，但当前实现以 `docs/protocol/` 和 `include/`、`src/` 为准。

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
