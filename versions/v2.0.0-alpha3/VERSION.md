# Version v2.0.0-alpha3

版本定位：

- V2 第三次增量（alpha）：针对审查反馈的定向收敛版本，确保自动选父逻辑与 V2 goal 一致。

本版完成：

1. 修复 parent 记录污染风险：
- `team_upstream_parent_note()` 在下行分支读取 client conn member 时改为临时变量，不再覆写函数入参 `parent_id`。
- 避免 parent 状态、日志与统计写入错误成员 ID。

2. 放开 leaf 自动选父：
- `team_member_autoselect_parent()` 去除 `relay_allowed` 与 `relay_client_started` 前置限制。
- relay/leaf 统一进入 RSSI 选父、滞回、冷却逻辑，和 V2 goal 中“member 自动选父”对齐。

3. 行为保持：
- 保持 next-hop 定向转发策略不变。
- 保持 leader approve 权限模型与 pairing stop 自动审批逻辑不变。
