# Version v2.0.0-alpha2

版本定位：

- V2 第二次增量修复（alpha）：统一 `pairing stop` 在不同入口的自动审批行为，消除 CLI/WebUI 路径差异。

本版完成：

1. 统一 pairing stop 语义（核心修复）：
- 在 `sle_team_node_pairing_stop()` 内部增加 pending 自动审批流程。
- 默认审批策略为 `relay_allowed=0`，保持“relay 权限需显式授予”的模型。

2. 兼容现有 WebUI 自动 relay 授权流程：
- WebUI 入口仍可在 stop 前先执行策略化审批（例如首批 relay 配额）。
- `pairing_stop()` 内部补齐后不会破坏既有流程，只用于兜底统一行为。

3. 结果：
- CLI 执行 `pairing stop` 时，不再出现 pending 成员被直接清空而未审批的问题。
- WebUI 与 CLI 对同一动作的结果保持一致。
