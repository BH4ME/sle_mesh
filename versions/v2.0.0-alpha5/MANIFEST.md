# v2.0.0-alpha5 Manifest

## 变更范围

- `include/sle_team_web_api.h`
- `src/sle_team_web_api.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `versions/README.md`
- `versions/v2.0.0-alpha5/VERSION.md`
- `versions/v2.0.0-alpha5/MANIFEST.md`

## 关键改动

1. 路由收敛统计核心：
- 新增 leader 周期任务 `team_leader_route_metrics_update()`；
- 统计 `active/direct/relayed/unreachable/stale`，并计算 `converged/epoch/last_change/last_converged`。

2. Web API 指标输出：
- 扩展 `sle_team_web_write_status_json(...)` 入参，支持注入 `routeMetrics`；
- `/api/status` 返回体新增 `routeMetrics` 节点。

3. 页面与事件提示：
- status 页面新增 route 收敛相关字段；
- 指标变化时推送 system event 记录到事件环形缓冲。
- events 页面展示 system summary，便于直接查看收敛变化摘要。

4. 指标语义修正：
- `last_converged_s` 改为仅在“未收敛 -> 收敛”状态跃迁时更新。

## 验证

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test
```

结果：通过。
