# v1.2.5 Manifest

## Protocol

- `include/sle_team_node.h`
  - 增加 `pairing_enabled`。
  - 增加 pending member 表。
  - 增加 leader 配队和 member 入队/退出 API。
  - member/pending 记录增加完整 MAC 和 MAC ready 标志。
- `include/sle_team_packet.h`
  - HELLO body 增加完整 MAC，用于 WebUI 显示和 leader pending 识别。
- `src/sle_team_node.c`
  - leader 配队窗口开启时，未批准 member 的 `HELLO` 进入 pending。
  - leader approve 后加入白名单并发送 `CONFIG + ACK`。
  - member select/leave 会复位 joined 状态和本地节点缓存。

## CLI

- `src/sle_team_cli.c`
  - 增加 `pairing [start|stop|approve <id>|pending]`。
  - 增加 `join <team> <leader> <channel>`。
  - 增加 `leave`。
  - `state` 输出增加 `pairing=...`。

## Board WebUI

- `xc/ws63_team_network/src/ws63_team_network_app.c`
  - 改为统一运行时角色固件，开机默认 `unconfigured`。
  - 从 WiFi MAC 派生 SSID、自标识和内部 route ID。
  - 增加 `/pairing` 页面。
  - 增加 `/api/pending`。
  - 增加 `/api/pairing?action=start|stop|approve&id=...`。
  - 增加 `/api/member/select?team=...&leader=...&channel=...`。
  - 增加 `/api/member/leave`。
  - 操作类接口执行后重定向回 `/pairing`，手机端不再停留在 JSON 页面。

## Hosted WebUI

- `webui/src/protocol/types.ts`
  - `TeamStatus` 增加 `pairingEnabled`。
- `webui/src/api/client.ts`
  - 串口 `state` 解析增加 `pairing=...`。

## Tests

- Packet test passed.
- Network demo test passed with pairing assertions.
- Hosted WebUI build passed.
