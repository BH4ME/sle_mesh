# WS63 SLE Team Network Sample

这是把 `sle_mesh` 协议骨架接到小熊派 `BearPi-Pico H3863 / WS63` 的上板样例。

当前目标很务实：先跑通两块板的最小组网闭环，并让现场只靠手机内置 WebUI 就能完成角色选择和配队。

```text
member --HELLO--> leader
member <--ACK----- leader
member <--CONFIG-- leader
member --HB/POS--> leader
```

## 当前能力

- 一个统一固件包，所有 WS63 都烧同一份 `.fwpkg`。
- 开机默认 `unconfigured`，SLE 暂不启动；先启动 SoftAP、HTTP WebUI 和串口 CLI。
- WebUI 或串口选择角色后，leader 启动 SLE UART client/central，member 启动 SLE UART server/peripheral。
- WebUI 选择的角色、队伍号、leader MAC 后四位和 channel 会写入 WS63 NV flash；断电/复位后自动恢复。
- WiFi SSID 和页面自标识使用 WiFi MAC 后四位，适合批量烧录：`SLE-TEAM-WS63-XXXX`、`UXXXX`、`LXXXX`、`MXXXX`。
- SLE 收到二进制包后进入 `sle_team_node_on_packet`，周期任务调用 `sle_team_node_tick`。
- leader 可开/关配队窗口、批准 pending member；member 可选择 leader 或退出队伍。
- 板端 WebUI 用 C 端 SSR 输出完整 HTML，不烧录 Vite 产物，不依赖前端 JS，优先保护 RAM 和手机兼容性。

## WiFi 控制台

```text
SSID: SLE-TEAM-WS63-XXXX
Password: 123456789
URL: http://192.168.43.1/
```

接口：

```text
GET /api/status
GET /api/nodes
GET /api/events
GET /api/pending
GET /api/pairing?action=start|stop|approve&id=...
GET /api/member/select?team=...&leader=...&channel=...
GET /api/member/leave
GET /api/factory-reset
```

页面：

- `/`：状态页。未选角色时显示 `unconfigured`。
- `/nodes`：节点列表。空数组 `[]` 表示当前还没有 member 入队。
- `/events`：收发事件。空数组 `[]` 表示还没有真实 SLE 包。
- `/pairing`：角色选择、leader 配队、member 选择 leader。
- `/api/factory-reset`：清除 WebUI 写入 flash 的配置并重启，恢复到 `UXXXX` 未配置状态。

手机调试注意：

- WS63 SoftAP 没有外网，手机可能自动切蜂窝；如果浏览器请求失败，先确认手机仍连在 `SLE-TEAM-WS63-XXXX`。
- Safari 或微信内置浏览器刷新页面会取消旧 HTTP 连接，串口可能出现 `errno=104`。这通常是客户端断开，不代表板端 HTTP 服务崩溃。
- `v1.2.7 / ssr=v5` 起，每个 HTTP 连接都有 1.2 秒收发超时，手机半开连接不会长期占住板端 Web 服务。
- `set leader` / `set member` 后会先返回页面，SLE 初始化在后台主任务执行；页面可能短暂显示 `starting SLE`。
- 操作类按钮会自动回到 `/pairing`，不会停在 JSON 页面。

## 角色选择逻辑

统一固件启动后，设备标识是 `UXXXX`。

leader：

1. 手机连该板 `SLE-TEAM-WS63-XXXX`。
2. 打开 `http://192.168.43.1/pairing`。
3. 点 `set leader`。
4. 页面标识变为 `LXXXX`，leader 开始 SLE 广播/server。

member：

1. 手机连该板 `SLE-TEAM-WS63-YYYY`。
2. 打开 `http://192.168.43.1/pairing`。
3. 在 member 表单里填 leader 的后四位 `XXXX`、队伍号和 channel。
4. 点 `set member`。
5. 页面标识变为 `MYYYY`，member 启动 SLE client 并向 leader 发送 HELLO。

说明：

- MAC 后四位用于现场识别和选择 leader。
- 完整 MAC 会放进 HELLO，leader 的 pending/member 记录会保存完整 MAC。
- 1 字节 route ID 仍用于包路由，由 MAC 低字节派生；极端情况下 MAC 低字节可能冲突，后续做大规模队伍时需要升级路由 ID 或加入绑定表。
- 当前 WebUI 角色配置会持久化到 NV flash；member 点 `leave` 或 WebUI 点 `factory reset` 会清掉该配置。
- SLE pending 列表、节点在线状态和事件日志仍是运行时状态，不写 flash，重启后重新发现。

## 编译和烧录

优先使用局域网 Ubuntu 编译机：

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

烧录脚本里的 `leader/member` 只用于选择默认串口和二次确认，实际烧的是同一个统一固件包：

```sh
scripts/ws63_flash_team.sh leader /dev/tty.usbserial-10
scripts/ws63_flash_team.sh member /dev/tty.usbserial-110
```

脚本会在烧录前打印角色、串口和固件路径，并要求输入 `flash leader` 或 `flash member` 才会继续。
macOS 烧录优先使用 `/dev/tty.usbserial-*`，不要用 `/dev/cu.usbserial-*`。
看到 `Waiting for device reset...` 后，需要按板子的 `BOOT + RESET` 或按当前烧录器要求复位让它握手。

## LED 诊断

已确认的小熊派 WS63 蓝色用户灯配置：

- 引脚：`GPIO2`
- 极性：`active-high`
- 空闲保持灭灯；SLE 配队/扫描和真实协议 TX/RX 使用不同闪烁节奏。
- pairing window 打开时 leader 会每秒极短闪一下，表示正在开放发现/配队。
- leader pairing window open 或尚未连上 member 时会每秒极短闪一下，表示正在扫描 member。
- `[sle-tx-ok]`：本板已把业务包交给 SLE driver，LED 快闪两下。
- `[sle-tx-fail]`：本板尝试 SLE 发送失败，LED 不闪。
- `[sle-rx]`：本板真实收到 SLE 业务包，LED 慢闪一下。

串口诊断命令：

```text
led status
led on
led off
led tx
led rx
led active_high
led active_low
led pin <0-31>
```

## 串口命令

串口波特率：`115200 8N1`

未选角色时：

```text
wifi
http
role leader
role member <leader_mac_suffix>
led status
```

选好角色后：

```text
help
state
members
pairing start
pairing pending
pairing approve <id>
pairing stop
join <team> <leader_id> <channel>
leave
hello [dst]
hb [dst] [battery] [rssi] [fix]
pos [dst] [lat_e6] [lon_e6] [speed] [heading] [battery] [fix] [sat]
```

`role member` 用 leader MAC 后四位；`join` 是已配置 member 后的低层调试命令，参数仍是内部 1 字节 leader ID。

## 当前不是完整 Mesh

第一版仍是星型 SLE 连接：

- leader = client / central / seeker
- member = server / peripheral / advertiser
- leader 主动连接 member
- 业务包使用 MeshCore 风格外层包帧，后面可扩展成中继或 flood

这样做的原因是先确保真实 WS63 SLE 收发、串口控制、状态机、板端 WebUI 都跑通，再做多跳。

## 文件说明

- `src/ws63_team_network_app.c`：WS63 上板适配层、WiFi 控制台、运行时角色选择。
- `Kconfig`：统一固件、串口、WiFi、心跳、LED 参数。
- `CMakeLists.txt`：把协议核心和 SLE UART helper 拉进样例。

协议核心来自仓库根目录：

- `include/sle_team_packet.h`
- `include/sle_team_node.h`
- `include/sle_team_cli.h`
- `src/sle_team_packet.c`
- `src/sle_team_node.c`
- `src/sle_team_cli.c`

## 已知限制

- `v1.2.9` 起连接方向改成官方 1vs8 样例方向：leader 扫描并连接多个 member，member 广播等待连接。
- pairing window open 时 leader 会持续扫描；需要现场串口确认两个 member 能否同时连接并进入 pending。
- 还没有扩到 20 个 member，仍保持当前 `SLE_TEAM_MAX_MEMBERS`。
- 当前不是完整“扫描附近 leader 并选择”的发现系统，member 需要填写 leader MAC 后四位。
- 当前配队不是加密认证，`cipher_mac` 仍未用于真实鉴权。
- 角色和配队配置已写入 flash；factory reset 会清除配置并重启。
