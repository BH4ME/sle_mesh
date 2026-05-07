# Version v2.0.0-alpha1

版本定位：

- V2 第一阶段（alpha）：围绕 `/goal v2` 落地“pairing window 可见性突破 + 自动 relay 初始授权”。

本版完成：

1. Pairing window 分时轮询发现：
- leader 在 pairing window 打开期间，周期性轮换并释放“未进入 pending/online”的连接，持续扫描新候选。
- 目标是突破“同一时刻最多8直连”的可见性瓶颈，让 pending 列表可逐批覆盖更多逻辑成员。

2. Pairing stop 自动审批 + 自动 relay 首批授权：
- 关闭 pairing window 时自动审批当前 pending 成员。
- 自动 relay 授权采用首批配额策略（当前默认最多3个 relay）。

3. 客户端连接管理能力增强（供 V2 调度使用）：
- 读取 active 连接列表；
- 查询连接绑定的 member；
- 按连接主动断开。

版本管理约定（从本版开始强制执行）：

1. 每完成一批功能必须执行一次提交（不积压大改动）。
2. 每次提交前至少跑本地基础回归：
- `SLE_TEAM_NETWORK_TEST`
- `SLE_TEAM_PACKET_TEST`
3. 每次提交后必须更新对应版本目录：
- `VERSION.md` 说明“完成了什么”
- `MANIFEST.md` 说明“改了哪里、怎么验证”
