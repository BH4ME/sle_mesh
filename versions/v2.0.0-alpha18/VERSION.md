# Version v2.0.0-alpha18

版本定位：

- V2 第十八次增量（alpha）：修复 SLE 多跳/配对状态机里“发送失败但状态已提交”的几个高风险路径。

本版完成：

1. 广播转发失败不阻塞本地处理：
- relay 转发 leader 广播失败时只记录错误；
- 本节点仍继续处理广播心跳/配置；
- 避免 relay 因空口转发失败误判 leader 超时并重加入。

2. HELLO 入网状态提交后移：
- leader 只有在 CONFIG 与 ACK 都发送成功后才提交成员入网；
- CONFIG/ACK 任一失败会回滚新成员记录；
- 避免应用层收到 joined 回调但成员配置没有真正发出。

3. pairing stop hidden mode 可重试：
- pairing stop 会检查 relay CONFIG 刷新结果；
- 清除 relay discovery-only/hidden mode 的 CONFIG 发送失败时返回错误；
- 后续再次 stop 可以继续重发配置，避免 relay 永驻 hidden mode。

4. member 成员表污染修复：
- member 收到 leader 心跳只刷新 leader-seen 超时基准；
- 不再把 leader 写入 `members[]`，避免应用层成员查询被误导。
