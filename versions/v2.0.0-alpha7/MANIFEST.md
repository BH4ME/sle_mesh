# v2.0.0-alpha7 Manifest

## 变更范围

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `docs/v2/networking-goal.md`
- `versions/README.md`
- `versions/v2.0.0-alpha7/VERSION.md`
- `versions/v2.0.0-alpha7/MANIFEST.md`

## 关键改动

1. leader 主动 ROUTE_UPDATE 收敛提示：
- 新增 `team_leader_route_convergence_hint(...)`；
- 新增 `team_leader_route_hint_parent_for_member(...)`；
- 在 `team_leader_route_metrics_update()` 指标变化后触发主动提示。

2. 提示下发策略：
- 直接连到 leader 的 member：提示 parent=leader；
- 已存在 next-hop 路由的 member：提示 parent=next_hop；
- 无有效路由线索的 member：跳过，避免误导与无效发送。

3. 事件与日志：
- 新增 system event：`route hint sent=<n> fail=<n> st=<n> un=<n>`；
- 控制台日志输出 route hint 聚合结果与失败项。

## 验证

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test

/tmp/sle_team_network_test
/tmp/sle_team_packet_test
```

结果：通过。
