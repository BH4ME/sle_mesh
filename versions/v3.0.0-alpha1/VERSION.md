# Version v3.0.0-alpha1

版本定位：

- V3 首个 alpha 基线：建立“手机定位 -> 固件 Web API -> SLE `POS_REPORT` -> 节点回显”的端到端链路。

本版完成：

1. 手机定位桥接：
- WebUI 新增 geolocation 发送入口；
- 新增 `TeamApi.sendLocation()`；
- 支持把手机经纬度转换为 `E6` 并发送到板端。

2. WS63 HTTP 定位接口：
- 新增 `GET /api/location`；
- 解析 `lat/lon/dst/speed/heading/battery/fix/sat`；
- 调用 `sle_team_node_send_position()` 发 `POS_REPORT`。

3. 节点位置状态回显：
- member 记录新增位置相关字段；
- 收到 `POS_REPORT` 后缓存到成员状态；
- `/api/nodes` 输出 `latitudeE6/longitudeE6/speedCms/headingDeg/satCount`。

4. 版本与调试资产：
- 新增 `docs/v2/systematic-debugging_phone_location_plan.md`；
- 新增 `docs/v3/README.md`；
- 完成 `webui` API 合同与测试同步。
