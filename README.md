# sle_mesh

`sle_mesh` 是基于 WS63 SLE 的组网项目，支持运行时 leader/member 角色配置、relay 自适应中继、板端 HTTP WebUI、浏览器 WebUI、远程构建、自动烧录和多板验证。

本 README 使用 UTF-8 中文编写，可在 GitHub 正常显示。

## 当前版本

- 最新仓库记录：`v4.4.96`
- 当前固件版本：`v4.4.95`
- 最新仓库记录说明：[versions/v4.4.96/VERSION.md](versions/v4.4.96/VERSION.md)
- 当前固件记录说明：[versions/v4.4.95/VERSION.md](versions/v4.4.95/VERSION.md)
- 版本索引：[versions/README.md](versions/README.md)

## 目录结构

| 目录 | 用途 |
| --- | --- |
| [firmware/](firmware/) | 固件工程索引和固件版本边界说明。 |
| [xc/ws63_team_network/](xc/ws63_team_network/) | WS63 板端固件工程。 |
| [include/](include/), [src/](src/) | SLE team 协议、状态机、CLI 和 Web API 共享代码。 |
| [examples/](examples/) | 本地 C 回归和演示程序。 |
| [scripts/build/](scripts/build/) | 固件构建脚本。 |
| [scripts/flash/](scripts/flash/) | 烧录脚本。 |
| [scripts/serial/](scripts/serial/) | 串口配置脚本。 |
| [scripts/sim/](scripts/sim/) | 仿真脚本。 |
| [scripts/test/](scripts/test/) | 测试总入口。 |
| [automation/ws63/](automation/ws63/) | WS63 自动化工具、串口预检和多板测试。 |
| [webui/](webui/) | 浏览器 WebUI。 |
| [hardware/](hardware/) | PCB、原理图、3D 打印外壳和制造资料。 |
| [versions/](versions/) | 仓库、固件、硬件版本记录和验证记录。 |
| [docs/](docs/) | 阶段文档和维护说明。 |

更完整的目录说明见 [docs/repository_layout.md](docs/repository_layout.md)。

## 当前固件重点

- 所有 WS63 节点使用同一份统一固件包，leader/member 角色通过串口命令或 WebUI 在运行时配置。
- 支持 leader 配对窗口关闭后保留已经在线的 member。
- 保留动态 relay budget 和 relay swap hysteresis：非 relay 候选需要比当前最弱 relay 强至少 8 dB，并稳定 30 秒后才交换。
- 支持四板自然验证：leader 直连容量受限时，member 可自适应选择直连或 relay。
- 板端 SoftAP/HTTP WebUI 默认启动。

## 板端 WebUI

默认板端 Wi-Fi：

```text
SSID: SLE-TEAM-V4-XXXX
Password: 123456789
URL: http://192.168.43.1/
```

常用页面：

- `/`：状态页。
- `/nodes`：已入队节点。
- `/events`：最近收发事件。
- `/pairing`：角色选择、leader 配队、member 选择 leader、手机定位上报。

常用 API：

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

## 串口配置

板端常用命令：

```text
cfg status
cfg leader now <team> <channel>
cfg member now <leader_suffix_hex> <team> <channel>
cfg apply
cfg clear
cfg reboot
```

Windows PowerShell 辅助脚本：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/serial/ws63_serial_cfg.ps1 -Port COM16 -Mode leader -Team 7 -Channel 33
powershell -ExecutionPolicy Bypass -File scripts/serial/ws63_serial_cfg.ps1 -Port COM13 -Mode member -LeaderSuffix 9A2F -Team 7 -Channel 33
powershell -ExecutionPolicy Bypass -File scripts/serial/ws63_serial_cfg.ps1 -Port COM13 -Mode status
```

## 远程编译

推荐使用远程 Ubuntu 编译机：

```sh
UBUNTU_HOST=192.168.6.5 \
UBUNTU_USER=owen \
UBUNTU_PASS='<set locally, do not commit secrets>' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 \
BUILD_JOBS=4 \
scripts/build/ws63_build_v4_ubuntu.sh unified
```

统一固件输出：

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

## 烧录

单板烧录：

```sh
WS63_FLASH_NO_CONFIRM=1 scripts/flash/ws63_flash_team.sh --yes unified /dev/tty.usbserial-10
```

Windows 多串口烧录：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/flash/ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -Parallel `
  -Firmware output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg `
  -ExpectedVersion v4.4.95
```

自动烧录工具：

```powershell
python automation\ws63\tools\ws63_auto_burn.py `
  -p COM16 `
  -b 115200 `
  --software-reset-only `
  --reset-command reboot `
  output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

## 测试与仿真

```sh
python -m unittest discover -s automation/ws63/tests -t .
python -m unittest tools.test_sle_team_python_sim
scripts/sim/simulate_v2.sh --suite=python --stress=1
npm --prefix webui test
npm --prefix webui run build
git diff --check
```

本地 C smoke test：

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test && /tmp/sle_team_packet_test
```

## 硬件与外壳

- 硬件入口：[hardware/README.md](hardware/README.md)
- 当前 3D 打印外壳：[hardware/enclosures/sle-pcb-enclosure/v1.1.4/](hardware/enclosures/sle-pcb-enclosure/v1.1.4/)
- 以后上传 PCB、Gerber、BOM、原理图时，放入 `hardware/boards/` 或 `hardware/schematics/` 的对应版本目录。

## 版本管理

- 仓库、固件和硬件版本规则见 [docs/version_management.md](docs/version_management.md)。
- 固件行为变化时更新固件版本和 `versions/<version>/`。
- 仅文档、目录或脚本布局变化时，更新仓库记录版本，不冒充固件升级。
- 构建产物、日志、本地草稿和临时模型不提交到 Git。

## License

Apache-2.0
