# Code Review Feedback

**Reviewer:** Review Service (configured-provider)
**Date:** 2026-05-10
**Version:** v2.0.0-alpha11
**Branch:** line/v2-auto-networking
**Scope:** 增量/小修改 — alpha11 之上的 3 项加固修复（HELLO ACK 拒绝、CONFIG 角色/来源守卫、心跳零间隔守卫）

---

## 1. 新代码逻辑审查

### 审查清单

- [x] **状态机完备性** — HELLO ACK 拒绝后 member 保持 `DISCOVERING`/`JOINING` 状态不变；CONFIG 守卫不涉及状态机变更；心跳零间隔守卫仅跳过发送，不改变 state
- [x] **竞态条件** — 所有变更在主循环单线程路径执行，无可重入/并发路径
- [x] **缓冲区安全** — 无新增缓冲区操作
- [x] **空指针与边界** — HELLO ACK 处理在 `node==NULL || app==NULL || body_len < sizeof(*ack)` 格式检查之后；CONFIG 守卫在相同格式检查之后；心跳守卫仅读取 `cfg` 字段，无指针操作
- [x] **资源泄漏** — 无动态分配
- [x] **整数与类型** — `status_code` 为 `uint8_t` 与 `0U` 比较正确；`heartbeat_interval_s` 为 `uint16_t` 与 `0U` 比较正确；无符号溢出风险：在 `heartbeat_interval_s != 0U` 前置条件下，`(now_s - last_heartbeat_s) >= interval` 依赖 `now_s >= last_heartbeat_s`（单调递增时间源），行为安全
- [x] **魔法数字** — `SLE_TEAM_APP_HELLO`、`SLE_TEAM_ROLE_MEMBER`、`SLE_TEAM_ERR_UNSUPPORTED` 均为已有枚举/宏
- [x] **协议兼容性** — 所有变更均为接收端守卫逻辑，不改变包结构或发送行为；新旧互通不受影响

---

### 1.1 HELLO ACK 拒绝处理 (sle_team_node.c:630-632)

```c
if (ack->status_code != 0U) {
    sle_team_log(node, "hello ack rejected");
    return SLE_TEAM_ERR_UNSUPPORTED;
}
```

**变更前：** member 收到任何 HELLO ACK（包括非零 status_code）都会触发 `joined=1U`、`state=ONLINE` 等入网动作。

**变更后：** 当 leader 返回非零 status_code 时，member 拒绝入网，保持原状态不变。

- 守卫位置正确：在格式检查之后、`upstream_parent_id`/`joined`/`state` 等状态变更之前 ✅
- `status_code` 为零表示批准、非零表示拒绝，语义符合通用约定 ✅
- 使用 `SLE_TEAM_ERR_UNSUPPORTED` 作为返回值，调用方可感知处理失败 ✅
- 返回前不修改任何 node 状态，无副作用 ✅

**测试 (team_network_demo.c:226-234)：**
```
ACK(status_code=1) → assert(joined==0, state==DISCOVERING)  // 拒绝后保持未入网
ACK(status_code=0) → assert(joined!=0)                       // 正常批准后入网
```
覆盖拒绝路径和正常路径，边界明确 ✅

---

### 1.2 CONFIG 角色/来源守卫 (sle_team_node.c:656-660)

```c
if (node->cfg.role != SLE_TEAM_ROLE_MEMBER || app->src_id != node->cfg.leader_id) {
    sle_team_log(node, "config rejected by role/source");
    return SLE_TEAM_ERR_UNSUPPORTED;
}
```

**变更前：** 任何节点收到 CONFIG 包都会处理并覆盖自身配置，包括 leader 收到 member 发来的 CONFIG。

**变更后：** 仅 `role==MEMBER` 且 `src_id==leader_id` 时处理 CONFIG，其余情况拒绝。

- 守卫位置正确：在格式检查之后、`report_interval_s` 等配置写入之前 ✅
- `role != MEMBER` 防止 leader 被 member 的 CONFIG 污染 ✅
- `src_id != leader_id` 防止 member 被非 leader 来源的 CONFIG 误导 ✅
- 多跳场景中 app 层 `src_id` 保持原始发送者不变（mesh 转发不改 app 包内容），因此通过 relay 转发的 leader CONFIG 仍会通过此守卫 ✅

**测试 (team_network_demo.c:362-377)：**
```
fake_member(role=MEMBER, self_id=99) → send_config(leader) →  leader 收到 CONFIG
→ assert(leader.cfg.report_interval_s 不变)
```
覆盖 `role != MEMBER` 路径（leader 收到 CONFIG 时拒绝）。`src_id != leader_id` 路径未独立测试但逻辑等价（同一 if-OR 守卫），低风险。

---

### 1.3 心跳零间隔守卫 (sle_team_node.c:852-853)

```c
if (node->cfg.heartbeat_interval_s != 0U &&
    (now_s - node->last_heartbeat_s) >= node->cfg.heartbeat_interval_s) {
```

**变更前：** `(now_s - last_heartbeat_s) >= 0` 始终为真（无符号算术），导致 `heartbeat_interval_s=0` 时每个 tick 都发送心跳，产生无意义流量。

**变更后：** `interval != 0` 前置条件确保 `heartbeat_interval_s=0` 时完全跳过心跳发送，语义等价于"禁用心跳"。

- 前置 `&&` 短路，`interval==0` 时不计算时间差，无整数溢出风险 ✅
- 不影响 `interval>0` 的正常心跳逻辑 ✅
- 逻辑一致性：interval=0 表示"不发送心跳"是直观的零值语义 ✅

**测试 (team_network_demo.c:378-392)：**
```
hb_member(heartbeat_interval_s=0, joined=1, state=ONLINE) → tick()
→ assert(last_tx_len == 0)  // 无心跳包发出
```
覆盖零间隔场景 ✅

---

## 2. 发现的问题

**Blocker:** 无

**Warning:** 无

**Note:** 无

---

## 3. 汇总

| 类别 | 数量 | 说明 |
|------|------|------|
| **Blocker** | 0 | 无阻塞性问题 |
| **Warning** | 0 | — |
| **Note** | 0 | — |

**结论:** ✅ **通过**

alpha11 增量加固三个防守点：

1. **HELLO ACK 拒绝**：当 leader 通过 `status_code != 0` 拒绝 HELLO 时，member 不再错误入网。逻辑完整（守卫位置、无副作用、状态不变），测试覆盖拒绝+正常双路径。
2. **CONFIG 角色/来源守卫**：仅 member 且来自 leader 的 CONFIG 被接受，防止 leader 被 member 的 CONFIG 污染。守卫在格式检查后、状态变更前，多跳场景兼容。
3. **心跳零间隔守卫**：修复 `interval=0` 时每个 tick 都发心跳的 bug，语义修正为"禁用心跳"。改动单行、影响范围明确、测试覆盖。
