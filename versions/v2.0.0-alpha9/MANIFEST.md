# v2.0.0-alpha9 Manifest

## 变更范围

- `include/sle_team_web_api.h`
- `src/sle_team_web_api.c`
- `examples/team_network_demo.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `docs/v2/networking-goal.md`
- `versions/README.md`
- `versions/v2.0.0-alpha9/VERSION.md`
- `versions/v2.0.0-alpha9/MANIFEST.md`

## 关键改动

1. Web route metrics 结构扩展：
- `sle_team_web_route_metrics_t` 新增：
  - `hint_sent_total`
  - `hint_failed_total`
  - `hint_cooldown_skipped_total`
  - `hint_last_activity_s`

2. Status JSON 输出扩展：
- `sle_team_web_write_status_json(...)` 在 `routeMetrics` 中新增：
  - `routeHintSentTotal`
  - `routeHintFailedTotal`
  - `routeHintCooldownSkippedTotal`
  - `routeHintLastActivityS`

3. leader hint 计数落地：
- 新增 `team_route_hint_note_skip(...)`、`team_route_hint_note_send_result(...)`；
- 冷却命中、发送成功、发送失败分别累计并更新时间戳。

4. 页面展示增强：
- status 页面新增 route hint 四项指标展示。

5. 测试（TDD 竖切）：
- 在 `examples/team_network_demo.c` 新增 status JSON 断言，验证新增字段输出。

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
