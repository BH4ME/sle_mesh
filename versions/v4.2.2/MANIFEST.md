# v4.2.2 Manifest

## 变更范围

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/shared/ws63-api.json`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `docs/v4/README.md`
- `scripts/ws63_build_v4_ubuntu.sh`
- `xc/ws63_team_network/README.md`
- `webui/README.md`
- `versions/README.md`
- `versions/v4.2.2/VERSION.md`
- `versions/v4.2.2/MANIFEST.md`

## 关键改动

1. API：
- 恢复并增强 `GET /api/location`；
- 发送结果 JSON 增加 `ret` 和 `reason` 字段。

2. 板端页面：
- `/pairing` 新增 `Phone Location` 卡片；
- 新增 `pairing-location-form`、`pairing-location-usegps`、`pairing-location-auto`；
- 使用 `navigator.geolocation.getCurrentPosition` 和 `watchPosition` 支持一次性/持续定位发送。

3. 合同测试：
- 路由合同加入 `/api/location`；
- 固件结构测试要求存在 geolocation/watchPosition/clearWatch 标识。

4. 文档：
- 根 README、WS63 README、WebUI README 增补 `/api/location` 与使用说明。
- 统一版本命名文案：当前版本固定标注为 `v4.2.2`，`v4.1` 仅用于描述板级硬件约束。

## 验证

```sh
npm --prefix webui test
```
