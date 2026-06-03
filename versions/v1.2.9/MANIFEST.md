# v1.2.9 Manifest

本版本记录 LED 三态提示、leader 多连接表、leader 节点标签修复，以及 2026-05-04 双 member 配队测试。

主要变更：

- `src/sle_team_node.c`
  - leader approve pending member 时，把 pending 中的 MAC/role/battery/last_seen 迁移到正式 member 记录。
  - 已登记到 nodes 的 member 即使不在当前 allowlist 数组里，也视为已批准 member；避免 leader 打开 pairing window 后把老 member 重新打回 pending。
- `xc/ws63_team_network/src/ws63_team_network_app.c`
  - 增加 `SLE_TEAM_LED_EVENT_SEEK`，用于 pairing/扫描阶段的极短闪提示。
  - leader pairing window open 时周期触发 seek LED。
  - leader 作为 SLE client/central 启动，member 作为 SLE server/peripheral 启动。
  - member leave 后清除 NV 配置并回到未配置页面状态。
  - leader 收到上行包时解析源 member id，并绑定到对应 SLE `conn_id`。
  - HTTP 响应头增加 no-cache，并扩大 header buffer。
  - route label fallback 改为角色前缀，避免 `Nxx` 混淆。
- `xc/ws63_team_network/sle_uart_client/sle_uart_client.c`
  - 按官方 1vs8 思路，把 leader 侧 client 改成最多 8 个连接的连接表。
  - connect/disconnect 时维护连接数量、remote addr、member id 和 RSSI。
  - leader 单播下行优先按 `member_id -> conn_id` 精确发送；广播下行仍发到所有已连接链路。
  - 新增地址去重，避免扫描时反复连接同一个 member。
  - 新增带返回状态的 member/conn 查询接口，修复 `conn_id=0` 被误判为“未找到连接”的问题。
- `xc/ws63_team_network/sle_uart_server/sle_uart_server.c`
  - member 侧保留 server/peripheral 角色，支持被 leader 连接后通过 notify 上行。
- `xc/ws63_team_network/README.md`
  - 更新 LED 诊断说明。

验证：

```sh
UBUNTU_HOST=192.168.6.5 UBUNTU_USER=owen UBUNTU_PASS='<set locally, do not commit secrets>' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 BUILD_JOBS=4 \
scripts/ws63_build_team_ubuntu.sh unified
```

结果：

- Ubuntu 编译通过。
- 产物：
  `<sdk-root>/output_from_vm/team_network_unified_runtime_role/ws63-liteos-app_unified_all.fwpkg`
- 烧录 leader 与 member 成功。
- 串口测试确认并修复：
  - 首个 SLE 连接可为 `conn_id=0`，旧逻辑会把它当成“未找到连接”。
  - leader 打开 pairing window 后，旧逻辑会把已 joined member 打回 pending，导致 timeout/rejoining。
- 本地回归测试已覆盖：
  - member 未 approve 前只进入 pending，不 joined。
  - approve 后 member joined。
  - 已 joined member 在 leader 再次打开 pairing window 后不会回到 pending。

限制：

- 多连接表已经实现，但第二个 member 同时出现在 pairing window 仍需烧录后复测。
- 当前已做 member 到 conn_id 的精确映射；per-member RSSI 仍待 WebUI 展示。
