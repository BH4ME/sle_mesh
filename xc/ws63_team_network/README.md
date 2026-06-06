# V4.4.32 WS63 SLE Team Network + ST7789

这是 v4 上板工程，基于 v3 组网样例改出，目标板主控为 WS63 模块，显示器为 1.14 寸 ST7789。主线仍然是组网：leader 负责配队和汇总 member 状态，member 失联时保留最后一次位置并生成 timeout alert。

```text
member --HELLO--> leader
member <--ACK----- leader
member <--CONFIG-- leader
member --HB/POS--> leader
```

## 当前能力

- 一个统一固件包，所有 WS63 都烧同一份 `.fwpkg`。
- 开机默认 `unconfigured`，SLE 暂不启动；SoftAP、HTTP WebUI 和串口 CLI 必须默认启动，供板端页面、域名 WebUI 和批量部署读取/写入配置。
- WebUI 或串口选择角色后，leader 启动 SLE UART client/central，member 启动 SLE UART server/peripheral。
- 角色、队伍号、leader MAC 后四位和 channel 会写入 WS63 NV flash；断电/复位后自动恢复。
- WiFi SSID 和页面标识使用 WiFi MAC 后四位：`SLE-TEAM-V4-XXXX`、`UXXXX`、`LXXXX`、`MXXXX`。
- ST7789 显示角色、节点数、失联次数和最后失联位置。
- 普通子节点心跳超时后，leader 会广播 `ALERT_TIMEOUT`，alert 内包含最后一次位置和最后上报时间。
- 板端 WebUI 用 C 端 SSR 输出完整 HTML，不烧录 Vite 产物，优先保护 RAM 和手机兼容性。

## V4 引脚

| 信号 | WS63 IO | 用途 |
|------|---------|------|
| `U0TX` | `IO21` | 串口调试 TX |
| `U0RX` | `IO22` | 串口调试 RX |
| `RGB` | `IO0` | WS2812C 数据脚 |
| `CHRG` | `IO2` | 充电状态，不作为 LED 驱动 |
| `ADC_CTRL` | `IO5` | 电池采样控制，预留 |
| `SCL` | `IO6` | ST7789 SPI SCLK |
| `CS` | `IO7` | ST7789 CS |
| `SDA` | `IO8` | ST7789 SPI MOSI |
| `RS` | `IO9` | ST7789 DC/RS |
| `ADC_VBAT` | `IO12` | 电池 ADC，预留 |
| `RESET` | `IO13` | ST7789 RESET |
| `BUZZ` | `IO14` | 蜂鸣器 |
| `U1TX` | `IO17` | GPS UART TXD1 |
| `U1RX` | `IO18` | GPS UART RXD1 |

## v4.4+ 确认项

- ST7789：`GPIO6/7/8/9/13`，`240x135`，offset `40,53`，MADCTL `0x60`，soft SPI mode 0。
- 背光：BLK 由硬件/default-on 处理，固件不要重新加入 GPIO11 控制。
- GPS：当前只保留 `IO17/IO18` pinmap 和日志，不做完整 GPS/NMEA 数据解析。
- WS2812：Kconfig 默认 `n`，v4 构建脚本启用 IO0。
- 蜂鸣器：默认关闭，只做安全拉低/命令控制。
- SLE 功率：实际 announce 参数和广播声明字段统一为 `18 dBm`。

屏幕问题和解决过程见 [../../versions/v4.4/ST7789_DISPLAY_FIX.md](../../versions/v4.4/ST7789_DISPLAY_FIX.md)。每次改代码、编译和烧录前先读 [../../meta/PROJECT_OPERATION_SOP.md](../../meta/PROJECT_OPERATION_SOP.md)。

## WiFi 控制台

```text
SSID: SLE-TEAM-V4-XXXX
Password: 123456789
URL: http://192.168.43.1/
```

接口：

```text
GET /api/status
GET /api/nodes
GET /api/events
GET /api/pending
GET /api/location?lat=...&lon=...&dst=255&speed=...&heading=...&battery=...&fix=...&sat=...
GET /api/config/status
GET /api/config/leader?team=1&channel=17&now=1
GET /api/config/member?leader=C7E9&team=1&channel=17&now=1
GET /api/config/apply
GET /api/config/clear
GET /api/config/reboot
GET /api/pairing?action=start|stop|approve&id=...
GET /api/member/select?team=...&leader=...&channel=...
GET /api/member/leave
GET /api/factory-reset
```

页面：

- `/`：状态页。未选角色时显示 `unconfigured`。
- `/nodes`：节点列表。空数组 `[]` 表示当前还没有 member 入队。
- `/events`：收发事件。空数组 `[]` 表示还没有真实 SLE 包。
- `/pairing`：角色选择、leader 配队、member 选择 leader、手机定位上报。
- `/api/factory-reset`：清除 WebUI 写入 flash 的配置并重启，恢复到 `UXXXX` 未配置状态。

## 串口配置

串口波特率：`115200 8N1`

未选角色时也可用：

```text
wifi
http
cfg status
cfg leader [team channel]
cfg leader now <team> <channel>
cfg member <leader_suffix_hex> <team> <channel>
cfg member now <leader_suffix_hex> <team> <channel>
cfg apply
cfg clear
cfg reboot
reboot
reset
```

批量部署推荐命令：

```text
cfg leader now 7 33
cfg member now 9A2F 7 33
cfg status
```

Windows 批量串口脚本：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM7 -Mode leader -Team 7 -Channel 33
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM8 -Mode member -LeaderSuffix 9A2F -Team 7 -Channel 33
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM8 -Mode status
```

串口 `cfg status` 会输出结构化日志：

```text
[cfg-json] {"ok":true,...}
```

域名/上位机 WebUI 会解析这个 `[cfg-json]` 并显示在 `Settings -> One-click node config` 的串口日志里。

## 角色选择逻辑

统一固件启动后，设备标识是 `UXXXX`。

leader：

1. 手机连该板 `SLE-TEAM-V4-XXXX`，或用 WebSerial 连接串口。
2. WebUI 调 `/api/config/leader?team=1&channel=17&now=1`，或串口发 `cfg leader now 1 17`。
3. 配置写入 NV，leader 开始 SLE 扫描/连接 member。

member：

1. 获取 leader 的 MAC 后四位，例如 `9A2F`。
2. WebUI 调 `/api/config/member?leader=9A2F&team=1&channel=17&now=1`，或串口发 `cfg member now 9A2F 1 17`。
3. 配置写入 NV，member 启动 SLE 广播并等待 leader 连接。

说明：

- MAC 后四位用于现场识别和选择 leader。
- 完整 MAC 会放进 HELLO，leader 的 pending/member 记录会保存完整 MAC。
- 1 字节 route ID 仍用于包路由，由 MAC 低字节派生；极端情况下 MAC 低字节可能冲突，后续大规模队伍需要升级路由 ID 或加入绑定表。
- SLE pending 列表、节点在线状态和事件日志仍是运行时状态，不写 flash，重启后重新发现。

## 编译和烧录

优先使用局域网 Ubuntu 编译机：

```sh
UBUNTU_HOST=192.168.6.5 \
UBUNTU_USER=owen \
UBUNTU_PASS='<set locally, do not commit secrets>' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 \
BUILD_JOBS=4 \
scripts/ws63_build_v4_ubuntu.sh unified
```

输出统一固件包：

```text
<repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

COM16 自动烧录流程见 [../../meta/PROJECT_OPERATION_SOP.md](../../meta/PROJECT_OPERATION_SOP.md)，成功记录见 [../../versions/v4.4/AUTO_FLASH_NOTES.md](../../versions/v4.4/AUTO_FLASH_NOTES.md)。

## 当前不是完整 Mesh

第一版仍是星型 SLE 连接：

- leader = client / central / seeker
- member = server / peripheral / advertiser
- leader 主动连接 member
- 业务包使用 MeshCore 风格外层包帧，后面可扩展成中继或 flood

这样做的原因是先确保真实 WS63 SLE 收发、串口控制、状态机、板端 WebUI 都跑通，再做多跳。

## 文件说明

- `src/ws63_team_network_app.c`：WS63 上板适配层、WiFi 控制台、运行时角色选择、串口 `cfg` 配置。
- `src/ws63_st7789_display.c`：ST7789 初始化、显示刷新、可选 LVGL 后端和内置文字 fallback。
- `Kconfig`：统一固件、串口、WiFi、心跳、LED、ST7789 参数。
- `CMakeLists.txt`：把协议核心和 SLE UART helper 拉进样例。

协议核心来自仓库根目录：

- `include/sle_team_packet.h`
- `include/sle_team_node.h`
- `include/sle_team_cli.h`
- `src/sle_team_packet.c`
- `src/sle_team_node.c`
- `src/sle_team_cli.c`

## 已知限制

- 当前不是完整“扫描附近 leader 并选择”的发现系统，member 需要填写 leader MAC 后四位。
- 当前配队不是加密认证，`cipher_mac` 仍未用于真实鉴权。
- 当前逻辑成员上限已经对齐到 `30`，但真实 30 节点联调仍需继续依赖板级回归验证。
- 如果 SDK 没有 LVGL 头文件，固件会自动 fallback 到内置文字渲染器。
