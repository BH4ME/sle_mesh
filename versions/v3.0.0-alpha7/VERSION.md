# Version v3.0.0-alpha7

版本定位：

- 在 `v3.0.0-alpha6` 基线上，修复两处状态一致性问题（配队审批回滚与 offline 即时重平衡），并补齐可维护性注释，保证行为语义更明确。

本版完成：

1. 配队审批失败回滚修复：
- `sle_team_node_pairing_approve_with_relay()` 增加 `had_allowed_before` 跟踪；
- 当 member 槽位申请失败（`SLE_TEAM_ERR_BUF`）且 allowlist 为本次新增时，执行 `sle_team_node_remove_allowed_member()` 回滚；
- 避免“审批失败但 allowlist 残留”的部分成功状态。

2. relay offline 即时重平衡生效：
- `team_leader_rebalance_relays()` 改为 `team_leader_rebalance_relays(uint8_t force_now)`；
- `team_on_relay_offline()` 走 `force_now=1` 路径，绕过 cooldown 立即执行；
- 主循环周期调度继续使用 `force_now=0`，保持原有节流策略。

3. 可维护性增强：
- 删除 `sle_team_node_pairing_approve_with_relay()` 中恒真守卫 `if (member != NULL)`；
- 为 `sle_team_node_try_parent_switch()` 增加 `SLE_TEAM_ERR_UNSUPPORTED` 语义注释；
- 在 `webui/tests/ws63-api-contract.test.mjs` 增加“结构契约测试对格式敏感”的顶部说明注释。

4. 回归验证：
- WebUI 合同测试通过；
- C 侧 demo 回归通过；
- `simulate_v2` failover 压测通过；
- Python 仿真压力回归通过（`pass=10 fail=0`）。
