# v2.0.0-alpha1 Manifest

## 变更范围

- `xc/ws63_team_network/sle_uart_client/sle_uart_client.h`
- `xc/ws63_team_network/sle_uart_client/sle_uart_client.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `docs/v2/networking-goal.md`
- `docs/v2/protocol.md`
- `versions/v2.0.0-alpha1/VERSION.md`
- `versions/v2.0.0-alpha1/MANIFEST.md`

## 关键改动

1. 新增 client 连接管理接口：
- `sle_uart_client_get_active_conns`
- `sle_uart_client_get_conn_member`
- `sle_uart_client_disconnect_conn`

2. leader pairing window 连接轮换：
- 周期轮换释放“未进入 pending/online”的连接，保留关键连接槽位。

3. pairing stop 自动审批与首批自动 relay 授权：
- 自动审批 pending；
- relay 授权按配额自动分配（当前默认上限3）。

4. 文档更新：
- `/goal v2` 固化至 `docs/v2/networking-goal.md`。
- 协议文档整理至 `docs/v2/protocol.md`（并在 v0/v1/v2 分线维护）。

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
