# Version v2.0.0-alpha6

版本定位：

- V2 第六次增量（alpha）：在 alpha5 可观测性闭环基础上，补齐时间窗口逻辑的回绕安全与地址派生 route id 稳定性，提升长期运行可靠性。

本版完成：

1. 时间窗口比较回绕安全化：
- 新增统一的秒级时间差辅助函数（`team_elapsed_s` / `team_interval_not_reached` / `team_elapsed_exceeds`）；
- 替换 route metrics、parent switch cooldown、leader rescan、pairing rotate、relay rebalance 等窗口判断中的直接减法比较；
- 替换 stale/timeout 判定中的直接减法，避免 32-bit 秒计数回绕后误判。

2. 路由陈旧与超时判定增强：
- `team_route_entry_is_stale()` 改为基于统一辅助函数判定；
- relay 候选超时、relay 回收 stale 判定统一走回绕安全路径。

3. 地址派生 route id fallback 改进：
- `team_route_id_from_sle_addr()` 在缺少显式 route 字节时，从“单字节推导”改为“全地址混合推导”；
- 降低不同设备地址映射到同一 route id 的冲突概率，提升临时路由识别稳定性。

4. 与 alpha5 能力兼容：
- 不改协议包结构；
- 不改变 leader 唯一准入控制者模型；
- routeMetrics / events / status 页面可观测能力保持不变。
