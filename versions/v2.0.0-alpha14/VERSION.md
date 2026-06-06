# Version v2.0.0-alpha14

版本定位：

- V2 第十四次增量（alpha）：补齐 pairing stop 场景下的失败一致性缺口，确保“发送失败保留 pending 待重试”策略在窗口关闭路径同样成立。

本版完成：

1. pairing_stop 失败保留策略：
- 关闭 pairing window 时，逐个 approve pending；
- 若某成员 approve 发送失败，不再被全量清空 pending；
- 保留 pending 供后续重连/重试，避免“UI 已操作但目标成员丢失”的矛盾状态。

2. pairing_stop 返回值语义增强：
- 若存在 approve 失败，`sle_team_node_pairing_stop` 返回失败码（沿用最后一次错误）；
- 同步日志输出 `pairing stopped with pending retry`，便于上层识别部分失败。

3. 回归测试增强：
- `examples/team_network_demo.c` 新增 stop 场景 fail-once 注入，验证 stop 失败后 pending 仍保留。
