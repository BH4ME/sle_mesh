# Version v2.0.0-alpha11

版本定位：

- V2 第十一次增量（alpha）：补齐 pairing window 阶段“临时隐藏 relay”闭环，实现“只转发发现/路由控制、不转发业务”的可控中继模式。

本版完成：

1. 隐藏 relay 标志位与配置链路：
- 复用 `CONFIG.reserved` 标志位，定义 `SLE_TEAM_CONFIG_FLAG_RELAY_DISCOVERY_ONLY`；
- leader 在下发 relay CONFIG 时可标记“discovery-only”模式；
- member 在接收 CONFIG 时解析并更新本地 `relay_discovery_only` 状态。

2. relay 转发白名单：
- 当 relay 处于 discovery-only 模式时，仅允许转发 `HELLO` 与 `ROUTE_UPDATE`；
- `POS_REPORT` 等业务消息在该模式下不经 relay 转发；
- 保持非 discovery-only 模式下既有转发行为不变。

3. pairing start/stop 自动收敛：
- `pairing start` 后，leader 主动刷新在线 relay 配置，切入 discovery-only；
- `pairing stop` 后，leader 主动刷新在线 relay 配置，恢复常规转发；
- 避免窗口开关后 relay 长时间停留在旧模式。

4. 验证覆盖：
- `examples/team_network_demo.c` 新增断言，覆盖：
  - pairing 开/关导致的 discovery-only 标志切换；
  - discovery-only 模式下 `POS_REPORT` 不转发；
  - discovery-only 模式下 `ROUTE_UPDATE` 可转发。
