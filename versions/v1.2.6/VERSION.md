# Version v1.2.6

当前版本：

- `v1.2.6`

说明：

- WS63 内置 WebUI 的角色配置改为写入 NV flash。
- WebUI 设置 leader 后，会保存 leader 角色和本机 MAC 后四位。
- WebUI 设置 member 或重新选择 leader 后，会保存 member 角色、team、leader MAC 后四位和 channel。
- 断电或复位后，固件会校验 flash 配置并自动恢复 leader/member 角色。
- WebUI 增加 `factory reset` 操作，清除 flash 中的 Web 配置并用 watchdog 重启，恢复到 `UXXXX` 未配置状态。
- member 执行 `leave` 会清除 flash 配置，避免重启后自动回到旧 leader。
- `/pairing` 页面取消 3 秒自动刷新，避免手机填写表单时被刷新打断；status/nodes/events 页面仍自动刷新。
- 板端页面版本标识更新为 `ssr=v4`。

已验证状态：

- Ubuntu 编译机 `owen@192.168.6.130` 出包通过。
- 新统一固件包已生成：
  `<sdk-root>/output_from_vm/team_network_unified_runtime_role/ws63-liteos-app_unified_all.fwpkg`

已知限制：

- pending 列表、在线节点状态和 events 日志仍是 RAM 运行时状态，重启后重新发现。
- 当前仍不是完整“扫描附近 leader 并选择”的发现系统；member 选择 leader 需要填写 `team / leader / channel`。
- 当前配队不是加密认证，`cipher_mac` 仍未用于真实鉴权。
- 新 HELLO 和旧固件不兼容；leader/member 两块板都需要烧录同一版统一固件。
