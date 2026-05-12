# v2.0.0-alpha18 Manifest

## 变更范围

- `src/sle_team_node.c`
- `examples/team_node_regression_test.c`
- `versions/README.md`
- `versions/v2.0.0-alpha18/VERSION.md`
- `versions/v2.0.0-alpha18/MANIFEST.md`

## 关键改动

1. relay 广播处理：
- 非目标广播包的转发失败不再直接返回错误；
- 广播仍进入本地 app handler，保持 leader 心跳/配置处理连续。

2. leader HELLO 处理：
- 成员表写入前保存旧记录；
- CONFIG 失败或 ACK 失败时恢复旧记录，或清除新建成员槽位；
- joined 回调只在两段确认都成功后触发。

3. relay CONFIG 刷新：
- `sle_team_leader_refresh_relay_config()` 返回最后一次发送错误；
- pairing stop 将 CONFIG 刷新失败作为可见错误返回，保留重试入口。

4. member 心跳处理：
- member 不再把 leader heartbeat 创建为普通 member record；
- 只刷新 leader 可见性时间，用于防止误超时。

## 验证

```sh
cc -Wall -Wextra -Werror -Iinclude \
  examples/team_node_regression_test.c src/sle_team_node.c src/sle_team_packet.c \
  -o /tmp/sle_team_regression_test && /tmp/sle_team_regression_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test && /tmp/sle_team_packet_test

cd webui
npm test
```
