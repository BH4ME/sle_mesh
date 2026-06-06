# v4.0.0-alpha1 Manifest

## 变更范围

- `src/sle_team_node.c`
- `examples/team_network_demo.c`
- `xc/ws63_team_network/CMakeLists.txt`
- `xc/ws63_team_network/Kconfig`
- `xc/ws63_team_network/README.md`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `xc/ws63_team_network/src/ws63_st7789_display.h`
- `scripts/ws63_build_v4_ubuntu.sh`
- `docs/README.md`
- `docs/v4/README.md`
- `docs/v4/task_plan.md`
- `docs/v4/findings.md`
- `docs/v4/progress.md`
- `docs/v4/reference/SCH_Schematic1_2026-05-19.pdf`
- `versions/README.md`
- `versions/v4.0.0-alpha1/VERSION.md`
- `versions/v4.0.0-alpha1/MANIFEST.md`
- `README.md`

## 关键改动

1. 协议层：
- 普通 member 超时时，leader 侧补齐 `ALERT_TIMEOUT` 广播。
- alert 里保留最后位置和最后时间。

2. V4 板级配置：
- UART 默认改为 `IO21/IO22`。
- `CONFIG_SLE_TEAM_LED_PIN=255` 禁用旧 GPIO2 LED 驱动。
- SoftAP 默认 SSID 前缀改为 `SLE-TEAM-V4`。

3. 显示：
- 新增 ST7789 SPI 驱动模块。
- 运行时显示角色、在线数、失联信息。

4. 构建与交付：
- 新增 `scripts/ws63_build_v4_ubuntu.sh`，支持远端 Ubuntu 交叉编译。
- 编译脚本源码同步路径统一到 `xc/ws63_team_network/`。

5. 文档与结构治理：
- V4 工作文档归档到 `docs/v4/`。
- 新增 `versions/v4.0.0-alpha1/` 版本账本。
- 根目录 `v4/` 临时工作区已移除。

## 验证

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

./scripts/simulate_v2.sh --suite=core

UBUNTU_HOST=100.91.84.124 UBUNTU_USER=owen UBUNTU_PASS='<set locally, do not commit secrets>' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 BUILD_JOBS=4 \
scripts/ws63_build_v4_ubuntu.sh unified
```

产物：

- `<sdk-root>/output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
