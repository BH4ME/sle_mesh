# v3.0.0-alpha7 Manifest

## 变更范围

- `src/sle_team_node.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/tests/ws63-api-contract.test.mjs`
- `versions/README.md`
- `docs/v3/README.md`
- `versions/v3.0.0-alpha7/VERSION.md`
- `versions/v3.0.0-alpha7/MANIFEST.md`

## 关键改动

1. 配队审批回滚：
- `sle_team_node_pairing_approve_with_relay()` 在槽位分配失败时回滚本次新增 allowlist；
- 消除审批失败后的 allowlist 脏状态。

2. offline 事件强制重平衡：
- `team_leader_rebalance_relays()` 增加 `force_now` 参数；
- 回调 `team_on_relay_offline()` 使用 `force_now=1` 立即执行；
- 周期调度使用 `force_now=0` 保留 cooldown。

3. 代码与测试可维护性：
- 删除恒真守卫（`if (member != NULL)`）；
- 增加 `try_parent_switch` 返回码语义注释；
- 在合同测试文件顶部增加“结构契约测试”说明。

## 验证

```sh
npm --prefix webui test
npm --prefix webui run build

cc -std=c99 -Wall -Wextra -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

./scripts/simulate_v2.sh --suite=failover --stress=10

python3 tools/sle_team_python_sim.py \
  --members 40 --direct-cap 8 --relay-target 5 --ticks 30 \
  --batch-fail-relay-count 5 --batch-fail-relay-ticks "8" --stress 10
```
