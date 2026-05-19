# Findings & Decisions

## Requirements
- 主控：WS63 模块
- 显示：1.14 寸 ST7789
- 主功能：组网
- 辅助功能：子节点丢失时向主节点上报，并保留最后位置
- 需要先给计划书，再开始改代码

## Research Findings
- 当前仓库里，WS63 组网样例主要在 `xc/ws63_team_network/`
- 当前 v3 基线是 `v3.0.0-alpha8`
- 现有工程已经有串口 CLI、WebUI、节点状态、位置上报和路由记录框架
- WS63 SDK 里有 SPI master 样例，使用 `uapi_spi_init` 和 `uapi_spi_master_write`
- v3 核心已有 `ALERT` 报文结构，缺口是普通 member 超时时没有主动构造 alert
- v4 的 ST7789 需要 `CONFIG_SPI_SUPPORT_MASTER=y`，否则 SPI master 写接口可能不参与编译/声明
- v4 WebUI/SoftAP 是运行时选择 leader/member 的主要入口，因此 `CONFIG_SLE_TEAM_WIFI_AP_ENABLE` 需要默认开启

## Visual/Browser Findings
- 原理图第 3 页里，WS63 模块可见信号包括：
  - `RGB -> IO0`
  - `CHRG -> IO2`
  - `BOOT -> IO3`
  - `ADC_CTRL -> IO5`
  - `SCL -> IO6`
  - `CS -> IO7`
  - `SDA -> IO8`
  - `RS -> IO9`
  - `ADC_VBAT -> IO12`
  - `RESET -> IO13`
  - `BUZZ -> IO14`
- 原理图第 4 页里，ST7789 FPC 可见信号包括：
  - `V3_3`
  - `SCL`
  - `SDA`
  - `RESET`
  - `RS`
  - `CS`
  - `GND`
- 原理图里没有看到独立的 `BL` 背光控制网，背光是否独立控制还要再确认
- v4 默认串口调试使用 `U0TX/IO21` 和 `U0RX/IO22`
- v4 的 GPS 模块接 `U1TX/IO17`、`U1RX/IO18`，当前仅保留硬件事实，未做 NMEA 解析

## Technical Decisions
| Decision | Rationale |
|----------|-----------|
| 先按原理图整理引脚，再改代码 | 先把硬件边界弄准，后面不会来回返工 |
| 保留组网主流程 | 你说工程主要做组网，不想把显示喧宾夺主 |
| 用 `v4/` 作为工作区 | 方便把计划、发现和后续实现集中起来 |
| ST7789 单独做 `ws63_st7789_display` 模块 | 让显示失败不影响组网核心 |
| 失联上报放在协议核心超时剪枝处 | 根因是超时剪枝只标离线不发 alert，从源头补最小逻辑 |
| ST7789 初始化放在 v4 主任务启动早期 | 开机即可显示 `idle` 状态，角色配置后刷新为 leader/member |
| ST7789 使用 6x8 ASCII 点阵 | 比占位伪字体可读，足够显示 ID、在线数、失联数、最后位置 |
| v4 默认打开 SoftAP/WebUI | 符合统一固件运行时选角色的使用方式 |
| v4 默认打开 SPI master | ST7789 走 SPI 写屏，不能依赖 SDK 默认关闭状态 |
| v4 SoftAP 前缀改成 `SLE-TEAM-V4` | 避免现场和 v3 固件混淆 |

## Issues Encountered
| Issue | Resolution |
|-------|------------|
| `python` 命令不可用 | 用 `python3` 替代 |
| macOS 本地 WS63 SDK 头链语法检查缺平台宏/porting include | 不作为板级结论，实际以 Ubuntu SDK 交叉构建为准 |
| Ubuntu 编译机 `192.168.6.5` SSH 超时 | 记录为环境阻塞；待主机可达后重跑 v4 构建脚本 |

## Resources
- `/Users/bh4me_macair/Desktop/SCH_Schematic1_2026-05-19.pdf`
- `/Users/bh4me_macair/Documents/Codex/sle_intercom/xc/ws63_team_network/README.md`
- `/Users/bh4me_macair/Documents/Codex/sle_intercom/xc/ws63_team_network/src/ws63_team_network_app.c`
- `/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v3.0.0-alpha8/VERSION.md`
- `/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v3.0.0-alpha8/MANIFEST.md`
- `/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/application/samples/peripheral/spi/spi_master_demo.c`
- `/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/application/samples/peripheral/spi/ssd1306.c`
