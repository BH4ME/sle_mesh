# Version v3.0.0-alpha4

版本定位：

- 在 `v3.0.0-alpha3` 的故障恢复链路上补齐 parent 切换发送失败边缘场景，消除 member 侧潜在悬挂态。

本版完成：

1. 修复 `try_parent_switch` 发送失败悬挂：
- `sle_team_node_try_parent_switch()` 调整为“先发 `HELLO`，后清 `upstream_parent_id`”；
- 若 `HELLO` 发送失败，则保留旧 parent 与 `last_parent_seen_s`，下一次 `tick` 自动重试；
- 发送成功后再进入“无 parent 等待 leader 重新分配”的状态。

2. 补充回归用例：
- `examples/team_network_demo.c` 新增 parent-switch 首次发送失败场景；
- 验证失败时不清 parent、不掉 joined，恢复发送后可再次触发 `HELLO` 并完成轻量切换流程。

3. 稳定性验证：
- C 侧 demo 回归通过；
- `simulate_v2` failover 压测通过；
- Python 仿真压力回归通过（`lost_parent=0`）。
