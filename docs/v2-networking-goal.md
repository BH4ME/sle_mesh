/goal
V2组网目标：从“V1手动relay授权 + relay主动找leaf”升级为“自动relay选举 + member自动选父 + relay失效自愈”的1vs20逻辑组网。

一、核心约束
1. 物理连接上限仍是单跳8（SDK限制），但逻辑成员目标20。
2. leader仍是唯一准入控制者（approve authority不变）。
3. 不合并到主分支，在v2分支独立迭代。

二、问题1目标（pairing window看到全部member）
1. 目标：leader在pairing window中展示尽可能完整的member列表（>=20逻辑容量），而不是只看到8个直连。
2. 结论：仅靠leader直连不可能长期同时>8；要看到更多成员必须“时间复用连接”或“经中继转发HELLO”。
3. V2方案：
- 方案A（优先）：pairing window期间启用“发现模式轮询连接”（leader循环连接候选、拉取HELLO、写入pending缓存后释放连接槽）。
- 方案B（增强）：引入“临时隐藏relay（仅转发HELLO/ROUTE_UPDATE，不转发业务）”以覆盖leader直连不到的节点。
- pending成员表与UI展示按逻辑成员容量20设计，带last_seen和来源路径。

三、问题2目标（自动relay授权与member选relay）
1. 目标：不再手动relay授权；pairing window关闭后自动产生relay集合。
2. 自动relay选举（leader侧）：
- 输入：RSSI到leader、在线稳定时长、电量、历史掉线率、当前负载。
- 输出：relay_allowed集合（限制数量与层级），下发CONFIG。
3. member自动选父（不是手工选）：
- 候选：leader或approved relay。
- 评分：RSSI、relay负载、链路稳定度、hop成本。
- 切换策略：滞回阈值 + 冷却时间，避免频繁抖动。

四、问题3目标（relay掉线自愈）
1. 触发：parent断连或heartbeat timeout。
2. 行为：member进入reselecting，重新扫描候选父节点并上报ROUTE_UPDATE。
3. leader行为：对已approved成员fast rejoin（不要求人工再approve），更新member_id->next_hop路由。
4. 组内若relay不足：自动重新选举新relay并下发授权。

五、验收标准
1. pairing window内可看到20个逻辑member（直连+轮询+临时中继路径）。
2. 关闭pairing window后自动形成relay拓扑，无需手动点relay=1。
3. 任一relay掉线后，组内leaf可在超时窗口内自动恢复上报（无人工干预）。
4. 业务路由保持next-hop定向转发，不回退为不受控泛洪。

---

当前进度（V2 Phase-1，已实现）
1. 已实现 pairing window 分时轮询连接：
- leader 在 pairing window 期间按周期轮换断开“未进入 pending/online 的连接”，释放连接槽继续扫描新候选。
- 目标是突破“同时8直连”限制，逐批发现更多 member 并写入 pending。
2. 已实现 pairing stop 自动审批：
- 关闭 pairing window 时，leader 自动审批当前 pending 列表。
- 自动 relay 授权采用首批配额策略（当前默认最多3个 relay）。
3. 已补齐 client 连接管理接口：
- 读取 active conn 列表；
- 查询 conn 绑定 member；
- 按 conn 主动断开。
