# Progress Log

## Session: 2026-05-19

### Phase 1: 需求与依据确认
- **Status:** complete
- **Started:** 2026-05-19
- Actions taken:
  - 读取仓库现有版本与 `xc/ws63_team_network` 结构
  - 读取 `v3.0.0-alpha8` 的版本说明和清单
  - 提取原理图第 3、4 页的关键引脚和外设信息
  - 建立 v4 计划与发现文档（后续已归档到 `docs/v4/`）
  - 按 TDD 增加普通 member 超时失联 alert 行为测试
  - 修改核心状态机，在 leader 剪枝超时 member 时广播 `ALERT_TIMEOUT`
  - 复制 v3 上板工程并在 `xc/ws63_team_network` 上完成 v4 改造
  - 增加 ST7789 显示模块和 v4 引脚配置
  - 把 ST7789 初始化接到 v4 启动路径
  - 把 v4 默认串口 fallback 从 IO17/18 改成 IO21/22
  - 把 ST7789 文本从占位伪字体换成 6x8 ASCII 点阵
  - 增加 ST7789 135x240 可视窗口 offset 配置
  - 显式打开 v4 WebUI/SoftAP、SPI master 依赖
  - 使用 Tailscale 远端主机 `100.91.84.124` 完成 Ubuntu 交叉构建
- Files created/modified:
  - `docs/v4/README.md` (created)
  - `docs/v4/task_plan.md` (created)
  - `docs/v4/findings.md` (created)
  - `docs/v4/progress.md` (created)
  - `src/sle_team_node.c` (modified)
  - `examples/team_network_demo.c` (modified)
  - `xc/ws63_team_network/` (updated for v4)
  - `scripts/ws63_build_v4_ubuntu.sh` (created)

## Test Results
| Test | Input | Expected | Actual | Status |
|------|-------|----------|--------|--------|
| session catchup | `python3 .../session-catchup.py` | 读取上次上下文 | 成功 | ✓ |
| member timeout alert | `cc ... team_network_demo.c ... && /tmp/sle_team_network_test` | leader 广播带最后位置的 `ALERT_TIMEOUT` | 通过 | ✓ |
| core simulation | `./scripts/simulate_v2.sh --suite=core` | network/packet 回归通过 | 通过 | ✓ |
| diff format | `git diff --check` | 无空白/patch 格式问题 | 通过 | ✓ |
| v4 config scan | `rg ... xc/ws63_team_network scripts/ws63_build_v4_ubuntu.sh` | UART=21/22、ST7789、SPI master、WebUI 配置可见 | 通过 | ✓ |
| Ubuntu cross build | `scripts/ws63_build_v4_ubuntu.sh unified` | 生成 v4 统一固件包 | 通过（Tailscale 100.91.84.124） | ✓ |

## Error Log
| Timestamp | Error | Attempt | Resolution |
|-----------|-------|---------|------------|
| 2026-05-19 | `python` 命令不存在 | 1 | 改用 `python3` |
| 2026-05-19 | macOS 本地 SDK 头链检查缺 `platform_core.h`、`gpio_porting.h`、`dma_porting.h` 等 | 1 | 这是本地 include 环境限制，实际板级验证走 Ubuntu 交叉构建 |
| 2026-05-19 | `ssh: connect to host 192.168.6.5 port 22: Operation timed out` | 1 | 改走 Tailscale 主机 `100.91.84.124`，编译通过 |

## 5-Question Reboot Check
| Question | Answer |
|----------|--------|
| Where am I? | Phase 5 |
| Where am I going? | 上板验证与现场联调 |
| What's the goal? | 做出 WS63 + ST7789 的 v4 组网版本，并支持失联上报最后位置 |
| What have I learned? | 见 `findings.md` |
| What have I done? | 见上面的动作记录 |
