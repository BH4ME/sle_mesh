# Version v3.0.0-alpha6

版本定位：

- 在 `v3.0.0-alpha5` 的连接跟踪精简基础上，继续移除 `team_conn_track_t.addr` 冗余字段，进一步降低 WS63 runtime RAM 占用，同时保持 parent/route 行为路径不变。

本版完成：

1. 连接跟踪字段进一步精简：
- 删除 `team_conn_track_t.addr`；
- `team_conn_track_update()` 去除 `addr` 到 track 的复制；
- 保留 `team_pending_conn_t.addr` 作为 pending 命中与清理的主键，不改变 pending -> route_id 绑定逻辑。

2. 合同测试补强（TDD）：
- `webui/tests/ws63-api-contract.test.mjs` 新增断言：
  - `team_conn_track_t` 不再包含 `sle_addr_t addr`；
  - `team_pending_conn_t` 仍保留 `sle_addr_t addr`。

3. 稳定性回归：
- WebUI 测试与构建通过；
- C 侧 demo 回归通过；
- `simulate_v2` failover 压测通过；
- Python 仿真压力回归通过（`pass=10 fail=0`）。
