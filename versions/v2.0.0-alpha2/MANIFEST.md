# v2.0.0-alpha2 Manifest

## 变更范围

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/sle_uart_server/sle_uart_server.h`
- `xc/ws63_team_network/sle_uart_server/sle_uart_server.c`
- `versions/v2.0.0-alpha2/VERSION.md`
- `versions/v2.0.0-alpha2/MANIFEST.md`

## 关键改动

1. member 自动选父：
- 增加 `team_member_autoselect_parent()`，在 member 侧从可用候选连接选更优 parent；
- 引入切换冷却与 RSSI 滞回（减少抖动）；
- 切换后自动上报 `ROUTE_UPDATE` 给 leader。

2. upstream disconnect 判定修复：
- 断连回调里仅在“当前 parent 断开”时触发 leave/reselect；
- 非 parent 链路断开不再导致整机误离队。

3. server 连接管理接口补齐：
- `sle_uart_server_get_active_conns`
- `sle_uart_server_get_conn_member`
- `sle_uart_server_get_conn_rssi`
- `sle_uart_server_disconnect_conn`

4. 审查回合定向修复：
- `team_upstream_parent_note()`：
  - 下行分支读取 client conn member 时改为临时变量，避免覆写入参 `parent_id`。
- `team_member_autoselect_parent()`：
  - 去除 `relay_allowed` / `relay_client_started` 前置限制，leaf 与 relay 统一走自动选父路径。

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
- `scripts/run_review_with_service.sh` 本地执行受当前 API Key 状态影响（本轮返回 authentication invalid），需在可用密钥环境重跑以更新 `review_feedback.md`。
