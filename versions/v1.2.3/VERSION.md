# Version v1.2.3

当前版本：

- `v1.2.3`

说明：

- WS63 leader 固件加入 SoftAP + HTTP 本机控制台。
- 手机连接 `SLE-TEAM-WS63-L1` 后访问 `http://192.168.43.1/`。
- 板端页面接入真实接口：`/api/status`、`/api/nodes`、`/api/events`。
- 页面采用手动刷新为主，并在失败时保留最后成功数据，降低 iOS/Safari 刷新、取消连接或无互联网 WiFi 导致的抖动影响。
- HTTP 页面使用小块分段发送，绕开 WS63 上长字符串/发送路径导致页面截断的问题。

已验证状态：

- leader SoftAP 正常启动。
- 手机可连接 AP，并已验证 `/api/status` 正常。
- `/` 页面可完整分段发送。
- `nodes/events` 目前不能标记为正常：串口观察到请求和 `[]` 响应，但手机页面侧仍有 loading/刷新不稳定现象。
- 当前没有 member 加入 leader，所以即便接口成功，`nodes/events` 的业务数据也应为空。

已知坑：

- iOS 看到 WS63 WiFi 无互联网时，可能自动切到蜂窝数据。右上角如果显示 `5G` 而不是 WiFi，页面请求 `192.168.43.1` 会失败。
- Safari 刷新页面时会取消旧连接，串口可能出现 `errno=104`，这通常是客户端断开，不等于板端 HTTP 服务崩溃。调试时优先使用页面内按钮刷新数据。
- 当前现场基线只确认 `status` 正常；`nodes/events` 需要后续继续修。
- 当前 DHCP 仍需继续观察；必要时手机手动设置静态 IP 到 `192.168.43.x` 网段。

固件记录：

- 最新 leader 测试包：
  `/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_leader_wifi_console_stable_fetch/ws63-liteos-app_all.fwpkg`
