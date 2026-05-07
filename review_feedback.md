# Review Feedback: SLE Relay Routing Update

基于 README 对照当前代码（commit `21382a4` + follow-up patches）的审查结果。

---

## README 声明逐项验证

| README 声明 | 代码位置 | 结论 |
|---|---|---|
| 分层 relay 路由（leader + 多级 member） | `sle_team_should_relay_packet()` (node.c:208-226), `sle_team_forward_packet()` (node.c:191-206), bucket 路由 (app.c:756-761) | ✅ 已实现 |
| leader 是唯一审批方 | `sle_team_node_pairing_approve_with_relay()` (node.c:441) — 只有 leader 调用 | ✅ 已实现 |
| 1vs8 物理直连约束下的 1vs20 逻辑成员管理 | `SLE_TEAM_MAX_LOGICAL_MEMBERS 20U`, `SLE_TEAM_MAX_DIRECT_CONNECTIONS 8U` (node.h:13-14), 服务端连接表 8 (server.c:24) | ✅ 已实现 |
| leader-controlled approve-to-relay | `pairing_approve_with_relay()` 设置 `member->relay_allowed`, 扩展 CONFIG 携带 relay 字段, `team_relay_start_client_if_ready()` 要求 `joined + relay_allowed` (app.c:2898-2917) | ✅ 已实现 |
| member_id → next_hop 路由学习，优先走 next-hop 定向转发 | `team_route_note()` 记录 (app.c:940-956), `team_route_find()` 优先 next_hop 解析 (app.c:957-981) | ✅ 已实现 |
| member 上行 parent 状态维护与断连后 reselection/rejoin | `team_upstream_parent_note/reset()` (app.c:1252-1281), `sle_team_member_rejoin()` (node.c:115-130) | ✅ 已实现 |
| next-hop 严格选路：next_hop 不可达时报 NO_ROUTE，不回退陈旧 conn_id | `team_route_find()` line 974 在 next_hop 不可达时 return 0（不降级到 conn_id）(app.c:964-975) | ✅ 已实现 |

---

## 上一轮反馈状态更新

### 已关闭（当前代码已解决）

| 上一轮反馈 | 状态 | 解决证据 |
|---|---|---|
| Relay activation not approval-gated | ✅ **已修复** | `team_relay_start_client_if_ready()` 要求 `g_team_node.joined != 0U` 且 `relay_allowed != 0U` (app.c:2898-2917) |
| Logical team size still capped at 8 | ✅ **已修复** | `SLE_TEAM_MAX_LOGICAL_MEMBERS 20U`, `SLE_TEAM_MAX_MEMBERS` = 20 (node.h:13-15) |
| Return path relies on send-all fallback | ✅ **已修复** | `team_route_find()` next-hop strict: 不可达直接 return 0 (app.c:974), 路由表优先于广播回退 |
| relay_allowed 默认值为 1 | ✅ **已修复** | HTTP pairing: relay 参数缺失时默认 0 (app.c:2229-2230: `relay_allowed = 0U`) |
| Connection callback direction race | ✅ **已修复** | `g_team_pending_conns[]` 表存储候选者地址/route_id/bucket (app.c:842-868) |
| Next-hop not really used in routing | ✅ **已修复** | `team_route_find()` 优先 next_hop 查找，`team_sle_send()` 使用路由表寻址 |
| VM script not documenting aliases | ✅ **已修复** | 已删除旧版 VM 脚本，统一 ws63_build_team_ubuntu.sh |
| Header includes incomplete | ✅ **已修复** | `sle_uart_server_adv.h` 已添加 `#include "errcode.h"` 和 `#include "sle_common.h"`，自包含 |

### 仍存在的观察项（非阻塞）

1. **重连后 relay 使能间隙** — ACK（joined=1, relay_enabled=0）和 CONFIG（relay_allowed=1, relay_enabled=1）之间有一个短暂窗口 relay 未激活。这是预期行为——relay 授权来自 leader CONFIG，不是本地状态。

2. **任务间共享状态无显式锁** — `TeamNetworkTask` 和 `TeamWifiApTask` 共享 `g_team_node`，无互斥保护。在 WS63/LiteOS 中 SLE 回调可能串行化到主任务队列，但代码层面无保证。

3. **函数命名问题** — `sle_uart_client_is_connected()` 定义在 `sle_uart_server.c`，返回服务端连接数 `g_sle_conn_count`。命名与语义不符，但所有调用点用法正确。

4. **sle_uart_server_adv.h 头文件依赖** — 使用 `errcode_t` 和 `SLE_ADDR_LEN` 但未包含对应头文件（依赖调用方前置包含）。

---

## 当前代码审查

新增检查项：

### 协议一致性

- `sle_team_node_on_packet()` line 850：成员只处理 `src_id == leader_id` 的包。这是星型拓扑的正确行为，非 leader 源广播在到达此处前已被 relay 逻辑过滤或拒绝。
- `sle_team_should_relay_packet()` 的广播转发决策：只有 leader 源的广播被转发（line 220），防止路由环。
- ACK handler line 582：`relay_enabled = relay_allowed != 0 ? 1 : 0` — relay 状态从已保存的 `relay_allowed` 派生，不是独立设置。

### 路由完整性

- `team_route_find()` next-hop 严格选路完整验证：若 next_hop_id 存在但对应连接不可达 → 立即 return 0（NO_ROUTE），不降级到 route->conn_id。
- 无 next_hop_id 时（传统 conn_id 路由）：先验证 `team_route_conn_is_active()` 再使用。
- `team_bind_packet_source()` 在 member upstream 方向的 parent 覆盖保护（app.c:1072-1078）：leader 源包不覆盖已学习的 first-hop relay route_id。

### 连接生命周期

- 上游断连 → `member_leave()` 清空 relay → parent = RESELECTING → tick 驱动 rejoin → HELLO → ACK+CONFIG 完整恢复。
- 连接回调方向分类使用三层回退：conn track → pending addr table → client/server conn table。pending 表在 seek filter 命中时填充。
- Disconnect handler 清除 route 和 conn track，防止 stale 状态残留。

---

## 总结

README 的 7 项功能声明经代码验证全部属实。上一轮 review_feedback 中的主要问题（relay 审批门控、逻辑容量 20、next-hop 严格路由、relay_allowed 默认值、pending 连接表）均在当前代码中已解决。剩余 4 项观察为非阻塞性质。
