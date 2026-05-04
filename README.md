# sle_mesh

`sle_mesh` 是面向 WS63 星闪设备的轻量组网和队伍协同协议工程。当前主线目标是把统一固件、SLE 多成员连接、板端 WiFi 控制台和协议状态机跑稳。

当前版本：`v1.2.10`

## 当前状态

- 统一固件：所有 WS63 烧同一个 `.fwpkg`，开机后通过板端 WebUI 或串口选择 `leader/member`。
- 板端 WebUI：手机连接 `SLE-TEAM-WS63-XXXX`，打开 `http://192.168.43.1/`。
- 当前拓扑：星型 SLE 网络，leader 主动扫描/连接 member。
- 已验证：两块 member `MC7E9`、`ME7F1` 可同时进入 leader `L279A` 队伍，并进入 `HEARTBEAT`。
- 最近调参：降低 leader SLE 扫描占空比，缓解 SLE 扫描/发送时 WiFi 页面卡住的问题。

## 快速入口

- [现场更新和踩坑记录](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/field-notes-2026-05-04.md)
- [文档索引](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/README.md)
- [版本记录](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/README.md)
- [协议总览](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/README.md)
- [WS63 板端样例说明](/Users/bh4me_macair/Documents/Codex/sle_intercom/xc/ws63_team_network/README.md)
- [域名 WebUI 说明](/Users/bh4me_macair/Documents/Codex/sle_intercom/webui/README.md)

## 目录

- [include/](/Users/bh4me_macair/Documents/Codex/sle_intercom/include)：协议公共头文件。
- [src/](/Users/bh4me_macair/Documents/Codex/sle_intercom/src)：协议、组网状态机、串口 CLI、Web API 序列化。
- [examples/](/Users/bh4me_macair/Documents/Codex/sle_intercom/examples)：本地协议测试和接入示例。
- [xc/ws63_team_network/](/Users/bh4me_macair/Documents/Codex/sle_intercom/xc/ws63_team_network)：WS63 上板样例。
- [webui/](/Users/bh4me_macair/Documents/Codex/sle_intercom/webui)：域名上位机 WebUI。
- [docs/](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs)：协议、架构、现场记录。
- [versions/](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions)：版本记录。
- [scripts/](/Users/bh4me_macair/Documents/Codex/sle_intercom/scripts)：编译和烧录脚本。

## WS63 使用

板端 WiFi：

```text
SSID: SLE-TEAM-WS63-XXXX
Password: 123456789
URL: http://192.168.43.1/
```

常用页面：

- `/`：状态。
- `/nodes`：已入队节点。
- `/events`：最近收发事件。
- `/pairing`：角色选择、leader 配队、member 选择 leader。

常用 API：

- `GET /api/status`
- `GET /api/nodes`
- `GET /api/events`
- `GET /api/pending`
- `GET /api/pairing?action=start|stop|approve&id=...`
- `GET /api/member/select?team=...&leader=...&channel=...`
- `GET /api/member/leave`
- `GET /api/factory-reset`

## 编译和烧录

优先使用局域网 Ubuntu 编译机：

```sh
UBUNTU_HOST=192.168.6.5 \
UBUNTU_USER=owen \
UBUNTU_PASS='67215837' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 \
BUILD_JOBS=4 \
scripts/ws63_build_team_ubuntu.sh unified
```

输出统一固件：

```text
/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_unified_runtime_role/ws63-liteos-app_unified_all.fwpkg
```

本机烧 leader：

```sh
printf 'flash leader\n' | scripts/ws63_flash_team.sh leader /dev/tty.usbserial-10
```

烧录脚本中的 `leader/member` 只用于选择串口和二次确认，实际烧录的是同一个统一固件。

## 本地验证

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test && /tmp/sle_team_packet_test
```

## 当前限制

- 当前不是完整 Mesh，仍是 leader/member 星型网络。
- 当前默认成员上限仍按 8 个连接表实现，尚未扩到 20 个。
- member 选择 leader 仍需要填写 leader MAC 后四位，还不是自动扫描附近 leader 后选择。
- `cipher_mac` 目前是协议预留字段，尚未做真实加密认证。
- 板端 WebUI 为保护 RAM 使用 C 端 SSR，不直接烧录 Vite 产物。

## License

Apache-2.0
