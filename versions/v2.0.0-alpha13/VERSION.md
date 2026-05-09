# Version v2.0.0-alpha13

版本定位：

- V2 第十三次增量（alpha）：修复 pairing approve 与 HELLO 路径在“发送失败窗口期”下的状态一致性问题，避免前端显示已批准但消息未送达导致的体验偏差。

本版完成：

1. approve 发送失败不再清 pending：
- `sle_team_node_pairing_approve_with_relay` 只有在 `CONFIG` 与 `ACK` 都发送成功时才清除 `pending`；
- 若任一发送失败，保留 `pending`，并记录失败日志，便于后续重试与可观测性。

2. leader 处理已允许 member 的 HELLO 时，改为“先发后清”：
- leader 在 `HELLO` 路径中先发送 `CONFIG/ACK`；
- 仅在 `ACK` 发送成功后清除 pending 并触发 joined 事件；
- 避免“先清 pending，再发送失败”导致的状态跳变。

3. 回归测试增强：
- `examples/team_network_demo.c` 新增“approve 单次发送失败”注入场景；
- 断言发送失败后 pending 仍保留，避免逻辑回归。
