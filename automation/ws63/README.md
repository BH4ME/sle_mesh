# WS63 Automation (Independent)

该目录用于承载 **WS63 自动化工具链**，与主程序代码、仿真核心代码、固件工程解耦管理。

## 目录结构

- `automation/ws63/tools/`
  - `ws63_auto_burn.py`：自动复位烧录工具（串口 `reboot` + DTR/RTS）。
  - `ws63_link_cycle_test.py`：串口 member 生命周期回归（重启自动恢复 + 手动 leave/rejoin）。
  - `ws63_flash_bind_team.py`：批量烧录并绑定角色（1 个 leader -> N 个 members）。
- `automation/ws63/scripts/`
  - `ws63_test_system.sh`：自动化总入口（单测 / 仿真 / 烧录 / role-bind / link-cycle）。
- `automation/ws63/tests/`
  - 对应工具链单测。

## 与其他代码边界

- 主程序代码：`src/`, `include/`, `xc/ws63_team_network/`
- 仿真核心代码：`tools/sle_team_python_sim.py`, `scripts/simulate_v2.sh`
- 固件构建与烧录入口：`scripts/ws63_build_team_*.sh`, `scripts/ws63_flash_team.sh`
- 自动化编排与验证：`automation/ws63/*`

## 兼容入口

为兼容已有命令，以下旧路径保留为薄封装入口：

- `tools/ws63_auto_burn.py`
- `tools/ws63_link_cycle_test.py`
- `tools/ws63_flash_bind_team.py`
- `tools/test_ws63_*.py`
- `scripts/ws63_test_system.sh`

建议新开发统一使用 `automation/ws63/*` 下的新路径。
