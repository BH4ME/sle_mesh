# V4 文档入口（WS63 模块 + ST7789 + 节点失联上报）

`docs/v4/` 记录从 `v3.0.0-alpha8` 演进到 V4 的第一阶段能力：

- WS63 模块引脚按新原理图重映射
- 1.14 寸 ST7789 显示接入（角色、在线数、失联提示）
- 组网主流程保持不变
- 子节点超时失联时向主节点上报并保留最后位置

版本命名说明：

- 当前对外交付版本是 `v4.3`。

## 当前里程碑

- `v4.3`：
  - 基于原理图固定 `显示/ST7789 + 蜂鸣器 + WS 灯 + GPS` 四项映射；
  - 合并烧录自动复位链路与 v4 WebUI 连通性修复；
  - 作为本阶段对外交付标签版本。
- `v4.2.3`：
  - 按原理图再次确认并固定 `显示/ST7789 + 蜂鸣器 + WS 灯 + GPS` 四项引脚映射；
  - GPS 映射进入 Kconfig 与启动日志，便于现场核对接线；
  - 下载电路相关逻辑未改动（按“未焊接下载电路”要求保持不动）。
- `v4.2.2`：
  - 恢复 `GET /api/location`；
  - `/pairing` 增加 `Phone Location`（手动发送、单次 GPS、自动持续上报）；
  - `/api/location` 返回补充 `ret/reason`，便于定位 `NOT_READY_OR_NO_ROUTE / WRITE_FAIL`。
- `v4.2.1`：
  - 固定 `v4.2` 线稳定基线（统一固件 + 运行时角色切换 + v4.1 板级约束）。
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
