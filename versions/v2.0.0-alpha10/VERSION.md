# Version v2.0.0-alpha10

版本定位：

- V2 第十次增量（alpha）：围绕“复杂多跳自愈闭环”的实机验证需求，补齐 leader 侧 ROUTE_UPDATE 观测指标，让父节点重选/重挂载可量化追踪。

本版完成：

1. ROUTE_UPDATE 接收与重挂载计数（leader 侧）：
- 新增累计计数：`routeUpdateRxTotal`；
- 新增重挂载累计：`routeReparentTotal`；
- 新增最近重挂载时间：`routeReparentLastS`。

2. 状态接口扩展：
- `GET /api/status` 的 `routeMetrics` 新增以上 3 个字段；
- 保持现有字段兼容，不改协议包结构。

3. 状态页可观测性增强：
- 新增 `Route Update RX Total` / `Route Reparent Total` / `Route Reparent Last`；
- `Route Reparent Last` 为 0 时显示 `N/A`。

4. 统计策略：
- leader 收到有效 `ROUTE_UPDATE` 即累计 `routeUpdateRxTotal`；
- 当同成员 next-hop 相比历史路由发生变化时累计 `routeReparentTotal`，并刷新 `routeReparentLastS`。
