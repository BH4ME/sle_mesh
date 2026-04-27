# 协议总览

当前实现版本：

- `v1.2.2`

当前协议由三层组成：

```text
Mesh Packet
  -> GROUP_DATA Payload
    -> App Packet
      -> App Body
```

## 设计目标

- 主从节点共用同一套协议代码
- 先跑通 `leader/member` 入网和消息收发
- 串口终端可以控制组网和发包
- 外层保留 MeshCore 风格字段，便于后续扩展
- 内层使用项目自定义业务包，便于承载队伍协同数据

## 当前安全状态

当前没有真正加密。

`cipher_mac` 字段现在只是占位，`app_packet` 当前仍是明文。

## 文档结构

- [packet-structure.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/packet-structure.md)
- [messages.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/messages.md)
- [terminal-cli.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/terminal-cli.md)
- [versioning.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/versioning.md)
