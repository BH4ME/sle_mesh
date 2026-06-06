# Version v2.0.0-alpha4

版本定位：

- V2 第四次增量（alpha）：补齐 leader 侧 relay 自动重选与授权闭环，完成 V2 自愈链路的剩余核心能力。

本版目标：

1. leader 自动补选 relay：
- 当在线 relay 数量低于阈值时，leader 自动从 approved + online 成员里选举新 relay。
- 选举输入至少包含：链路质量、稳定时长、当前负载。

2. relay 授权自动下发：
- 对选中节点自动下发 `relay_allowed=1` 配置；
- 支持 relay 回收（失效/不稳定）并可重新分配名额。

3. 路由自愈闭环打通：
- relay 失效后，leaf 自动重选父并上报 `ROUTE_UPDATE`；
- leader 侧快速更新 `member_id -> next_hop` 并触发拓扑收敛检查。

4. 准入模型保持不变：
- 仍保持 leader 为唯一 approve authority；
- 对已 approved 节点使用 fast rejoin，不要求人工二次 approve。

当前实现（本次提交）：

1. leader relay 池自动重平衡：
- 新增周期任务，在 leader 且 pairing 关闭时执行；
- 根据在线成员规模计算 relay target（0/2/3），自动补齐 relay 名额。

2. relay 自动授权下发：
- 从在线成员中按候选条件（在线、最近可见、RSSI）筛选并提升 relay；
- 提升/回收均通过 CONFIG 下发 `relay_allowed`，保持协议兼容。

3. relay 回收与路由清理：
- 对 offline/stale relay 自动回收；
- 回收时清理 `next_hop == relay_id` 的路由项，避免陈旧路径残留。

4. 可观测性：
- Web 状态页新增 `Relay Target` 与 `Relay Online`；
- 增加 `relay set/revoke/rebalance` 日志，便于实机跟踪。
