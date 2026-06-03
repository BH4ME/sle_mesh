# Version v1.2.8

当前版本：

- `v1.2.8`

说明：

- WS63 统一固件继续由 WebUI/CLI 在运行时选择 leader/member 角色，不再维护两份业务固件。
- RSSI 改为调用官方 SLE 连接管理接口 `sle_read_remote_device_rssi(conn_id)`，通过 `read_rssi_cb` 缓存真实 dBm；未读到时显示 `NA`。
- 协议心跳里的 RSSI 不再使用模拟 `-50`，而是使用当前 SLE 连接的真实 RSSI。
- member 发送时如果 SLE client 还没有 ready，或写入失败，会按 5 秒节流触发重新扫描，避免一直停在 `NOT_READY`。
- member 已 joined 后，如果超过心跳超时没有再收到 leader 的包，会自动退回 discovering/joining 并重新发送 `HELLO`。
- leader 侧 SLE 断开后会重新启动 announce，等待 member 重新连接。
- leader/member SLE UART sample 已复制到本项目样例目录，避免修改 SDK 公共样例后被其他实验覆盖。
- WebUI 和 CLI 的 RSSI 字段支持 `null/NA`，避免把未知值当成真实信号。

已验证状态：

- Ubuntu 编译机 `owen@192.168.6.130` 出包通过。
- 两块 WS63 已烧录同一个统一固件包：
  `<sdk-root>/output_from_vm/team_network_unified_runtime_role/ws63-liteos-app_unified_all.fwpkg`
- 实测 member 首次 `NOT_READY` 后出现 `member force rescan reason=not_ready`，随后完成 SLE scan/connect/pair/service discovery。
- 实测 SLE RSSI 回调有真实值，例如 `-31 dBm`、`-28 dBm`、`-15 dBm`。
- leader 收到 member `HELLO`，批准后 member 收到 `CONFIG + ACK` 并进入 joined。
- 实测双向心跳稳定：
  - `HEARTBEAT 241->154`
  - `HEARTBEAT 154->255`

已知限制：

- leader 的 Nodes 页在本版本现场发现仍可能显示 `NF1` 这类 fallback 标签，因为 approve 时 pending member 的 MAC 尚未迁移到正式 node 记录；下一版修复。
- member 的手机页面在 SoftAP 无外网场景下仍可能被浏览器提示网络丢失，下一版继续优化 HTTP 响应和连接体验。
- pending 列表、在线节点状态和 events 日志仍是 RAM 运行时状态，重启后重新发现。
- 当前仍不是完整“扫描附近 leader 并选择”的发现系统；member 选择 leader 需要填写 `team / leader / channel`。
- 当前配队不是加密认证，`cipher_mac` 仍未用于真实鉴权。
