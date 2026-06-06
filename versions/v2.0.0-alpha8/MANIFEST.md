# v2.0.0-alpha8 Manifest

## 变更范围

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `docs/v2/networking-goal.md`
- `versions/README.md`
- `versions/v2.0.0-alpha8/VERSION.md`
- `versions/v2.0.0-alpha8/MANIFEST.md`

## 关键改动

1. route hint 成员级冷却机制：
- 新增缓存字段：`route_hint_member_ids[]`、`route_hint_parent_ids[]`、`route_hint_last_sent_s[]`；
- 新增辅助函数：
  - `team_route_hint_cache_acquire(...)`
  - `team_route_hint_should_send(...)`
  - `team_route_hint_mark_sent(...)`
- 冷却策略：同成员同 parent 在 `SLE_TEAM_ROUTE_HINT_COOLDOWN_S`（12s）内跳过重复下发。

2. 主动提示逻辑优化：
- `team_leader_route_convergence_hint(...)` 中将 `hint_parent_id` 改为循环内局部变量；
- 在成功与失败路径都刷新 hint 缓存时间，避免失败场景瞬时重试刷屏。

3. 常量新增：
- `SLE_TEAM_ROUTE_HINT_COOLDOWN_S 12U`

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
