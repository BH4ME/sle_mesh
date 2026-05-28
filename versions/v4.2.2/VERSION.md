# Version v4.2.2

版本定位：

- 在 `v4.2.1` 基线上恢复手机定位桥接，并新增“自动持续上报”模式，满足“手机前台持续采集并自动发送 POS_REPORT”。
- 命名约定明确：当前版本号是 `v4.2.2`；`v4.1` 仅表示沿用的板级硬件约束。

本版完成：

1. 板端 HTTP 定位接口恢复并增强：
- 恢复 `GET /api/location`；
- 继续支持 `lat/lon/dst/speed/heading/battery/fix/sat` 参数；
- 返回 JSON 增加 `ret` 与 `reason` 字段，便于快速区分 `NOT_READY_OR_NO_ROUTE / WRITE_FAIL` 等发送失败场景。

2. `/pairing` 页面新增 Phone Location 控件：
- 手动发送：`send location`；
- 单次定位并发送：`use phone gps`（`getCurrentPosition`）；
- 自动持续上报：`start auto` / `stop auto`（`watchPosition` + 节流发送）。

3. 文档同步：
- 根 README、WS63 README、WebUI README 同步加入 `/api/location` 与 Phone Location 使用说明。
- `docs/v4/README.md` 增补 `v4.2.1 / v4.2.2` 里程碑摘要。
- 强调手机权限与前后台限制：首次必须授权，锁屏/后台可能暂停网页定位。

回归结论：

- Web 合同测试覆盖了 `/api/location` 路由存在性与 pairing 页 geolocation/watchPosition 结构。
- 该版本面向“手机前台持续位置上传到 SLE”场景。
