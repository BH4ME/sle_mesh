# v2.0.0-alpha6 Manifest

## 变更范围

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `docs/v2/networking-goal.md`
- `versions/README.md`
- `versions/v2.0.0-alpha6/VERSION.md`
- `versions/v2.0.0-alpha6/MANIFEST.md`

## 关键改动

1. 时间窗口比较回绕安全：
- 新增 `team_elapsed_s()`、`team_interval_not_reached()`、`team_elapsed_exceeds()`；
- 用统一辅助函数替换以下窗口/超时逻辑中的直接减法比较：
  - route metrics 周期；
  - parent switch 冷却；
  - leader rescan 周期；
  - pairing rotate 周期；
  - relay rebalance 周期；
  - route stale 判定；
  - relay candidate 超时过滤；
  - relay stale 回收判定。

2. pending 连接淘汰策略与回绕一致性：
- 满表时“最旧条目淘汰”改为基于 `team_elapsed_s(now, last_seen)` 比较，避免回绕附近排序失真。

3. 地址派生 route id fallback 增强：
- `team_route_id_from_sle_addr()` 从单字节推导改为 6 字节混合推导，降低 route id 冲突概率。

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
