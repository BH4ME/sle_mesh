# v3.0.0-alpha6 Manifest

## 变更范围

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/tests/ws63-api-contract.test.mjs`
- `versions/README.md`
- `docs/v3/README.md`
- `versions/v3.0.0-alpha6/VERSION.md`
- `versions/v3.0.0-alpha6/MANIFEST.md`

## 关键改动

1. `team_conn_track_t.addr` 精简：
- 删除连接跟踪结构中的 `sle_addr_t addr` 字段；
- 删除 `team_conn_track_update(...)` 中对该字段的写入；
- `route_id` 绑定继续通过 `pending(addr)` 命中和 `team_route_id_from_sle_addr(...)` 路径完成，行为保持一致。

2. 边界断言测试：
- 新增合同测试锁定结构体边界：
  - `team_conn_track_t` 不含 `sle_addr_t addr`；
  - `team_pending_conn_t` 仍含 `sle_addr_t addr`，避免误删 pending 主键。

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
