# Version v2.0.0-alpha9

版本定位：

- V2 第九次增量（alpha）：补齐 leader 主动 ROUTE_UPDATE 提示的细粒度指标输出，让收敛提示从“事件可见”升级为“指标可量化”。

本版完成：

1. route hint 细粒度指标（leader 侧）：
- 新增累计计数：`routeHintSentTotal`、`routeHintFailedTotal`、`routeHintCooldownSkippedTotal`；
- 新增最近活动时间：`routeHintLastActivityS`。

2. 状态接口扩展：
- `GET /api/status` 的 `routeMetrics` 节点新增以上 4 个字段；
- 保持现有字段兼容，不改协议包结构。

3. 状态页可观测性增强：
- 新增 `Route Hint Sent/Failed/Cooldown Skip/Last Activity` 展示；
- Last Activity 为 0 时显示 `N/A`。

4. 统计更新策略：
- hint 发送成功/失败分别计数；
- 命中冷却跳过计数；
- 任意 hint 行为发生时刷新 `routeHintLastActivityS`。
