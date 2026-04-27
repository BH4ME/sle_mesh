# v1.2.2 Manifest

## 版本定位

`v1.2.2` 是当前 `sle_mesh` 的基础组网协议版本。

该版本包含：

- MeshCore 风格外层包骨架
- `GROUP_DATA` 包装层
- 明文 `App Packet`
- `leader/member` 组网状态机
- 串口终端 CLI
- 本地测试示例

## 源码快照

- [include/sle_team_packet.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/include/sle_team_packet.h)
- [include/sle_team_node.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/include/sle_team_node.h)
- [include/sle_team_cli.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/include/sle_team_cli.h)
- [src/sle_team_packet.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/src/sle_team_packet.c)
- [src/sle_team_node.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/src/sle_team_node.c)
- [src/sle_team_cli.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/src/sle_team_cli.c)

## 示例快照

- [examples/team_node_common.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/examples/team_node_common.c)
- [examples/team_network_demo.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/examples/team_network_demo.c)
- [examples/app_terminal_node.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/examples/app_terminal_node.c)

## 文档快照

- [docs/protocol/README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/docs/protocol/README.md)
- [docs/protocol/packet-structure.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/docs/protocol/packet-structure.md)
- [docs/protocol/messages.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/docs/protocol/messages.md)
- [docs/protocol/terminal-cli.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/docs/protocol/terminal-cli.md)
- [docs/protocol/versioning.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/docs/protocol/versioning.md)

## 校验命令

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  src/sle_team_packet.c examples/team_node_common.c \
  -o /tmp/sle_team_packet_test && /tmp/sle_team_packet_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  src/sle_team_packet.c src/sle_team_node.c examples/team_network_demo.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_TERMINAL_TEST \
  src/sle_team_packet.c src/sle_team_node.c src/sle_team_cli.c examples/app_terminal_node.c \
  -o /tmp/sle_team_terminal_test
```

## 安全说明

`v1.2.2` 没有真正加密。

`cipher_mac` 是预留字段，`app_packet` 当前仍为明文。
