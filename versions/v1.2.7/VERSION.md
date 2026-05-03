# Version v1.2.7

当前版本：

- `v1.2.7`

说明：

- WS63 板端 HTTP 服务给每个手机连接设置 1.2 秒接收超时和发送超时。
- HTTP listen backlog 从 2 调整为 4，减少手机浏览器并发/半开连接时卡住下一次访问的概率。
- WebUI 点 `set leader` / `set member` 后只排队角色切换并立刻返回页面，SLE 初始化改由主任务执行，避免 HTTP 请求被 SLE 启动过程拖住。
- member 初次 HELLO 如果 SLE client 还没 ready，不再把 Web 设置判失败；后续 tick 会继续每 3 秒重发 HELLO。
- SLE 收包后串口打印上层协议处理结果 `node packet ... ret=...`，用于判断灯亮但 pending/nodes 不更新时是解包失败、team/channel 不匹配还是 allowlist 拒绝。
- 板端页面版本标识更新为 `ssr=v5`。
- 保留 v1.2.6 的统一固件、NV flash Web 配置、恢复出厂、`/pairing` 不自动刷新等能力。

已验证状态：

- Ubuntu 编译机 `owen@192.168.6.130` 出包通过。
- 新统一固件包已生成：
  `/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_unified_runtime_role/ws63-liteos-app_unified_all.fwpkg`

已知限制：

- pending 列表、在线节点状态和 events 日志仍是 RAM 运行时状态，重启后重新发现。
- 当前仍不是完整“扫描附近 leader 并选择”的发现系统；member 选择 leader 需要填写 `team / leader / channel`。
- 当前配队不是加密认证，`cipher_mac` 仍未用于真实鉴权。
- 新 HELLO 和旧固件不兼容；leader/member 两块板都需要烧录同一版统一固件。
