# Version v2.0.0-alpha2

版本定位：

- V2 第二次增量（alpha）：在已有 auto-approve 基础上补齐“parent 自主选择 + 断连自愈判定收敛”。

本版完成：

1. member 自动选父（autoselect parent）：
- 在 member 侧引入 parent 自动选择逻辑，从可用上行连接里按 RSSI 选择更优父节点。
- 增加切换冷却时间与 RSSI 滞回阈值，减少来回抖动。

2. 断连自愈判定收敛：
- 修复“非当前父链路断开也触发 member leave/reselect”的冲突。
- 现在仅当前 upstream parent 断开时才触发离队与重选，避免误重连。

3. 底层接口补齐（供选父与调度使用）：
- server 侧新增 active conn 列表、conn->member 查询、conn RSSI 查询、按 conn 主动断开接口。

4. 继承上一版行为：
- pairing stop 自动审批 pending 仍保持生效，且默认 `relay_allowed=0`。

5. 审查回合修复（W1/W2）：
- 修复 `team_upstream_parent_note()` 下行分支可能覆写 `parent_id` 的问题，避免 parent 记录与日志污染。
- 放开 `team_member_autoselect_parent()` 的 relay 限制，leaf member 也可执行 RSSI 选父逻辑，和 V2 goal 对齐。
