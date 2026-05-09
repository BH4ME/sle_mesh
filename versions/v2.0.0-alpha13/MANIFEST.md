# v2.0.0-alpha13 Manifest

## 变更范围

- `src/sle_team_node.c`
- `examples/team_network_demo.c`
- `versions/README.md`
- `versions/v2.0.0-alpha13/VERSION.md`
- `versions/v2.0.0-alpha13/MANIFEST.md`

## 关键改动

1. approve 行为一致性修复：
- `sle_team_node_pairing_approve_with_relay(...)` 调整为成功发送后再清 pending；
- `CONFIG` 或 `ACK` 任一发送失败时，pending 保留并返回失败。

2. HELLO 处理一致性修复：
- leader 在 `sle_team_handle_hello(...)` 中先发送 `CONFIG/ACK`；
- 仅 ACK 成功时清 pending 与上报 joined。

3. 回归测试：
- 新增 send fail-once 注入函数；
- 新增 approve 失败后 pending 保留断言。

## 验证

```sh
cc -Wall -Werror -Iinclude examples/team_network_demo.c src/sle_team_packet.c src/sle_team_node.c src/sle_team_web_api.c -DSLE_TEAM_NETWORK_TEST -o /tmp/sle_team_network_test
/tmp/sle_team_network_test

cc -Wall -Werror -Iinclude examples/team_node_common.c src/sle_team_packet.c src/sle_team_node.c -DSLE_TEAM_PACKET_TEST -o /tmp/sle_team_packet_test
/tmp/sle_team_packet_test
```

结果：通过。
