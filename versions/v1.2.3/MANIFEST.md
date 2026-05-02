# v1.2.3 Manifest

本版本记录 WS63 leader SoftAP + HTTP 控制台调试状态。

现场验证结论：

- `status` 正常，是当前可用基线。
- `nodes/events` 仍不正常或未稳定通过，不作为本版本成功项。
- 当前没有 member 入网，业务上 `nodes/events` 即使请求成功也可能为空。

主要变更：

- `xc/ws63_team_network/src/ws63_team_network_app.c`
  - SoftAP/HTTP 控制台。
  - `/api/status`、`/api/nodes`、`/api/events`。
  - HTML 分段发送。
  - 手机网络失败时保留最后成功数据。
- `include/sle_team_web_api.h`
  - Web API 事件缓存压缩以降低 RAM 压力。
- `README.md`
  - 更新当前版本和 WS63 WiFi 控制台说明。
- `xc/ws63_team_network/README.md`
  - 增加 v1.2.3 上板调试记录。
