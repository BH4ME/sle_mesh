# v2.0.0-alpha17 Manifest

## 变更范围

- `webui/src/api/client.ts`
- `webui/src/api/http.ts`
- `webui/src/main.ts`
- `webui/tests/http-timeout.test.mjs`
- `versions/README.md`
- `versions/v2.0.0-alpha17/VERSION.md`
- `versions/v2.0.0-alpha17/MANIFEST.md`

## 关键改动

1. HTTP client 抽出：
- 新增 `webui/src/api/http.ts`；
- `fetchJson` / `fetchAction` 统一加请求超时；
- GET 请求继续保持 simple request，不额外添加 JSON content-type。

2. 刷新策略调整：
- WiFi 刷新改为顺序读取 `status -> nodes -> events -> pending`；
- pending 失败不影响主状态刷新；
- fetch hang 后超时释放 busy，后续自动轮询继续运行。

3. 回归测试：
- 新增 `http-timeout.test.mjs` 覆盖 fetch hang 时超时失败；
- 覆盖 GET 不加 `content-type`、POST JSON 保留 `content-type`。

## 验证

```sh
cd webui
npm test
npm run build

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test
```

浏览器验证：

- 使用 mock WS63 API 让 `/api/nodes` 故意挂起；
- WebUI 显示 timeout 后不会永久 busy；
- mock 恢复后，WebUI 自动轮询恢复，无需手动刷新。
