# sle_mesh

## Current Release

- Current repo record: `v4.4.95`
- Current firmware version: `v4.4.95`
- Current version record: [versions/v4.4.95/VERSION.md](versions/v4.4.95/VERSION.md)
- Flash/test logs should be saved under `logs/burn/` and referenced from the matching `versions/<version>/` notes.

`sle_mesh` �?WS63 星闪组网工程。根目录保留编译、烧录、验证、版本记录和审查入口；板端业务代码在 `xc/ws63_team_network/`，域�?上位�?WebUI �?`webui/`�?

## 文档入口

- [docs/README.md](docs/README.md)：总索�?
- [docs/v0/README.md](docs/v0/README.md)：V0�?vs2 / 1vs8�?
- [docs/v1/README.md](docs/v1/README.md)：V1（手�?relay�?
- [docs/v2/README.md](docs/v2/README.md)：V2（自动组网）
- [docs/v3/README.md](docs/v3/README.md)：V3（手机定位桥接）
- [docs/v4/README.md](docs/v4/README.md)：V4（WS63 模块 + ST7789�?
- [versions/README.md](versions/README.md)：版本记�?
- [versions/v4.4.95/VERSION.md](versions/v4.4.95/VERSION.md)：当�?v4.4.95 仓库记录
- [meta/PROJECT_OPERATION_SOP.md](meta/PROJECT_OPERATION_SOP.md)：每次改代码、远程编译、自动烧录和版本管理前必须读取的作业 SOP
- [meta/DOC_WORKFLOW.md](meta/DOC_WORKFLOW.md)：文档编写与维护流程

## 当前版本

- 当前仓库记录版本：`v4.4.95`
- 当前固件版本：`v4.4.95`
- 统一固件：所�?WS63 节点烧同一�?`.fwpkg`，leader/member 通过 WebUI 或串口运行时配置�?- 板端 SoftAP/HTTP WebUI 默认自动启动；域�?上位�?WebUI 仍依赖板�?HTTP API 做一键配置和状态读取�?- ST7789 已确认参数：`240x135`，offset `40,53`，MADCTL `0x60`�?
- BLK/backlight 不再�?GPIO11 控制；背光按硬件默认开启�?
- GPS 当前只保�?pinmap 和日志，不做完整 NMEA/GPS 数据解析�?
- WS2812 �?Kconfig 默认�?`n`，构建脚本会�?v4 板子启用 IO0�?
- 蜂鸣器默认关闭，只做安全拉低/命令控制�?
- SLE 广播实际发射功率和广播声明字段统一�?`18 dBm`�?- ST7789/LVGL 屏幕事件显示已同步为科技感链路面板：`JOIN/LEFT/TIMEOUT/LOST/REJOIN + Mxxxx`，其�?`Mxxxx` 使用成员 MAC 后四位，避免再出�?`M241` 这种内部十进�?ID；v4.4.95 保留动�?relay budget，并增加 relay swap 滞回：非 relay 比最�?relay �?8 dB 且持�?30s 才交换�?
## 目录

- [include/](include)：协议公共头文件�?
- [src/](src)：协议、组网状态机、串�?CLI、Web API 序列化�?
- [examples/](examples)：本地协议测试和接入示例�?
- [xc/ws63_team_network/](xc/ws63_team_network)：WS63 上板样例和板�?WebUI�?
- [webui/](webui)：域�?上位�?WebUI，支�?WiFi HTTP �?WebSerial�?
- [versions/](versions)：版本记录、烧录记录和硬件问题记录�?
- [scripts/](scripts)：编译、烧录、串口配置脚本�?
- [automation/ws63/](automation/ws63)：自动化烧录、角色绑定与回归工具�?

## WS63 使用

板端 WiFi�?

```text
SSID: SLE-TEAM-V4-XXXX
Password: 123456789
URL: http://192.168.43.1/
```

常用页面�?

- `/`：状态�?
- `/nodes`：已入队节点�?
- `/events`：最近收发事件�?
- `/pairing`：角色选择、leader 配队、member 选择 leader、手机定位上报�?

常用 API�?

- `GET /api/status`
- `GET /api/nodes`
- `GET /api/events`
- `GET /api/pending`
- `GET /api/location?lat=...&lon=...&dst=255&speed=...&heading=...&battery=...&fix=...&sat=...`
- `GET /api/config/status`
- `GET /api/config/leader?team=1&channel=17&now=1`
- `GET /api/config/member?leader=C7E9&team=1&channel=17&now=1`
- `GET /api/config/apply`
- `GET /api/config/clear`
- `GET /api/config/reboot`
- `GET /api/pairing?action=start|stop|approve&id=...&relay=0|1`
- `GET /api/member/select?team=...&leader=...&channel=...`
- `GET /api/member/leave`
- `GET /api/factory-reset`

## 批量串口配置

v4.4 起支持通过串口一键配置，适合 30 个节点批量部署，不需要逐个连接每块板子�?WiFi�?

```text
cfg status
cfg leader now <team> <channel>
cfg member now <leader_suffix_hex> <team> <channel>
cfg apply
cfg clear
cfg reboot
```

Windows PowerShell 脚本�?

```powershell
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM7 -Mode leader -Team 7 -Channel 33
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM8 -Mode member -LeaderSuffix 9A2F -Team 7 -Channel 33
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM8 -Mode status
```

域名/上位�?WebUI �?`Settings -> One-click node config` 也支�?WebSerial：选择串口后可以读�?`[cfg-json]`、写�?leader/member 配置、apply/clear/reboot，并显示串口日志�?

## 编译和烧�?

优先使用局域网 Ubuntu 编译机：

```sh
UBUNTU_HOST=192.168.6.5 \
UBUNTU_USER=owen \
UBUNTU_PASS='<set locally, do not commit secrets>' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 \
BUILD_JOBS=4 \
scripts/ws63_build_v4_ubuntu.sh unified
```

输出统一固件�?

```text
<repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

COM16 自动烧录命令�?

```powershell
python <repo-root>\automation\ws63\tools\ws63_auto_burn.py `
  -p COM16 `
  -b 115200 `
  --software-reset-only `
  --reset-command reboot `
  --reset-command-fallback reset `
  --reset-command-delay 0.3 `
  --reset-command-retries 2 `
  --reset-command-retry-gap 0.2 `
  <repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

固定烧录流程�?[meta/PROJECT_OPERATION_SOP.md](meta/PROJECT_OPERATION_SOP.md)，成功烧录记录见 [versions/v4.4/AUTO_FLASH_NOTES.md](versions/v4.4/AUTO_FLASH_NOTES.md)�?

## 本地验证

```sh
npm --prefix webui test
npm --prefix webui run build
git diff --check
```

协议本地测试�?

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test && /tmp/sle_team_packet_test
```

## License

Apache-2.0
