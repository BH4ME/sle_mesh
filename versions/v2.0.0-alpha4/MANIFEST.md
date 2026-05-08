# v2.0.0-alpha4 Manifest

## 变更范围

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `docs/v2/networking-goal.md`
- `versions/v2.0.0-alpha4/VERSION.md`
- `versions/v2.0.0-alpha4/MANIFEST.md`

## 关键改动

1. leader relay 自动重平衡：
- 新增 `team_leader_rebalance_relays()`；
- 关闭 pairing 后按周期执行，基于在线成员数计算 relay target（0/2/3）；
- relay 不足时自动挑选候选并提升。

2. relay 授权自动下发与回收：
- 新增 `team_leader_set_member_relay_allowed()`，统一处理提升/回收和 CONFIG 下发；
- offline/stale relay 自动回收并打印原因日志。

3. 路由收敛清理：
- 新增 `team_route_clear_by_next_hop()`；
- relay 回收时清理依赖该 next-hop 的路由项，避免陈旧路径。

4. Web/日志可观测性：
- 状态页增加 `Relay Target`/`Relay Online`；
- 新增 `relay set/revoke/rebalance` 日志。

## 验证

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test && /tmp/sle_team_packet_test
```

结果：通过。

补充：
- 审查执行建议：`scripts/run_review_with_deepseek.sh --scope "V2 alpha4"`，输出覆盖根目录 `review_feedback.md`。
