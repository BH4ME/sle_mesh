# Version v4.2.1

版本定位：

- `v4.2.1` 作为当前 `v4.2` 线的稳定记录点，冻结“统一固件 + 角色运行时切换 + v4.1 板级约束”的基线状态。

本版记录：

1. 固件与角色模型：
- 统一 `.fwpkg` 继续沿用，`leader/member` 通过 WebUI 或串口在运行时配置。
- 角色、team、leader suffix、channel 继续走 NV 持久化，重启自动恢复。

2. v4.1 板级约束保持：
- WS2812（`IO0`）和蜂鸣器（`IO14`）默认保持禁用态。
- ST7789 状态显示路径保持可用。

3. Web 控制台：
- 板端仍采用 C 端 SSR 页面，主路径为 `/`、`/nodes`、`/events`、`/pairing`。
- 保持轻量 HTTP API：`/api/status`、`/api/nodes`、`/api/events`、`/api/pending`、角色与配队控制接口。

回归说明：

- 该版本用于“现状落档”，作为 `v4.2.2`（手机位置自动持续上报）的前序基线。
