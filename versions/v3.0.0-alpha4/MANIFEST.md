# v3.0.0-alpha4 Manifest

## 变更范围

- `src/sle_team_node.c`
- `examples/team_network_demo.c`
- `versions/README.md`
- `docs/v3/README.md`
- `versions/v3.0.0-alpha4/VERSION.md`
- `versions/v3.0.0-alpha4/MANIFEST.md`

## 关键改动

1. parent 切换失败兜底修复：
- `sle_team_node_try_parent_switch()` 仅在 `HELLO` 发送成功后清空 `upstream_parent_id`；
- 发送失败时保留旧 parent，避免 `joined=1 && upstream_parent_id=0` 悬挂态；
- 后续 `tick` 保持可重试路径。

2. 测试补强：
- `team_network_demo` 增加“HELLO 首次发送失败，再次重试成功”的断言；
- 覆盖状态保持（`joined` / parent / `last_parent_seen_s`）与重试发包行为。

## 验证

```sh
cc -std=c99 -Wall -Wextra -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

./scripts/simulate_v2.sh --suite=failover --stress=10

python3 tools/sle_team_python_sim.py \
  --members 40 --direct-cap 8 --relay-target 5 --ticks 30 \
  --batch-fail-relay-count 5 --batch-fail-relay-ticks "8" --stress 10
```
