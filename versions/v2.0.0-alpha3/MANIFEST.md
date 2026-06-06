# v2.0.0-alpha3 Manifest

## 变更范围

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `versions/README.md`
- `versions/v2.0.0-alpha3/VERSION.md`
- `versions/v2.0.0-alpha3/MANIFEST.md`

## 关键改动

1. `team_upstream_parent_note()`：
- 下行分支读取 client conn member 使用临时变量，避免覆写 `parent_id` 入参。

2. `team_member_autoselect_parent()`：
- 去除 relay 专属限制，leaf 也参与自动选父。
- 继续使用 RSSI 候选阈值、切换冷却和滞回阈值。

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

补充说明：
- `scripts/run_review_with_service.sh` 本地执行仍受当前 API key 状态影响（authentication invalid），待可用密钥环境重跑并覆盖 `review_feedback.md`。
