# v3.0.0-alpha2 Manifest

## 变更范围

- `webui/README.md`
- `webui/package.json`
- `webui/src/main.ts`
- `webui/tests/ws63-api-contract.test.mjs`
- `webui/vite.config.ts`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `versions/README.md`
- `versions/v3.0.0-alpha2/VERSION.md`
- `versions/v3.0.0-alpha2/MANIFEST.md`

## 关键改动

1. HTTPS 开发基线：
- Vite 启用 `@vitejs/plugin-basic-ssl`；
- `server.https` 与 `preview.https` 打开；
- 保留 `dev:https` / `preview:https` 脚本作为局域网 HTTPS 入口别名。

2. WebUI 定位入口与发送入口分层：
- WiFi 模式不展示 `/api/send` 表单；
- 手机定位按钮显式为非提交按钮；
- 手机定位面板只在 WiFi 模式展示，串口模式保留单一发送入口。

3. 固件 HTTP 参数解析防注入/防脏尾：
- `team_http_query_u8` / `team_http_query_u16` / `team_http_query_i32` 增加数字后终止符检查；
- 拒绝 `123abc`、`-90x` 之类尾随非法字符参数。

4. 测试补强：
- 新增/更新合同测试，覆盖：
  - WiFi 发送表单 gating；
  - 定位按钮 non-submit；
  - HTTPS 脚本存在性；
  - Vite HTTPS 配置；
  - README HTTPS 操作说明；
  - 固件数值参数终止符校验。

## 验证

```sh
npm --prefix webui test
npm --prefix webui run build
./scripts/simulate_v2.sh
```
