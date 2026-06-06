# v3.0.0-alpha8 Manifest

## 变更范围

- `src/sle_team_node.c`
- `include/sle_team_packet.h`
- `include/sle_team_node.h`
- `examples/team_network_demo.c`
- `webui/tests/ws63-api-contract.test.mjs`
- `versions/README.md`
- `docs/v3/README.md`
- `versions/v3.0.0-alpha8/VERSION.md`
- `versions/v3.0.0-alpha8/MANIFEST.md`

## 关键改动

1. 路由更新与 alert 解包：
- 将 `sle_team_handle_route_update()` / `sle_team_handle_alert()` 改为 `memcpy` 解包；
- 避免对齐敏感平台上的指针强转风险。

2. relay 授权语义：
- 新增 `SLE_TEAM_ROUTE_UPDATE_FLAG_RELAY_GRANT`；
- `sle_team_node_send_route_update()` 在 leader 侧按需置位；
- `sle_team_handle_route_update()` 仅在收到该标志时同步 `relay_enabled`。

3. discovery-only 过滤：
- relay discovery-only 节点忽略非发现类本地广播；
- 保持 HELLO / ROUTE_UPDATE 的发现与拓扑恢复能力。

4. 结构与文档：
- `SLE_TEAM_MAX_RELAY_TIERS` 提炼为命名常量；
- GPS `fix_status` 增加有效性注释；
- 更新版本索引和 V3 文档入口。

## 验证

```sh
npm --prefix webui test

cc -std=c99 -Wall -Wextra -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test
```
