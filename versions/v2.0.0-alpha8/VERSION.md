# Version v2.0.0-alpha8

版本定位：

- V2 第八次增量（alpha）：对 alpha7 的 leader 主动 ROUTE_UPDATE 收敛提示做流量治理与可读性收敛，降低 flapping 场景控制包噪声。

本版完成：

1. per-member ROUTE_UPDATE 提示冷却：
- 新增成员级 route hint 发送缓存（member_id / parent_id / last_sent）；
- 对“同成员 + 同 parent”提示增加冷却窗口（默认 12s），避免网络抖动时每个 metrics 周期重复下发。

2. 提示语义保持：
- 若 parent 发生变化，不受同-parent 冷却限制，可立即发送；
- 若成员首次出现或缓存淘汰后重入，可立即发送。

3. 代码可读性优化：
- 将 `hint_parent_id` 下沉为循环内局部变量，避免跨迭代作用域歧义；
- 保持失败路径可观测（失败项日志 + 系统事件聚合）。

4. 兼容性：
- 不改协议结构；
- 与 alpha7 的主动收敛提示、alpha6 的回绕安全逻辑兼容。
