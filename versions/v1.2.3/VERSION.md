# Version v1.2.3

当前版本：

- `v1.2.3`

说明：

- WS63 leader 固件加入 SoftAP + HTTP 本机控制台。
- 手机连接 `SLE-TEAM-WS63-L1` 后访问 `http://192.168.43.1/`。
- 板端页面接入真实接口：`/api/status`、`/api/nodes`、`/api/events`。
- 当前可用基线为 `ssr=v3`：板端直接渲染 `/`、`/nodes`、`/events` 三个页面。
- HTML 页面使用完整 `Content-Length` 响应，避免 iOS/微信内置浏览器对无长度流式响应一闪而过。
- JSON API 仍保留：`/api/status`、`/api/nodes`、`/api/events`。

已验证状态：

- leader SoftAP 正常启动。
- 手机可连接 AP，并已验证 `/api/status` 正常。
- `/`、`/nodes`、`/events` 页面均已在手机浏览器现场验证可打开。
- 页面底部显示 `page=... ssr=v3`，可用于确认当前烧录固件。
- 当前没有 member 加入 leader，所以 `nodes/events` 显示 `[]` 是预期状态。

已知坑：

- iOS 看到 WS63 WiFi 无互联网时，可能自动切到蜂窝数据。右上角如果显示 `5G` 而不是 WiFi，页面请求 `192.168.43.1` 会失败。
- Safari 刷新页面时会取消旧连接，串口可能出现 `errno=104`，这通常是客户端断开，不等于板端 HTTP 服务崩溃。调试时优先使用页面内按钮刷新数据。
- 旧的无 `Content-Length` 流式 HTML 在 iOS/微信内置浏览器里可能一闪而过；当前 `ssr=v3` 已改为完整长度响应。
- 当前 DHCP 仍需继续观察；必要时手机手动设置静态 IP 到 `192.168.43.x` 网段。

固件记录：

- 最新 leader 测试包：
  `<sdk-root>/output_from_vm/team_network_leader_wifi_console_ssr_v3/ws63-liteos-app_all.fwpkg`
