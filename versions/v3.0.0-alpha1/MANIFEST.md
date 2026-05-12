# v3.0.0-alpha1 Manifest

## 变更范围

- `include/sle_team_node.h`
- `src/sle_team_node.c`
- `src/sle_team_web_api.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/src/api/client.ts`
- `webui/src/main.ts`
- `webui/shared/ws63-api.json`
- `webui/tests/ws63-api-contract.test.mjs`
- `webui/README.md`
- `docs/v2/systematic-debugging_phone_location_plan.md`
- `docs/v3/README.md`
- `docs/README.md`
- `README.md`
- `versions/README.md`
- `versions/v3.0.0-alpha1/VERSION.md`
- `versions/v3.0.0-alpha1/MANIFEST.md`

## 关键改动

1. `api/location` 路由打通：
- 新增 HTTP 参数解析（`u16` / `i32`）；
- 定位参数映射到 `sle_team_pos_body_t`；
- 调用 `sle_team_node_send_position()`；
- 返回事件 JSON（成功/失败）。

2. 位置状态可视化：
- 成员记录增加经纬度、速度、航向、卫星数；
- `POS_REPORT` 处理后写入成员缓存；
- `/api/nodes` 输出新字段，WebUI 自动显示坐标。

3. WebUI 手机定位功能：
- 新增 `sendLocation` API；
- 新增“手机定位并发送”按钮；
- geolocation 权限/失败信息前端可见。

4. 版本管理升级：
- 创建 `v3.0.0-alpha1` 版本目录；
- 新增 `docs/v3` 文档入口；
- 将主 README 与 docs 索引纳入 V3 路线。

## 验证

```sh
npm --prefix webui test
npm --prefix webui run build
```
