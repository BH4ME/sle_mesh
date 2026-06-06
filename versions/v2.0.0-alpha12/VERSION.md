# Version v2.0.0-alpha12

版本定位：

- V2 第十二次增量（alpha）：修复入网/配置/心跳三个关键逻辑缺陷，并补齐对应回归断言，提升组网鲁棒性与安全边界。

本版完成：

1. HELLO ACK 入网判定修复：
- member 处理 `SLE_TEAM_APP_HELLO` 的 ACK 时，新增 `status_code` 成功校验；
- 仅当 `status_code == 0` 时才置 `joined=1`，避免拒绝 ACK 误入网。

2. CONFIG 来源与角色校验修复：
- `CONFIG` 处理路径新增角色与来源限制；
- 仅 member 接收且仅接受来自当前 `leader_id` 的 `CONFIG`，防止 leader 被错误配置覆盖。

3. heartbeat=0 刷包修复：
- `sle_team_node_tick` 发送心跳前新增 `heartbeat_interval_s != 0` 判定；
- 将 0 间隔语义视为“禁用周期心跳”，避免每 tick 都发包。

4. 回归测试补齐：
- `examples/team_network_demo.c` 新增断言覆盖：
  - HELLO ACK 失败状态不入网、成功状态可入网；
  - fake member 向 leader 发送 CONFIG 不会改写 leader 配置；
  - `heartbeat_interval_s=0` 时 tick 不发心跳。
