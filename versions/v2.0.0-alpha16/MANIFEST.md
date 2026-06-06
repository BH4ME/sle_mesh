# v2.0.0-alpha16 Manifest

## 变更范围

- `webui/src/api/client.ts`
- `webui/src/main.ts`
- `webui/src/protocol/types.ts`
- `webui/src/styles/app.css`
- `webui/src/time.ts`
- `webui/shared/console-pages.json`
- `webui/shared/ws63-api.json`
- `webui/tests/ws63-api-contract.test.mjs`
- `webui/tests/time.test.mjs`
- `webui/README.md`
- `webui/package.json`
- `examples/team_network_demo.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_console_pages.h`
- `versions/README.md`
- `versions/v2.0.0-alpha16/VERSION.md`
- `versions/v2.0.0-alpha16/MANIFEST.md`

## 关键改动

1. 域名 WebUI 接入当前 WS63 HTTP API：
- `GET /api/status`
- `GET /api/nodes`
- `GET /api/events`
- `GET /api/pending`
- `GET /api/role`
- `GET /api/pairing`
- `GET /api/member/select`
- `GET /api/member/leave`
- `GET /api/factory-reset`

2. 烧录固件 WebUI 同步：
- board version 更新到 `ssr=v6`；
- C 端 redirect 响应补充 CORS header，支持域名 WebUI 调用板端 GET 控制动作。

3. 回归测试：
- WebUI 新增 API 契约测试；
- WebUI 新增事件时间格式测试；
- C demo 测试补齐 route metrics JSON 字段断言。

## 验证

```sh
cd webui
npm run build
npm test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test

/tmp/sle_team_network_test
```

浏览器验证：

- Vite 本地页面连接 mock WS63 API；
- 验证 leader / member / unconfigured 三种状态；
- 验证 `/api/pairing?action=start` 跨域 redirect 动作无 console error；
- 验证固件秒数事件时间显示为 `12s`。
