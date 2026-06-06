# Alpha6 候选：`team_conn_track_t.addr` 精简评估

## 背景

在 `v3.0.0-alpha5` 已移除 `bucket` / `parent_selected_id` / `routeHintLastActivityS` 后，`team_conn_track_t.addr` 仍保留。该字段当前不直接参与路由收敛与 parent 选择决策路径。

目标：评估在不降低可观测性的前提下移除 `addr`，进一步压缩 RAM 占用。

## 预期收益

- 粗略估算：`sle_addr_t` 约 7B 级别（按平台 ABI 可能对齐更高）；
- `TEAM_CONN_TRACK_MAX = 16`，理论节省约 `112B + 对齐收益`。

## 风险

1. 连接方向推断依赖链：
- `team_conn_guess_direction_from_addr(...)`
- `team_conn_should_use_client(...)`
- `team_connection_state_changed_cbk(...)`

2. pending 映射路径：
- `team_pending_conn_find(addr)`
- `team_pending_conn_clear(addr)`

3. 诊断可追溯性：
- 即使当前日志不直接输出 `addr`，删除后会减少后续定位“连接重映射异常”的上下文证据。

## 建议实施策略（TDD + Systematic Debugging）

1. 先补测试（红灯）：
- 覆盖 member 场景下连接状态切换时 client/server 方向判定不变；
- 覆盖 pending 命中后 route_id 绑定与清理行为不变。

2. 引入最小替代观测：
- 若移除 `addr`，保留 `conn_id + route_id + dir + 最近事件时间` 的诊断输出；
- 必要时增加调试开关，不把观测字段常驻主路径结构体。

3. 最小改动实施：
- 仅移除 `team_conn_track_t.addr`，其余逻辑先不重构；
- 若测试揭示方向推断依赖地址强关联，则保留字段并回退该优化。

4. 回归门槛：
- `cc ... team_network_demo ...` 通过；
- `./scripts/sim/simulate_v2.sh --suite=failover --stress=10` 通过；
- `python3 tools/sle_team_python_sim.py ... --stress 10` 通过。
