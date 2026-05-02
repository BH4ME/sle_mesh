# WS63 SLE Team Network Sample

这是把 `sle_mesh` 协议骨架接到小熊派 `BearPi-Pico H3863 / WS63` 的第一版上板样例。

当前目标很务实：先跑通两块板的最小组网闭环。

```text
member --HELLO--> leader
member <--ACK----- leader
member <--CONFIG-- leader
member --HB/POS--> leader
```

## 当前能力

- 一套代码通过 Kconfig 切换 `leader/member`
- leader 复用 SDK 的 SLE UART server
- member 复用 SDK 的 SLE UART client
- 串口终端输入命令，调用 `sle_team_cli`
- SLE 收到二进制包后进入 `sle_team_node_on_packet`
- 周期任务调用 `sle_team_node_tick`
- leader 可启动 SoftAP，并通过手机访问板端 HTTP 控制台

## v1.2.3 WiFi 控制台

leader 固件当前提供本机控制台：

```text
SSID: SLE-TEAM-WS63-L1
Password: 123456789
URL: http://192.168.43.1/
```

接口：

```text
GET /api/status
GET /api/nodes
GET /api/events
```

页面行为：

- 当前稳定版为 `ssr=v3`，板端直接渲染 `/`、`/nodes`、`/events` 三个页面。
- 页面使用带 `Content-Length` 的完整 HTTP 响应，避免 iOS/微信内置浏览器对流式响应一闪而过。
- 页面底部显示 `page=... ssr=v3`，用于确认当前烧录的是这版固件。
- `nodes` / `events` 显示 `[]` 表示 leader 当前没有 member 入网。

当前现场基线：

- `status` 已确认正常。
- `nodes/events` 页面已确认可打开。
- 当前 leader 没有 member 加入，所以节点和事件列表没有真实业务数据。

手机调试注意：

- WS63 SoftAP 没有外网，手机可能自动切蜂窝；如果浏览器请求失败，先确认手机仍连在 `SLE-TEAM-WS63-L1`。
- Safari 刷新页面会取消旧 HTTP 连接，串口可能出现 `errno=104`。这通常是客户端断开，不代表板端 HTTP 服务崩溃。
- 板端 HTTP 是轻量单连接服务，调试时优先使用页面内按钮，不要连续点浏览器底部刷新。

最新已烧录过的 leader 可用包：

```text
/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_leader_wifi_console_ssr_v3/ws63-liteos-app_all.fwpkg
```

## 当前不是完整 Mesh

第一版先不是多跳 Mesh，而是星型 SLE 连接：

- leader = server / advertiser
- member = client / seeker
- member 主动连接 leader
- 业务包仍然使用 MeshCore 风格外层包帧，后面可扩展成中继或 flood

这样做的原因是：先确保真实 WS63 SLE 收发、串口控制、状态机都跑通，再做多跳，否则调试面太大。

## 文件说明

- `src/ws63_team_network_app.c`：WS63 上板适配层
- `Kconfig`：leader/member、节点 ID、串口和心跳参数
- `CMakeLists.txt`：把协议核心和 SLE UART helper 拉进样例

协议核心仍然来自仓库根目录：

- `include/sle_team_packet.h`
- `include/sle_team_node.h`
- `include/sle_team_cli.h`
- `src/sle_team_packet.c`
- `src/sle_team_node.c`
- `src/sle_team_cli.c`

## 接入 BearPi SDK

推荐放置方式：

```text
bearpi-pico_h3863/
  application/samples/products/sle_team_network/
    CMakeLists.txt
    Kconfig
    src/ws63_team_network_app.c
  third_party/sle_mesh/
    include/
    src/
```

也就是：

1. 把本目录复制到 SDK 的 `application/samples/products/sle_team_network`
2. 把本仓库的 `include/` 和 `src/` 复制到 SDK 的 `third_party/sle_mesh/`
3. 在 `CMakeLists.txt` 里让 `SLE_MESH_ROOT` 指向 `third_party/sle_mesh`

如果你不想复制协议核心，也可以在 `CMakeLists.txt` 里把 `SLE_MESH_ROOT` 改成外部绝对路径。

默认 `CMakeLists.txt` 已经兼容两种位置：

- 放在本仓库 `xc/ws63_team_network` 时，自动使用仓库根目录的 `include/` 和 `src/`
- 复制到 SDK `application/samples/products/sle_team_network` 时，自动使用 SDK 根目录的 `third_party/sle_mesh`

## SDK products CMake 接入

在 SDK 的：

```text
application/samples/products/CMakeLists.txt
```

增加：

```cmake
if(DEFINED CONFIG_SAMPLE_SUPPORT_SLE_TEAM_NETWORK)
    add_subdirectory_if_exist(sle_team_network)
endif()
```

同时需要在产品 sample 的 Kconfig 中增加一个开关，例如：

```kconfig
config SAMPLE_SUPPORT_SLE_TEAM_NETWORK
    bool "Support SLE team network sample"
    default y
```

不同 SDK 版本 Kconfig 组织略有差异，照 `sle_uart` 的接入方式放即可。

## 编译两种固件

leader 固件：

```text
CONFIG_SAMPLE_SUPPORT_SLE_TEAM_NETWORK=y
CONFIG_SLE_TEAM_NODE_IS_LEADER=y
CONFIG_SLE_TEAM_SELF_ID=1
```

member 固件：

```text
CONFIG_SAMPLE_SUPPORT_SLE_TEAM_NETWORK=y
# CONFIG_SLE_TEAM_NODE_IS_LEADER is not set
CONFIG_SLE_TEAM_SELF_ID=2
CONFIG_SLE_TEAM_LEADER_ID=1
```

两块板要保持一致：

```text
CONFIG_SLE_TEAM_TEAM_ID=1
CONFIG_SLE_TEAM_CHANNEL_HASH=0x11
```

默认串口脚位跟官方 `sle_uart` 样例保持一致：

```text
CONFIG_SLE_TEAM_UART_BUS=0
CONFIG_SLE_TEAM_UART_TXD_PIN=17
CONFIG_SLE_TEAM_UART_RXD_PIN=18
```

本次在 Debian VM 里已经验证过两种配置都能编译通过：

```text
leader: Build target:ws63_liteos_app success, packet success
member: Build target:ws63_liteos_app success, packet success
```

Mac 侧已拷出的固件路径：

```text
/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_leader/ws63-liteos-app_team_leader_all.fwpkg
/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_member/ws63-liteos-app_team_member_all.fwpkg
```

烧录 leader 示例：

```sh
/Users/bh4me_macair/Library/Python/3.9/bin/burn \
  -p /dev/tty.usbserial-10 \
  -b 115200 \
  /Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_leader/ws63-liteos-app_team_leader_all.fwpkg
```

看到 `Waiting for device reset...` 后，需要按板子的 `RST/RESET` 键让烧录器握手。

## 串口命令

串口波特率：

```text
115200 8N1
```

上电后输入：

```text
help
state
members
hello 1
hb 1 90 -45 1
pos 1 39908456 116397128 100 90 88 1 9
```

leader 上常用：

```text
members
state
config 2
```

member 上常用：

```text
state
hello 1
pos 1 39908456 116397128 100 90 88 1 9
```

## 预期日志

member 成功入网：

```text
[team] joined member=2
```

leader 收到位置：

```text
[team] pos member=2 lat=39908456 lon=116397128 battery=88
```

leader 查询成员：

```text
member=2 role=0 online=1 battery=88 fix=1 ...
```

## 下一步

- 第一步：两块板跑通 leader/member
- 第二步：把 member 心跳里的电量、RSSI、定位状态换成真实传感器/GNSS 数据
- 第三步：leader 增加超时检测，超时后广播 `ALERT`
- 第四步：再考虑 1 对多和中继/flood
