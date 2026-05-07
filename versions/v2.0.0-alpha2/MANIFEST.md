# v2.0.0-alpha2 Manifest

## 变更范围

- `src/sle_team_node.c`
- `versions/v2.0.0-alpha2/VERSION.md`
- `versions/v2.0.0-alpha2/MANIFEST.md`
- `versions/README.md`

## 关键改动

1. 在 `sle_team_node_pairing_stop()` 增加 pending 自动审批：
- 先快照 pending member_id 列表；
- 逐个调用 `sle_team_node_pairing_approve_with_relay(..., relay_allowed=0)`；
- 再执行 `pairing_enabled=0` 与 pending 清理。

2. 语义对齐：
- CLI `pairing stop` 与 WebUI `pairing stop` 均具备“关闭前审批 pending”的行为一致性。
- relay 权限默认不自动授予，继续遵循显式授权策略。

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
