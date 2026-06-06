# v2.0.0-alpha12 Manifest

## 变更范围

- `src/sle_team_node.c`
- `examples/team_network_demo.c`
- `review_feedback.md`
- `versions/README.md`
- `versions/v2.0.0-alpha12/VERSION.md`
- `versions/v2.0.0-alpha12/MANIFEST.md`

## 关键改动

1. ACK 入网安全校验：
- 在 `sle_team_handle_ack(...)` 对 `HELLO ACK` 增加 `status_code` 判定；
- 拒绝状态码不再触发 `joined`/`ONLINE` 状态切换。

2. CONFIG 处理边界收紧：
- 在 `sle_team_handle_config(...)` 增加角色与来源校验；
- 非 member 或来源非 `leader_id` 的 CONFIG 直接拒绝。

3. 心跳发送节流修复：
- 在 `sle_team_node_tick(...)` 增加 `heartbeat_interval_s != 0` 保护；
- 防止 0 周期配置导致高频刷包。

4. 回归断言：
- 在 `examples/team_network_demo.c` 增加三组断言覆盖上述修复点。

## 验证

```sh
cc -Wall -Werror -Iinclude examples/team_network_demo.c src/sle_team_packet.c src/sle_team_node.c src/sle_team_web_api.c -DSLE_TEAM_NETWORK_TEST -o /tmp/sle_team_network_test
/tmp/sle_team_network_test

cc -Wall -Werror -Iinclude examples/team_node_common.c src/sle_team_packet.c src/sle_team_node.c -DSLE_TEAM_PACKET_TEST -o /tmp/sle_team_packet_test
/tmp/sle_team_packet_test
```

结果：通过。
