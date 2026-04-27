# MeshCore 风格包帧参考

这份说明对应当前仓库里的公共协议代码：

- [include/sle_team_packet.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/include/sle_team_packet.h)
- [src/sle_team_packet.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/src/sle_team_packet.c)
- [examples/team_node_common.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/examples/team_node_common.c)

## 参考来源

本仓库当前采用的是“**参考 MeshCore 包头编码方式，自定义业务载荷**”的方案。

主要参考：

- MeshCore `Packet Format`：<https://docs.meshcore.io/packet_format/>
- MeshCore `Payload Format`：<https://docs.meshcore.io/payloads/>
- MeshCore 仓库：<https://github.com/meshcore-dev/MeshCore>

## 当前采用的外层帧格式

完全沿用 MeshCore v1 的外层组织方式：

```text
[header][transport_codes(optional)][path_length][path][payload]
```

其中：

- `header`：`0bVVPPPPRR`
- `VV`：payload version
- `PPPP`：payload type
- `RR`：route type

当前代码优先使用：

- `payload_type = GROUP_DATA (0x06)`
- `route_type = DIRECT (0x02)` 或 `FLOOD (0x01)`
- `payload_version = v1 (0x00)`

## 当前采用的内层业务载荷

MeshCore 的 `GROUP_DATA` 定义是：

```text
[channel_hash:1][cipher_mac:2][ciphertext...]
```

考虑到你现在先做 `WS63 + SLE` 的基础联调，当前代码里：

- `channel_hash` 保留
- `cipher_mac` 保留
- 真实业务部分先不做加密，直接把应用消息体放到后面

也就是：

```text
[channel_hash][cipher_mac][app_packet]
```

## app_packet 结构

这部分是当前项目自定义的业务包：

```text
[msg_type:1]
[flags:1]
[seq:2]
[team_id:1]
[src_id:1]
[dst_id:1]
[ttl:1]
[body_len:2]
[body:n]
```

这样做的好处是：

- 主从共用一套包结构
- 后面 leader/member 只改业务逻辑，不改编解码
- 后面要换成真正加密时，只需要替换 `GROUP_DATA` 内层

## 已定义业务消息

- `HELLO`
- `HEARTBEAT`
- `POS_REPORT`
- `ALERT`
- `CONFIG`
- `ACK`

## 当前建议

第一阶段先不要接入真正 MeshCore 的加密、签名和路径路由细节，先把下面几件事跑通：

1. 主从双方能共用同一套 `packet.c`
2. 主节点能正确解析队员位置包
3. 能通过 `seq/team_id/src_id/dst_id` 做基础联调
4. 后面再逐步补加密、重传和多跳
