# v3.0.0-alpha5 Manifest

## 变更范围

- `webui/src/main.ts`
- `webui/tests/ws63-api-contract.test.mjs`
- `webui/src/protocol/types.ts`
- `webui/README.md`
- `include/sle_team_web_api.h`
- `src/sle_team_web_api.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `examples/team_network_demo.c`
- `versions/README.md`
- `docs/v3/README.md`
- `versions/v3.0.0-alpha5/VERSION.md`
- `versions/v3.0.0-alpha5/MANIFEST.md`

## 关键改动

1. 串口模式 factory reset 可见引导：
- 串口模式下禁用 `Factory reset` 按钮；
- 增加明确提示文案，避免点击后才报错。

2. route hint 输出链路精简：
- 删除 `routeHintLastActivityS` 字段（runtime -> web api -> webui type）；
- 更新 JSON 序列化、示例文档、合同测试与 C 侧断言。

3. 路由/连接追踪冗余字段精简：
- 删除 `team_conn_track_t.bucket`；
- 删除 `team_pending_conn_t.bucket`；
- 删除 `g_team_rt.parent_selected_id`；
- 不改变路由收敛与切换决策行为。

4. 运维提醒：
- README 增加非隔离网络场景下外部门户 URL 轮换要求。

## 验证

```sh
npm --prefix webui test
npm --prefix webui run build

cc -std=c99 -Wall -Wextra -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

./scripts/simulate_v2.sh --suite=failover --stress=10

python3 tools/sle_team_python_sim.py \
  --members 40 --direct-cap 8 --relay-target 5 --ticks 30 \
  --batch-fail-relay-count 5 --batch-fail-relay-ticks "8" --stress 10
```
