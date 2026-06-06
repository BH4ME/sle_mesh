# v3.0.0-alpha3 Manifest

## 变更范围

- `include/sle_team_node.h`
- `src/sle_team_node.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `examples/team_network_demo.c`
- `versions/README.md`
- `docs/v3/README.md`
- `versions/v3.0.0-alpha3/VERSION.md`
- `versions/v3.0.0-alpha3/MANIFEST.md`

## 关键改动

1. relay 掉线即时恢复：
- 新增 `sle_team_relay_offline_cb` 与 `ops.on_relay_offline`；
- leader 清理 stale relay 时立即通知上层；
- WS63 回调中直接触发 `team_leader_rebalance_relays()`。

2. member parent 健康检查补盲区：
- 新增 `parent_timeout_s`、`last_parent_seen_s`；
- 新增 `sle_team_node_try_parent_switch()`；
- `tick` 中在 parent 超时后触发轻量切换（不 full rejoin）。

3. parent 可达观测与参数调优：
- ACK/CONFIG/ROUTE_UPDATE 路径同步 parent 最近观测时间；
- WS63 `team_upstream_parent_note()` 同步刷新 `last_parent_seen_s`；
- `SLE_TEAM_RELAY_REBALANCE_INTERVAL_S: 8 -> 3`；
- `SLE_TEAM_PARENT_SWITCH_COOLDOWN_S: 10 -> 5`。

4. 回归测试补强：
- `team_network_demo` 新增 relay-offline 回调验证；
- 新增 parent-timeout 切换行为验证（状态保持 + HELLO 重选父）。

## 验证

```sh
cc -std=c99 -Wall -Wextra -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

./scripts/simulate_v2.sh --suite=failover --stress=10

python3 tools/sle_team_python_sim.py \
  --members 40 --direct-cap 8 --relay-target 5 --ticks 30 \
  --batch-fail-relay-count 5 --batch-fail-relay-ticks "8" --stress 10 --show-timeline

npm --prefix webui test
npm --prefix webui run build
```
