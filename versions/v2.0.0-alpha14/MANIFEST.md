# v2.0.0-alpha14 Manifest

## 变更范围

- `src/sle_team_node.c`
- `examples/team_network_demo.c`
- `versions/README.md`
- `versions/v2.0.0-alpha14/VERSION.md`
- `versions/v2.0.0-alpha14/MANIFEST.md`

## 关键改动

1. `pairing_stop` 一致性修复：
- approve pending 时收集失败结果；
- 不再无条件 `memset(pending_members)`；
- 有失败时保留 pending 并返回错误。

2. 回归验证：
- 新增 stop 场景单次发送失败注入；
- 断言 stop 返回失败且 pending 条目仍在。

## 验证

```sh
cc -Wall -Werror -Iinclude examples/team_network_demo.c src/sle_team_packet.c src/sle_team_node.c src/sle_team_web_api.c -DSLE_TEAM_NETWORK_TEST -o /tmp/sle_team_network_test
/tmp/sle_team_network_test

cc -Wall -Werror -Iinclude examples/team_node_common.c src/sle_team_packet.c src/sle_team_node.c -DSLE_TEAM_PACKET_TEST -o /tmp/sle_team_packet_test
/tmp/sle_team_packet_test
```

结果：通过。
