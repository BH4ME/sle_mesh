# v2.0.0-alpha10 Manifest

## 变更范围

- `include/sle_team_web_api.h`
- `src/sle_team_web_api.c`
- `examples/team_network_demo.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `docs/v2/networking-goal.md`
- `versions/README.md`
- `versions/v2.0.0-alpha10/VERSION.md`
- `versions/v2.0.0-alpha10/MANIFEST.md`

## 关键改动

1. Web route metrics 结构扩展：
- `sle_team_web_route_metrics_t` 新增：
  - `route_update_rx_total`
  - `route_reparent_total`
  - `route_reparent_last_s`

2. Status JSON 输出扩展：
- `sle_team_web_write_status_json(...)` 在 `routeMetrics` 中新增：
  - `routeUpdateRxTotal`
  - `routeReparentTotal`
  - `routeReparentLastS`

3. leader 运行时观测落地：
- 新增 `team_route_update_observe(...)`；
- 在 `team_bind_packet_source(...)` 解包后接入观测；
- 统计 ROUTE_UPDATE 接收总量与 next-hop 重挂载次数/最近时间。

4. 页面展示增强：
- status 页面新增 route update/reparent 三项指标。

5. 测试（TDD 竖切）：
- `examples/team_network_demo.c` 新增 status JSON 断言，验证新增字段输出。

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
