# v1.2.8 Manifest

本版本记录 WS63 真实 SLE RSSI 与 member 自动重连修复。

主要变更：

- `include/sle_team_packet.h`
  - 增加 `SLE_TEAM_RSSI_UNKNOWN`，统一表示 RSSI 未知。
- `include/sle_team_node.h`
  - 增加 `rssi_dbm` 运行时回调。
  - 增加 member 侧 `last_leader_seen_s`，用于 leader 超时重入队。
- `src/sle_team_node.c`
  - 心跳 RSSI 使用回调读取真实值。
  - member 超过心跳超时未收到 leader 包时自动清除 joined 并重新发送 HELLO。
- `src/sle_team_web_api.c`
  - JSON 中未知 RSSI 输出为 `null`。
- `webui/src/api/client.ts`
  - CLI/模拟解析中把 `127` 识别为 RSSI unknown。
- `webui/src/main.ts`
  - UI 中未知 RSSI 显示 `RSSI NA`。
- `xc/ws63_team_network/sle_uart_client/`
  - 项目内复制 SLE UART client 样例。
  - 集成 `sle_read_remote_device_rssi` 与 `read_rssi_cb`。
  - 增加 `sle_uart_client_is_ready()` 和 `sle_uart_client_force_rescan()`。
- `xc/ws63_team_network/sle_uart_server/`
  - 项目内复制 SLE UART server 样例。
  - 集成 `sle_read_remote_device_rssi` 与 `read_rssi_cb`。
  - 断开后重新启动 announce。
- `xc/ws63_team_network/src/ws63_team_network_app.c`
  - leader/member 根据角色读取本端 SLE RSSI。
  - member `NOT_READY` / `WRITE_FAIL` 时触发节流重扫。
  - 继续保留统一固件运行时角色配置。
- `xc/ws63_team_network/CMakeLists.txt`
  - 使用项目内 SLE UART client/server 源码，避免依赖 SDK 外部样例路径。

验证：

```sh
npm run build

UBUNTU_HOST=192.168.6.130 UBUNTU_USER=owen UBUNTU_PASS='67215837' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 BUILD_JOBS=4 \
scripts/ws63_build_team_ubuntu.sh unified
```

结果：

- Vite/TypeScript WebUI build success。
- ws63-liteos-app build success。
- packet success。
- unified firmware copied back to local output path。
- 两块 WS63 均已烧录同一统一固件。
- 串口验证 SLE RSSI、HELLO、CONFIG、ACK、双向 HEARTBEAT 正常。
