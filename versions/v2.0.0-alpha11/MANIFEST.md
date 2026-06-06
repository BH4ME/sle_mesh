# v2.0.0-alpha11 Manifest

## 变更范围

- `include/sle_team_packet.h`
- `include/sle_team_node.h`
- `src/sle_team_node.c`
- `examples/team_network_demo.c`
- `docs/v2/networking-goal.md`
- `versions/README.md`
- `versions/v2.0.0-alpha11/VERSION.md`
- `versions/v2.0.0-alpha11/MANIFEST.md`

## 关键改动

1. 协议标志位扩展（兼容）：
- 新增 `SLE_TEAM_CONFIG_FLAG_RELAY_DISCOVERY_ONLY`（复用 `sle_team_config_body_t.reserved`）；
- 不改变现有包结构与字段布局。

2. 节点配置能力扩展：
- `sle_team_node_cfg_t` 新增 `relay_discovery_only`；
- member 侧在 `CONFIG` 处理路径中解析并保存该状态。

3. relay 转发行为约束：
- 在 `sle_team_should_relay_packet(...)` 增加 discovery-only 白名单判定；
- 仅 `HELLO/ROUTE_UPDATE` 可转发，业务类 app msg 被抑制。

4. pairing 窗口切换闭环：
- 新增 leader 辅助刷新函数，在 `pairing start/stop` 时主动重发在线 relay 的 `CONFIG`；
- 确保隐藏 relay 模式按窗口生命周期自动切换。

5. 测试（TDD 竖切）：
- `examples/team_network_demo.c` 新增断言覆盖窗口开关标志传播与消息白名单转发行为。

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
