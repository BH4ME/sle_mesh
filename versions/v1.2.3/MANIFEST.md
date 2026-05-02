# v1.2.3 Manifest

本版本记录 WS63 leader SoftAP + HTTP 控制台调试状态。

现场验证结论：

- `status`、`nodes`、`events` 页面均已在手机浏览器现场验证可打开。
- 当前可用基线为 `ssr=v3`。
- 当前没有 member 入网，业务上 `nodes/events` 显示 `[]` 是预期状态。

主要变更：

- `xc/ws63_team_network/src/ws63_team_network_app.c`
  - SoftAP/HTTP 控制台。
  - `/api/status`、`/api/nodes`、`/api/events`。
  - `/`、`/nodes`、`/events` 服务端 HTML 页面。
  - HTML 使用完整 `Content-Length` 响应，避免手机浏览器对流式响应一闪而过。
- `include/sle_team_web_api.h`
  - Web API 事件缓存压缩以降低 RAM 压力。
- `README.md`
  - 更新当前版本和 WS63 WiFi 控制台说明。
- `xc/ws63_team_network/README.md`
  - 增加 v1.2.3 上板调试记录。
