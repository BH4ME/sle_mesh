# Version v3.0.0-alpha3

版本定位：

- 在 `v3.0.0-alpha2` 之上聚焦组网恢复时延与 member 父节点容错，落实 `meta/recovery_optimization_spec.md` 三项改动并完成回归验证。

本版完成：

1. Leader 侧 relay 掉线即时重平衡：
- `sle_team_prune_stale_members` 在检测到 relay 成员离线时触发 `on_relay_offline` 回调；
- WS63 注册回调并立即执行 `team_leader_rebalance_relays()`，将新 relay 晋升等待从周期扫描缩短为事件触发。

2. Member 侧 parent 健康超时与轻量切换：
- 新增 `parent_timeout_s` 配置与 `last_parent_seen_s` 状态；
- 新增 `sle_team_node_try_parent_switch()`，在父节点超时后仅进入 reselect 并向 leader 发 `HELLO` 请求新 parent；
- 不做 full rejoin，不清 members 表，不撤销 relay permission，降低恢复抖动。

3. 参数调优与 parent 心跳观测补强：
- `SLE_TEAM_RELAY_REBALANCE_INTERVAL_S` 从 `8s` 调整到 `3s`；
- `SLE_TEAM_PARENT_SWITCH_COOLDOWN_S` 从 `10s` 调整到 `5s`；
- WS63 默认 `cfg.parent_timeout_s = heartbeat_timeout / 2`；
- WS63 在上游 parent 包观测路径刷新 `last_parent_seen_s`，避免健康链路下误触发 parent timeout。

4. 验证资产补充：
- `examples/team_network_demo.c` 增加 relay-offline 回调触发用例；
- 增加 member parent-timeout 轻量切换用例（保持 joined/online、保留 relay 标志与成员表、发起 `HELLO` 重新选父）。
