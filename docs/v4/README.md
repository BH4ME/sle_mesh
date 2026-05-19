# V4 文档入口（WS63 模块 + ST7789 + 节点失联上报）

`docs/v4/` 记录从 `v3.0.0-alpha8` 演进到 V4 的第一阶段能力：

- WS63 模块引脚按新原理图重映射
- 1.14 寸 ST7789 显示接入（角色、在线数、失联提示）
- 组网主流程保持不变
- 子节点超时失联时向主节点上报并保留最后位置

## 当前里程碑

- `v4.0.0-alpha1`：
  - 默认 UART 改为 `IO21/IO22`
  - 禁用旧 `GPIO2` 活动灯（`CHRG` 占用）
  - ST7789 SPI 驱动接入与启动显示
  - 超时失联 `ALERT_TIMEOUT` 闭环补齐
  - 远端 Ubuntu 编译脚本 `scripts/ws63_build_v4_ubuntu.sh`

## 工作记录

- [task_plan.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/v4/task_plan.md)
- [findings.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/v4/findings.md)
- [progress.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/v4/progress.md)

## 代码位置

- V4 上板工程统一维护在：
  - [xc/ws63_team_network](/Users/bh4me_macair/Documents/Codex/sle_intercom/xc/ws63_team_network)

## 版本管理

1. 每个版本必须包含 `versions/<version>/VERSION.md` 与 `MANIFEST.md`。
2. 每次发布必须更新 `versions/README.md` 顶部“当前版本”列表。
3. 每次发布至少执行一轮可复现验证（协议测试 / 构建 / 上板）。
