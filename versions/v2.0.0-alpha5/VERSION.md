# Version v2.0.0-alpha5

版本定位：

- V2 第五次增量（alpha）：补齐 leader 侧更强的路由收敛提示与指标输出，提升“可观测+可诊断”能力。

本版完成：

1. leader 路由收敛指标：
- 周期统计路由活跃数、直连/中继数、不可达数、陈旧数；
- 增加 `converged` 判定、`epoch` 版本号、`last_change_s/last_converged_s` 时间戳。

2. status JSON 指标扩展：
- `GET /api/status` 新增 `routeMetrics` 字段，包含上述收敛指标；
- 与现有字段兼容，不改协议包格式。

3. 状态页提示增强：
- `/` 页面新增 `Route Converged/Active/Direct/Relayed/Unreachable/Stale/Epoch`；
- 补充 `Route Last Change/Route Last Converged` 时间字段展示；
- 与 `Relay Target/Relay Online` 形成闭环观测。

4. 系统事件提示：
- 路由指标变化时写入 system event（events 页面可见），便于追踪拓扑波动。
- events 页面补充 system summary 展示，支持直接查看收敛变化摘要。

5. 收敛时间戳语义修正：
- `last_converged_s` 仅在状态从未收敛切换为收敛时更新，避免被周期采样覆盖。
