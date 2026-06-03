# Version v4.0.0-alpha1

版本定位：

- 基于 `v3.0.0-alpha8`，切入 V4 硬件形态：WS63 模块 + 1.14 寸 ST7789。
- 保持“组网优先”的主线，补齐“子节点丢失上报最后位置”的闭环。

本版完成：

1. V4 引脚与默认参数落地：
- 默认调试串口改为 `U0TX/IO21` + `U0RX/IO22`。
- 禁用旧活动灯 `GPIO2`（V4 原理图中为 `CHRG`）。
- SoftAP 默认 SSID 前缀改为 `SLE-TEAM-V4`。

2. ST7789 显示接入：
- 新增 `ws63_st7789_display` 模块并接入构建。
- 启动阶段完成显示初始化，展示角色、在线节点和失联提示。
- 默认分辨率与偏移按 1.14 寸 `135x240`（`x=52,y=40`）配置。

3. 失联上报闭环：
- 在核心状态机中补齐普通 member 超时后的 `ALERT_TIMEOUT` 广播。
- alert 保留最后一次上报位置与最后时间戳，满足主节点追踪需求。

4. 编译交付链路：
- 新增 `scripts/ws63_build_v4_ubuntu.sh`。
- 支持通过 Tailscale 主机（如 `100.91.84.124`）远程编译并回传统一固件包。

5. 结构整理（本次重整）：
- V4 工作文档归档到 `docs/v4/`。
- V4 上板工程收敛到 `xc/ws63_team_network/`（不再保留根目录 `v4/` 临时工作区）。

回归结论：

- 协议/状态机本地回归通过。
- 远端 Ubuntu 交叉编译通过，产物成功回传：
  - `<sdk-root>/output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
