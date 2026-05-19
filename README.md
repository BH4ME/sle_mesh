# sle_mesh

`sle_mesh` 是 WS63 星闪组网工程。根目录只放“怎么编译、怎么烧录、怎么验证、怎么审查”。

## 文档入口

- [docs/README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/README.md)：总索引
- [docs/v0/README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/v0/README.md)：V0（1vs2 / 1vs8）
- [docs/v1/README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/v1/README.md)：V1（手动 relay）
- [docs/v2/README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/v2/README.md)：V2（自动组网）
- [docs/v3/README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/v3/README.md)：V3（手机定位桥接）
- [docs/v4/README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/v4/README.md)：V4（WS63 模块 + ST7789）
- [versions/README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/README.md)：版本记录
- [meta/DOC_WORKFLOW.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/meta/DOC_WORKFLOW.md)：文档编写与维护流程

## 目录

- [include/](/Users/bh4me_macair/Documents/Codex/sle_intercom/include)：协议公共头文件。
- [src/](/Users/bh4me_macair/Documents/Codex/sle_intercom/src)：协议、组网状态机、串口 CLI、Web API 序列化。
- [examples/](/Users/bh4me_macair/Documents/Codex/sle_intercom/examples)：本地协议测试和接入示例。
- [xc/ws63_team_network/](/Users/bh4me_macair/Documents/Codex/sle_intercom/xc/ws63_team_network)：WS63 上板样例。
- [webui/](/Users/bh4me_macair/Documents/Codex/sle_intercom/webui)：域名上位机 WebUI。
- [docs/](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs)：按 v0~v4 分组的协议与组网文档。
- [versions/](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions)：版本记录。
- [scripts/](/Users/bh4me_macair/Documents/Codex/sle_intercom/scripts)：编译和烧录脚本。
- [automation/ws63/](/Users/bh4me_macair/Documents/Codex/sle_intercom/automation/ws63)：自动化烧录、角色绑定与回归测试（独立目录）。

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
- `GET /api/location?lat=...&lon=...&dst=255&speed=...&heading=...&battery=...&fix=...&sat=...`
- `GET /api/pairing?action=start|stop|approve&id=...&relay=0|1`
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

烧录脚本中的 `leader/member` 只用于选择串口和二次确认，实际烧录的是同一个统一固件。脚本默认会通过 `automation/ws63/tools/ws63_auto_burn.py`（保留兼容入口 `tools/ws63_auto_burn.py`）先发串口 `reboot` 并尝试 DTR/RTS 自动复位；第一次从老固件升级或烧录器没有复位控制线时，仍可能需要手按一次 `RESET/RST`。小熊派 WS63 这类没有 BOOT 键的开发板按 RESET 即可；带 BOOT 下载键的板子才需要按住 BOOT 再点 RESET。临时关闭自动复位：

```sh
printf 'flash leader\n' | AUTO_RESET=0 scripts/ws63_flash_team.sh leader /dev/tty.usbserial-10
```

## 本地验证

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test && /tmp/sle_team_packet_test
```

一键仿真（自动构建并运行两套测试，输出日志）：

```sh
./scripts/simulate_v2.sh
```

日志输出：
- `logs/sim/network_test.log`
- `logs/sim/packet_test.log`
- `logs/sim/relay_rebalance_test.log`（relay 断链补选与重连身份场景）

## 一键审查（DeepSeek）

```sh
scripts/run_review_with_deepseek.sh --scope "Bugfix / PR 审查" --goal-doc docs/v2/networking-goal.md
```

说明：
- 脚本会强制 DeepSeek 先读取 `docs/v2/review_framework.md`，并按 Stage 执行。
- 审查结果会覆盖写入`meta/review_feedback.md`。
- 可选 `--model deepseek-chat` 或其他 DeepSeek 模型。
- 先预览 prompt 可用 `--dry-run`。
- `scripts/run_review_with_gpt.sh` 仍可用，但已作为兼容入口转发到 DeepSeek 脚本。
- 审查与文档维护完整流程见 [meta/DOC_WORKFLOW.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/meta/DOC_WORKFLOW.md) 第 7、8 节。

## License

Apache-2.0
