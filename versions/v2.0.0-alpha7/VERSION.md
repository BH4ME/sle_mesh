# Version v2.0.0-alpha7

版本定位：

- V2 第七次增量（alpha）：补齐 leader 侧“主动 ROUTE_UPDATE 收敛提示”闭环，使路由收敛从被动观测升级为“可提示、可驱动”。

本版完成：

1. leader 主动 ROUTE_UPDATE 收敛提示：
- 在 leader 路由指标变化（`routeMetrics` changed）时触发收敛提示流程；
- 面向在线成员按当前路由状态推导建议父节点，逐个下发 `ROUTE_UPDATE(parent=next_hop)`。

2. 提示触发策略：
- 常规触发：指标变化即触发；
- 重点场景：收敛状态切换、`stale/unreachable` 波动时触发；
- 对无可用提示对象时自动跳过，避免无意义发送。

3. 可观测性补齐：
- 新增 system event 摘要：`route hint sent=<n> fail=<n> st=<n> un=<n>`；
- 日志新增 route hint 结果统计，便于定位“已提示但未收敛”的链路问题。

4. 与现有能力兼容：
- 不改协议包结构；
- 保持 leader 唯一准入控制者模型不变；
- 与 alpha5/alpha6 的 routeMetrics 与回绕安全逻辑兼容。
