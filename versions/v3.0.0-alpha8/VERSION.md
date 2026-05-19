# Version v3.0.0-alpha8

版本定位：

- 在 `v3.0.0-alpha7` 基线上，继续修复二次审查指出的协议解包与 relay-discovery 行为问题，并补齐更明确的版本/语义契约说明。

本版完成：

1. 协议解包安全性修复：
- `sle_team_handle_route_update()` 改为 `memcpy` 解包，避免指针强转带来的对齐风险；
- `sle_team_handle_alert()` 同步改为 `memcpy` 解包，和其它 handler 保持一致。

2. 路由更新语义收紧：
- 增加 `SLE_TEAM_ROUTE_UPDATE_FLAG_RELAY_GRANT`；
- leader 侧发送 `ROUTE_UPDATE` 时，仅在需要同步 relay 授权时置位；
- member 侧仅在明确收到该标志时刷新 `relay_enabled`。

3. relay discovery-only 行为修正：
- 广播包在 `relay_discovery_only` 节点上的本地处理增加过滤；
- 避免 HEARTBEAT/CONFIG 等非发现类广播触发不应有的状态更新。

4. 维护性与可读性：
- 提炼 relay tier 常量 `SLE_TEAM_MAX_RELAY_TIERS`；
- 为 GPS `fix_status` 增加有效性约定注释；
- 增加合同测试锁定上述行为。

5. 回归验证：
- WebUI 合同测试通过；
- C 侧 demo 回归通过。
